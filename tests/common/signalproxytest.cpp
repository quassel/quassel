/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "signalproxy.h"

#include <utility>

#include <QByteArray>
#include <QTest>

#include "invocationspy.h"
#include "mockedpeer.h"
#include "syncableobject.h"
#include "testglobal.h"

using namespace ::testing;
using namespace test;

class SignalProxyTest : public QObject, public ::testing::Test
{
    Q_OBJECT

public:
    void SetUp() override
    {
        // Set up peers and connect the signal proxies so they're ready to use
        _clientPeer->setPeer(_serverPeer);
        _serverPeer->setPeer(_clientPeer);
        _clientProxy.addPeer(_clientPeer);
        _serverProxy.addPeer(_serverPeer);
    }

protected:
    SignalProxy _clientProxy{SignalProxy::ProxyMode::Client, this};
    SignalProxy _serverProxy{SignalProxy::ProxyMode::Server, this};
    MockedPeer* _clientPeer{new MockedPeer{this}};
    MockedPeer* _serverPeer{new MockedPeer{this}};
};

// -----------------------------------------------------------------------------------------------------------------------------------------

// Object for testing attached signals
class ProxyObject : public QObject
{
    Q_OBJECT

public:
    using Data = std::pair<int, QString>;
    using Spy = ValueSpy<Data>;

    ProxyObject(Spy* spy)
        : _spy{spy}
    {}

signals:
    void sendData(int, const QString&);
    void sendMoreData(int, const QString&);

public slots:
    void receiveData(int i, const QString& s) { _spy->notify(std::make_pair(i, s)); }
    void receiveExtraData(int i, const QString& s) { _spy->notify(std::make_pair(-i, s.toUpper())); }

private:
    Spy* _spy;
};

TEST_F(SignalProxyTest, attachSignal)
{
    {
        InSequence s;
        EXPECT_CALL(*_clientPeer, Dispatches(RpcCall(Eq(SIGNAL(sendData(int,QString))), ElementsAre(42, "Hello"))));
        EXPECT_CALL(*_clientPeer, Dispatches(RpcCall(Eq(SIGNAL(sendExtraData(int,QString))), ElementsAre(42, "Hello"))));
        EXPECT_CALL(*_serverPeer, Dispatches(RpcCall(Eq("2sendData(int,QString)"), ElementsAre(23, "World"))));
    }

    ProxyObject::Spy clientSpy, serverSpy;
    ProxyObject clientObject{&clientSpy};
    ProxyObject serverObject{&serverSpy};

    // Deliberately not normalize some of the macro invocations
    _clientProxy.attachSignal(&clientObject, SIGNAL(sendData(int, const QString&)));
    _serverProxy.attachSlot(SIGNAL(sendData(int,QString)), &serverObject, SLOT(receiveData(int, const QString&)));

    _clientProxy.attachSignal(&clientObject, SIGNAL(sendMoreData(int,QString)), SIGNAL(sendExtraData(int,QString)));
    _serverProxy.attachSlot(SIGNAL(sendExtraData(int, const QString&)), &serverObject, SLOT(receiveExtraData(int,QString)));

    _serverProxy.attachSignal(&serverObject, SIGNAL(sendData(int,QString)));
    _clientProxy.attachSlot(SIGNAL(sendData(int, const QString&)), &clientObject, SLOT(receiveData(int,QString)));

    emit clientObject.sendData(42, "Hello");
    ASSERT_TRUE(serverSpy.wait());
    EXPECT_EQ(ProxyObject::Data(42, "Hello"), serverSpy.value());

    emit clientObject.sendMoreData(42, "Hello");
    ASSERT_TRUE(serverSpy.wait());
    EXPECT_EQ(ProxyObject::Data(-42, "HELLO"), serverSpy.value());

    emit serverObject.sendData(23, "World");
    ASSERT_TRUE(clientSpy.wait());
    EXPECT_EQ(ProxyObject::Data(23, "World"), clientSpy.value());
}

// -----------------------------------------------------------------------------------------------------------------------------------------

class SyncObj : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

    Q_PROPERTY(int intProperty READ intProperty WRITE setIntProperty NOTIFY intPropertyChanged)
    Q_PROPERTY(QString stringProperty READ stringProperty WRITE setStringProperty NOTIFY stringPropertyChanged)
    Q_PROPERTY(double doubleProperty READ doubleProperty WRITE setDoubleProperty)

public:

    int intProperty() const
    {
        return _intProperty;
    }

    QString stringProperty() const
    {
        return _stringProperty;
    }

    double doubleProperty() const
    {
        return _doubleProperty;
    }

    int syncedInt() const
    {
        return _syncedInt;
    }

    QString syncedString() const
    {
        return _syncedString;
    }

public slots:
    QByteArray initFooData() const
    {
        return _fooData;
    }

    void setInitFooData(const QByteArray& data)
    {
        _fooData = data;
        emit fooDataChanged(data);
    }

    void setIntProperty(int value)
    {
        _intProperty = value;
        SYNC(ARG(value));
        emit intPropertyChanged(value);
    }

    void setStringProperty(const QString& value)
    {
        _stringProperty = value;
        SYNC(ARG(value));
        emit stringPropertyChanged(value);
    }

    // Deliberately no sync nor changed signal
    void setDoubleProperty(double value)
    {
        _doubleProperty = value;
    }

    void syncMethod(int intArg, const QString& stringArg)
    {
        _syncedInt = intArg;
        _syncedString = stringArg;
        SYNC(ARG(intArg), ARG(stringArg));
        SYNC_OTHER(setIntProperty, ARG(intArg));
        SYNC_OTHER(setStringProperty, ARG(stringArg));
        emit syncMethodCalled(intArg, stringArg);
    }

    void requestMethod(const QString& stringArg, int intArg)
    {
        REQUEST(ARG(stringArg), ARG(intArg));
        REQUEST_OTHER(setStringProperty, ARG(stringArg));
        emit requestMethodCalled(stringArg, intArg);
    }

signals:
    void intPropertyChanged(int);
    void stringPropertyChanged(const QString&);
    void fooDataChanged(const QByteArray&);
    void syncMethodCalled(int, const QString&);
    void requestMethodCalled(const QString&, int);

private:
    int _intProperty{};
    QString _stringProperty;
    double _doubleProperty{};
    QByteArray _fooData{"FOO"};
    int _syncedInt{};
    QString _syncedString;
};

TEST_F(SignalProxyTest, syncableObject)
{
    {
        InSequence s;

        // Synchronize
        EXPECT_CALL(*_clientPeer, Dispatches(InitRequest(Eq("SyncObj"), Eq("Foo"))));
        EXPECT_CALL(*_serverPeer, Dispatches(InitData(Eq("SyncObj"), Eq("Foo"), QVariantMap{
                                                          {"stringProperty", "Hello"},
                                                          {"intProperty", 42},
                                                          {"doubleProperty", 4.2},
                                                          {"FooData", "FOO"}
                                                      })));

        // Set int property
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setIntProperty"), ElementsAre(23))));

        // Sync method
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("syncMethod"), ElementsAre(42, "World"))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setIntProperty"), ElementsAre(42))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setStringProperty"), ElementsAre("World"))));

        // Request method
        EXPECT_CALL(*_clientPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("requestMethod"), ElementsAre("Hello", 23))));
        EXPECT_CALL(*_clientPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setStringProperty"), ElementsAre("Hello"))));

        // Update properties (twice)
        EXPECT_CALL(*_clientPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("requestUpdate"), ElementsAre(QVariantMap{
                                                             {"stringProperty", "Quassel"},
                                                             {"intProperty", 17},
                                                             {"doubleProperty", 2.3}
                                                         })))).Times(2);
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setIntProperty"), ElementsAre(17))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setStringProperty"), ElementsAre("Quassel"))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("update"), ElementsAre(QVariantMap{
                                                             {"stringProperty", "Quassel"},
                                                             {"intProperty", 17},
                                                             {"doubleProperty", 2.3}
                                                         }))));
    }

    SignalSpy spy;

    SyncObj clientObject;
    SyncObj serverObject;

    // -- Set initial data

    serverObject.setIntProperty(42);
    serverObject.setStringProperty("Hello");
    serverObject.setDoubleProperty(4.2);
    serverObject.setObjectName("Foo");
    clientObject.setObjectName("Foo");

    // -- Synchronize

    spy.connect(&serverObject, &SyncableObject::initDone);
    _serverProxy.synchronize(&serverObject);
    ASSERT_TRUE(spy.wait());
    spy.connect(&clientObject, &SyncableObject::initDone);
    _clientProxy.synchronize(&clientObject);
    ASSERT_TRUE(spy.wait());

    // -- Check if client-side values are as expected

    EXPECT_EQ(42, clientObject.intProperty());
    EXPECT_EQ("Hello", clientObject.stringProperty());
    EXPECT_EQ(4.2, clientObject.doubleProperty());
    EXPECT_EQ("FOO", clientObject.initFooData());

    // -- Set int property
    spy.connect(&clientObject, &SyncObj::intPropertyChanged);
    serverObject.setIntProperty(23);
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ(23, clientObject.intProperty());

    // -- Sync method

    spy.connect(&clientObject, &SyncObj::syncMethodCalled);
    serverObject.syncMethod(42, "World");
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ(42, clientObject.syncedInt());
    EXPECT_EQ(42, clientObject.intProperty());
    EXPECT_EQ("World", clientObject.syncedString());
    EXPECT_EQ("World", clientObject.stringProperty());

    // -- Request method

    spy.connect(&serverObject, &SyncObj::requestMethodCalled);
    clientObject.requestMethod("Hello", 23);
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ("Hello", serverObject.stringProperty());

    // -- Property update

    QVariantMap propMap{{"intProperty", 17}, {"stringProperty", "Quassel"}, {"doubleProperty", 2.3}};
    spy.connect(&serverObject, &SyncableObject::updatedRemotely);
    clientObject.requestUpdate(propMap);
    ASSERT_TRUE(spy.wait());
    // We don't allow client updates yet, so the values shouldn't have changed
    EXPECT_EQ(23, serverObject.intProperty());
    EXPECT_EQ("Hello", serverObject.stringProperty());
    EXPECT_EQ(4.2, serverObject.doubleProperty());
    // Allow client updates, try again
    serverObject.setAllowClientUpdates(true);
    spy.connect(&clientObject, &SyncableObject::updated);
    clientObject.requestUpdate(propMap);
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ(17, serverObject.intProperty());
    EXPECT_EQ("Quassel", serverObject.stringProperty());
    EXPECT_EQ(2.3, serverObject.doubleProperty());
    EXPECT_EQ(17, clientObject.intProperty());
    EXPECT_EQ("Quassel", clientObject.stringProperty());
    EXPECT_EQ(2.3, clientObject.doubleProperty());
}

#include "signalproxytest.moc"

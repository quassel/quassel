/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

    ProxyObject(Spy* spy, Peer* expectedPeer)
        : _spy{spy}
        , _expectedPeer{expectedPeer}
    {}

signals:
    void sendData(int, const QString&);
    void sendMoreData(int, const QString&);
    void sendToFunctor(int, const QString&);

public slots:
    void receiveData(int i, const QString& s)
    {
        EXPECT_EQ(_expectedPeer, SignalProxy::current()->sourcePeer());
        _spy->notify(std::make_pair(i, s));
    }

    void receiveExtraData(int i, const QString& s)
    {
        EXPECT_EQ(_expectedPeer, SignalProxy::current()->sourcePeer());
        _spy->notify(std::make_pair(-i, s.toUpper()));
    }

private:
    Spy* _spy;
    Peer* _expectedPeer;
};

TEST_F(SignalProxyTest, attachSignal)
{
    {
        InSequence s;
        EXPECT_CALL(*_clientPeer, Dispatches(RpcCall(Eq(SIGNAL(sendData(int,QString))), ElementsAre(42, "Hello"))));
        EXPECT_CALL(*_clientPeer, Dispatches(RpcCall(Eq(SIGNAL(sendExtraData(int,QString))), ElementsAre(42, "Hello"))));
        EXPECT_CALL(*_serverPeer, Dispatches(RpcCall(Eq("2sendData(int,QString)"), ElementsAre(23, "World"))));
        EXPECT_CALL(*_serverPeer, Dispatches(RpcCall(Eq("2sendToFunctor(int,QString)"), ElementsAre(17, "Hi Universe"))));
    }

    ProxyObject::Spy clientSpy, serverSpy;
    ProxyObject clientObject{&clientSpy, _clientPeer};
    ProxyObject serverObject{&serverSpy, _serverPeer};

    // Deliberately not normalize some of the macro invocations
    _clientProxy.attachSignal(&clientObject, &ProxyObject::sendData);
    _serverProxy.attachSlot(SIGNAL(sendData(int,QString)), &serverObject, &ProxyObject::receiveData);

    _clientProxy.attachSignal(&clientObject, &ProxyObject::sendMoreData, SIGNAL(sendExtraData(int, const QString&)));
    _serverProxy.attachSlot(SIGNAL(sendExtraData(int, const QString&)), &serverObject, &ProxyObject::receiveExtraData);

    _serverProxy.attachSignal(&serverObject, &ProxyObject::sendData);
    //_clientProxy.attachSlot(SIGNAL(sendData(int, const QString&)), &clientObject, SLOT(receiveData(int,QString)));
    _clientProxy.attachSlot(SIGNAL(sendData(int, const QString&)), &clientObject, &ProxyObject::receiveData);

    _serverProxy.attachSignal(&serverObject, &ProxyObject::sendToFunctor);
    _clientProxy.attachSlot(SIGNAL(sendToFunctor(int, const QString&)), this, [this, &clientSpy](int i, const QString& s) {
        EXPECT_EQ(_clientPeer, SignalProxy::current()->sourcePeer());
        clientSpy.notify(std::make_pair(i, s));
    });

    emit clientObject.sendData(42, "Hello");
    ASSERT_TRUE(serverSpy.wait());
    EXPECT_EQ(ProxyObject::Data(42, "Hello"), serverSpy.value());

    emit clientObject.sendMoreData(42, "Hello");
    ASSERT_TRUE(serverSpy.wait());
    EXPECT_EQ(ProxyObject::Data(-42, "HELLO"), serverSpy.value());

    emit serverObject.sendData(23, "World");
    ASSERT_TRUE(clientSpy.wait());
    EXPECT_EQ(ProxyObject::Data(23, "World"), clientSpy.value());

    emit serverObject.sendToFunctor(17, "Hi Universe");
    ASSERT_TRUE(clientSpy.wait());
    EXPECT_EQ(ProxyObject::Data(17, "Hi Universe"), clientSpy.value());
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

    int requestInt(const QString& stringArg, int intArg)
    {
        REQUEST(ARG(stringArg), ARG(intArg));
        REQUEST_OTHER(setIntProperty, ARG(intArg));
        emit requestMethodCalled(stringArg, intArg);
        return -intArg;
    }

    QString requestString(const QString& stringArg, int intArg)
    {
        REQUEST(ARG(stringArg), ARG(intArg));
        REQUEST_OTHER(setStringProperty, ARG(stringArg));
        emit requestMethodCalled(stringArg, intArg);
        return stringArg.toUpper();
    }

    // Receive methods can either have the full signature of the request method + return value, or...
    void receiveInt(const QString&, int, int reply)
    {
        _intProperty = reply;
        emit intReceived(reply);
    }

    // ... only the return value
    void receiveString(const QString& reply)
    {
        _stringProperty = reply;
        emit stringReceived(reply);
    }


signals:
    void intPropertyChanged(int);
    void stringPropertyChanged(const QString&);
    void fooDataChanged(const QByteArray&);
    void syncMethodCalled(int, const QString&);
    void requestMethodCalled(const QString&, int);
    void intReceived(int);
    void stringReceived(const QString&);

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
                                                          {"FooData", QByteArray("FOO")}
                                                      })));

        // Set int property
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setIntProperty"), ElementsAre(23))));

        // Sync method
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("syncMethod"), ElementsAre(42, "World"))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setIntProperty"), ElementsAre(42))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setStringProperty"), ElementsAre("World"))));

        // Request method
        EXPECT_CALL(*_clientPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("requestInt"), ElementsAre("Hello", 23))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("receiveInt"), ElementsAre("Hello", 23, -23))));
        EXPECT_CALL(*_clientPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("setIntProperty"), ElementsAre(23))));

        EXPECT_CALL(*_clientPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("requestString"), ElementsAre("Hello", 23))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Foo"), Eq("receiveString"), ElementsAre("HELLO"))));
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

        // Rename object
        EXPECT_CALL(*_serverPeer, Dispatches(RpcCall(Eq("__objectRenamed__"), ElementsAre(QByteArray("SyncObj"), "Bar", "Foo"))));
        EXPECT_CALL(*_serverPeer, Dispatches(SyncMessage(Eq("SyncObj"), Eq("Bar"), Eq("setStringProperty"), ElementsAre("Hi Universe"))));
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

    spy.connect(&clientObject, &SyncObj::intReceived);
    clientObject.requestInt("Hello", 23);
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ(23, serverObject.intProperty());
    EXPECT_EQ(-23, clientObject.intProperty());

    spy.connect(&clientObject, &SyncObj::stringReceived);
    clientObject.requestString("Hello", 23);
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ("Hello", serverObject.stringProperty());
    EXPECT_EQ("HELLO", clientObject.stringProperty());

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

    // -- Rename object
    spy.connect(&clientObject, &SyncObj::stringPropertyChanged);
    serverObject.setObjectName("Bar");
    serverObject.setStringProperty("Hi Universe");
    ASSERT_TRUE(spy.wait());
    EXPECT_EQ("Bar", clientObject.objectName());
    EXPECT_EQ("Hi Universe", clientObject.stringProperty());
}

#include "signalproxytest.moc"

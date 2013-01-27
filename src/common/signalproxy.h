/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef SIGNALPROXY_H
#define SIGNALPROXY_H

#include <QAbstractSocket>
#include <QEvent>
#include <QList>
#include <QHash>
#include <QVariant>
#include <QVariantMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QByteArray>
#include <QTimer>

#include "protocol.h"

class SyncableObject;
struct QMetaObject;


class SignalProxy : public QObject
{
    Q_OBJECT

    class SignalRelay;

public:
    class AbstractPeer;

    enum ProxyMode {
        Server,
        Client
    };

    enum EventType {
        RemovePeerEvent = QEvent::User
    };

    SignalProxy(QObject *parent);
    SignalProxy(ProxyMode mode, QObject *parent);
    virtual ~SignalProxy();

    void setProxyMode(ProxyMode mode);
    inline ProxyMode proxyMode() const { return _proxyMode; }

    void setHeartBeatInterval(int secs);
    inline int heartBeatInterval() const { return _heartBeatInterval; }
    void setMaxHeartBeatCount(int max);
    inline int maxHeartBeatCount() const { return _maxHeartBeatCount; }

    bool addPeer(AbstractPeer *peer);

    bool attachSignal(QObject *sender, const char *signal, const QByteArray &sigName = QByteArray());
    bool attachSlot(const QByteArray &sigName, QObject *recv, const char *slot);

    void synchronize(SyncableObject *obj);
    void stopSynchronize(SyncableObject *obj);

    class ExtendedMetaObject;
    ExtendedMetaObject *extendedMetaObject(const QMetaObject *meta) const;
    ExtendedMetaObject *createExtendedMetaObject(const QMetaObject *meta, bool checkConflicts = false);
    inline ExtendedMetaObject *extendedMetaObject(const QObject *obj) const { return extendedMetaObject(metaObject(obj)); }
    inline ExtendedMetaObject *createExtendedMetaObject(const QObject *obj, bool checkConflicts = false) { return createExtendedMetaObject(metaObject(obj), checkConflicts); }

    bool isSecure() const { return _secure; }
    void dumpProxyStats();
    void dumpSyncMap(SyncableObject *object);
    inline int peerCount() const { return _peers.size(); }

public slots:
    void detachObject(QObject *obj);
    void detachSignals(QObject *sender);
    void detachSlots(QObject *receiver);

protected:
    void customEvent(QEvent *event);
    void sync_call__(const SyncableObject *obj, ProxyMode modeType, const char *funcname, va_list ap);
    void renameObject(const SyncableObject *obj, const QString &newname, const QString &oldname);

private slots:
    void removePeerBySender();
    void objectRenamed(const QByteArray &classname, const QString &newname, const QString &oldname);
    void updateSecureState();

signals:
    void peerRemoved(SignalProxy::AbstractPeer *peer);
    void connected();
    void disconnected();
    void objectInitialized(SyncableObject *);
    void heartBeatIntervalChanged(int secs);
    void maxHeartBeatCountChanged(int max);
    void lagUpdated(int lag);
    void secureStateChanged(bool);

private:
    template<class T>
    class PeerMessageEvent;

    void init();
    void initServer();
    void initClient();

    static const QMetaObject *metaObject(const QObject *obj);

    void removePeer(AbstractPeer *peer);
    void removeAllPeers();

    template<class T>
    void dispatch(const T &protoMessage);

    void handle(AbstractPeer *peer, const Protocol::SyncMessage &syncMessage);
    void handle(AbstractPeer *peer, const Protocol::RpcCall &rpcCall);
    void handle(AbstractPeer *peer, const Protocol::InitRequest &initRequest);
    void handle(AbstractPeer *peer, const Protocol::InitData &initData);

    bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params, QVariant &returnValue);
    bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params = QVariantList());

    void requestInit(SyncableObject *obj);
    QVariantMap initData(SyncableObject *obj) const;
    void setInitData(SyncableObject *obj, const QVariantMap &properties);

    static void disconnectDevice(QIODevice *dev, const QString &reason = QString());

    QSet<AbstractPeer *> _peers;

    // containg a list of argtypes for fast access
    QHash<const QMetaObject *, ExtendedMetaObject *> _extendedMetaObjects;

    // SignalRelay for all manually attached signals
    SignalRelay *_signalRelay;

    // RPC function -> (object, slot ID)
    typedef QPair<QObject *, int> MethodId;
    typedef QMultiHash<QByteArray, MethodId> SlotHash;
    SlotHash _attachedSlots;

    // slaves for sync
    typedef QHash<QString, SyncableObject *> ObjectId;
    QHash<QByteArray, ObjectId> _syncSlave;

    ProxyMode _proxyMode;
    int _heartBeatInterval;
    int _maxHeartBeatCount;

    bool _secure; // determines if all connections are in a secured state (using ssl or internal connections)

    friend class SignalRelay;
    friend class SyncableObject;
    friend class InternalConnection;
    friend class RemoteConnection;
};


// ==================================================
//  ExtendedMetaObject
// ==================================================
class SignalProxy::ExtendedMetaObject
{
    class MethodDescriptor
    {
    public:
        MethodDescriptor(const QMetaMethod &method);
        MethodDescriptor() : _returnType(-1), _minArgCount(-1), _receiverMode(SignalProxy::Client) {}

        inline const QByteArray &methodName() const { return _methodName; }
        inline const QList<int> &argTypes() const { return _argTypes; }
        inline int returnType() const { return _returnType; }
        inline int minArgCount() const { return _minArgCount; }
        inline SignalProxy::ProxyMode receiverMode() const { return _receiverMode; }

    private:
        QByteArray _methodName;
        QList<int> _argTypes;
        int _returnType;
        int _minArgCount;
        SignalProxy::ProxyMode _receiverMode; // Only acceptable as a Sync Call if the receiving SignalProxy is in this mode.
    };


public:
    ExtendedMetaObject(const QMetaObject *meta, bool checkConflicts);

    inline const QByteArray &methodName(int methodId) { return methodDescriptor(methodId).methodName(); }
    inline const QList<int> &argTypes(int methodId) { return methodDescriptor(methodId).argTypes(); }
    inline int returnType(int methodId) { return methodDescriptor(methodId).returnType(); }
    inline int minArgCount(int methodId) { return methodDescriptor(methodId).minArgCount(); }
    inline SignalProxy::ProxyMode receiverMode(int methodId) { return methodDescriptor(methodId).receiverMode(); }

    inline int methodId(const QByteArray &methodName) { return _methodIds.contains(methodName) ? _methodIds[methodName] : -1; }

    inline int updatedRemotelyId() { return _updatedRemotelyId; }

    inline const QHash<QByteArray, int> &slotMap() { return _methodIds; }
    const QHash<int, int> &receiveMap();

    const QMetaObject *metaObject() const { return _meta; }

    static QByteArray methodName(const QMetaMethod &method);
    static QString methodBaseName(const QMetaMethod &method);

private:
    const MethodDescriptor &methodDescriptor(int methodId);

    const QMetaObject *_meta;
    int _updatedRemotelyId; // id of the updatedRemotely() signal - makes things faster

    QHash<int, MethodDescriptor> _methods;
    QHash<QByteArray, int> _methodIds;
    QHash<int, int> _receiveMap; // if slot x is called then hand over the result to slot y
};


// ==================================================
//  AbstractPeer
// ==================================================
class SignalProxy::AbstractPeer : public QObject
{
    Q_OBJECT

public:
    AbstractPeer(QObject *parent = 0) : QObject(parent) {}

    virtual QString description() const = 0;

    virtual void setSignalProxy(SignalProxy *proxy) = 0;

    virtual bool isOpen() const = 0;
    virtual bool isSecure() const = 0;
    virtual bool isLocal() const = 0;

    virtual QString errorString() const { return QString(); }

    virtual int lag() const = 0;

public slots:
    virtual void dispatch(const Protocol::SyncMessage &msg) = 0;
    virtual void dispatch(const Protocol::RpcCall &msg) = 0;
    virtual void dispatch(const Protocol::InitRequest &msg) = 0;
    virtual void dispatch(const Protocol::InitData &msg) = 0;

    virtual void close(const QString &reason = QString()) = 0;

signals:
    void disconnected();
    void error(QAbstractSocket::SocketError);
    void secureStateChanged(bool secure = true);
    void lagUpdated(int msecs);
};

#endif

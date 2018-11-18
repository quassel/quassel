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

#pragma once

#include "common-export.h"

#include <QEvent>
#include <QSet>

#include <functional>
#include <initializer_list>

#include "protocol.h"

struct QMetaObject;
class QIODevice;

class Peer;
class SyncableObject;

class COMMON_EXPORT SignalProxy : public QObject
{
    Q_OBJECT

    class SignalRelay;

public:
    enum ProxyMode {
        Server,
        Client
    };

    enum EventType {
        RemovePeerEvent = QEvent::User
    };

    SignalProxy(QObject *parent);
    SignalProxy(ProxyMode mode, QObject *parent);
    ~SignalProxy() override;

    void setProxyMode(ProxyMode mode);
    inline ProxyMode proxyMode() const { return _proxyMode; }

    void setHeartBeatInterval(int secs);
    inline int heartBeatInterval() const { return _heartBeatInterval; }
    void setMaxHeartBeatCount(int max);
    inline int maxHeartBeatCount() const { return _maxHeartBeatCount; }

    bool addPeer(Peer *peer);

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

    static SignalProxy *current();

    /**@{*/
    /**
     * This method allows to send a signal only to a limited set of peers
     * @param peers A list of peers that should receive it
     * @param closure Code you want to execute within of that restricted environment
     */
    void restrictTargetPeers(QSet<Peer*> peers, std::function<void()> closure);
    void restrictTargetPeers(Peer *peer, std::function<void()> closure) {
        QSet<Peer*> set;
        set.insert(peer);
        restrictTargetPeers(set, std::move(closure));
    }

    //A better version, but only implemented on Qt5 if Initializer Lists exist
#ifdef Q_COMPILER_INITIALIZER_LISTS
    void restrictTargetPeers(std::initializer_list<Peer*> peers, std::function<void()> closure) {
        restrictTargetPeers(QSet<Peer*>(peers), std::move(closure));
    }
#endif
    /**}@*/

    inline int peerCount() const { return _peerMap.size(); }
    QVariantList peerData();

    Peer *peerById(int peerId);

    /**
     * @return If handling a signal, the Peer from which the current signal originates
     */
    Peer *sourcePeer();
    void setSourcePeer(Peer *sourcePeer);

    /**
     * @return If sending a signal, the Peer to which the current signal is directed
     */
    Peer *targetPeer();
    void setTargetPeer(Peer *targetPeer);

public slots:
    void detachObject(QObject *obj);
    void detachSignals(QObject *sender);
    void detachSlots(QObject *receiver);

protected:
    void customEvent(QEvent *event) override;
    void sync_call__(const SyncableObject *obj, ProxyMode modeType, const char *funcname, va_list ap);
    void renameObject(const SyncableObject *obj, const QString &newname, const QString &oldname);

private slots:
    void removePeerBySender();
    void objectRenamed(const QByteArray &classname, const QString &newname, const QString &oldname);
    void updateSecureState();

signals:
    void peerRemoved(Peer *peer);
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

    void removePeer(Peer *peer);
    void removeAllPeers();

    int nextPeerId() {
        return _lastPeerId++;
    }

    template<class T>
    void dispatch(const T &protoMessage);
    template<class T>
    void dispatch(Peer *peer, const T &protoMessage);

    void handle(Peer *peer, const Protocol::SyncMessage &syncMessage);
    void handle(Peer *peer, const Protocol::RpcCall &rpcCall);
    void handle(Peer *peer, const Protocol::InitRequest &initRequest);
    void handle(Peer *peer, const Protocol::InitData &initData);

    template<class T>
    void handle(Peer *, T) { Q_ASSERT(0); }

    bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params, QVariant &returnValue, Peer *peer = nullptr);
    bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params = QVariantList(), Peer *peer = nullptr);

    void requestInit(SyncableObject *obj);
    QVariantMap initData(SyncableObject *obj) const;
    void setInitData(SyncableObject *obj, const QVariantMap &properties);

    static void disconnectDevice(QIODevice *dev, const QString &reason = QString());

    QHash<int, Peer*> _peerMap;

    // containg a list of argtypes for fast access
    QHash<const QMetaObject *, ExtendedMetaObject *> _extendedMetaObjects;

    // SignalRelay for all manually attached signals
    SignalRelay *_signalRelay;

    // RPC function -> (object, slot ID)
    using MethodId = QPair<QObject *, int>;
    using SlotHash = QMultiHash<QByteArray, MethodId>;
    SlotHash _attachedSlots;

    // slaves for sync
    using ObjectId = QHash<QString, SyncableObject *>;
    QHash<QByteArray, ObjectId> _syncSlave;

    ProxyMode _proxyMode;
    int _heartBeatInterval;
    int _maxHeartBeatCount;

    bool _secure; // determines if all connections are in a secured state (using ssl or internal connections)

    int _lastPeerId = 0;

    QSet<Peer *> _restrictedTargets;
    bool _restrictMessageTarget = false;

    Peer *_sourcePeer = nullptr;
    Peer *_targetPeer = nullptr;

    friend class SignalRelay;
    friend class SyncableObject;
    friend class Peer;
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

/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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

#include <QEvent>
#include <QList>
#include <QHash>
#include <QVariant>
#include <QVariantMap>
#include <QPair>
#include <QString>
#include <QByteArray>
#include <QTimer>

class SyncableObject;
struct QMetaObject;

class SignalProxy : public QObject
{
    Q_OBJECT

    class AbstractPeer;
    class IODevicePeer;
    class SignalProxyPeer;

    class SignalRelay;

public:
    enum ProxyMode {
        Server,
        Client
    };

    enum RequestType {
        Sync = 1,
        RpcCall,
        InitRequest,
        InitData,
        HeartBeat,
        HeartBeatReply
    };

    enum ClientConnectionType {
        SignalProxyConnection,
        IODeviceConnection
    };

    enum CustomEvents {
        PeerSignal = QEvent::User,
        RemovePeer
    };

    SignalProxy(QObject *parent);
    SignalProxy(ProxyMode mode, QObject *parent);
    SignalProxy(ProxyMode mode, QIODevice *device, QObject *parent);
    virtual ~SignalProxy();

    void setProxyMode(ProxyMode mode);
    inline ProxyMode proxyMode() const { return _proxyMode; }

    void setHeartBeatInterval(int secs);
    inline int heartBeatInterval() const { return _heartBeatInterval; }
    void setMaxHeartBeatCount(int max);
    inline int maxHeartBeatCount() const { return _maxHeartBeatCount; }

    bool addPeer(QIODevice *iodev);
    bool addPeer(SignalProxy *proxy);
    void removePeer(QObject *peer);
    void removeAllPeers();

    bool attachSignal(QObject *sender, const char *signal, const QByteArray &sigName = QByteArray());
    bool attachSlot(const QByteArray &sigName, QObject *recv, const char *slot);

    void synchronize(SyncableObject *obj);
    void stopSynchronize(SyncableObject *obj);

    //! Writes a QVariant to a device.
    /** The data item is prefixed with the resulting blocksize,
     *  so the corresponding function readDataFromDevice() can check if enough data is available
     *  at the device to reread the item.
     */
    static void writeDataToDevice(QIODevice *dev, const QVariant &item, bool compressed = false);

    //! Reads a data item from a device that has been written by writeDataToDevice().
    /** If not enough data bytes are available, the function returns false and the QVariant reference
     *  remains untouched.
     */
    static bool readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item, bool compressed = false);

    class ExtendedMetaObject;
    ExtendedMetaObject *extendedMetaObject(const QMetaObject *meta) const;
    ExtendedMetaObject *createExtendedMetaObject(const QMetaObject *meta, bool checkConflicts = false);
    inline ExtendedMetaObject *extendedMetaObject(const QObject *obj) const { return extendedMetaObject(metaObject(obj)); }
    inline ExtendedMetaObject *createExtendedMetaObject(const QObject *obj, bool checkConflicts = false) { return createExtendedMetaObject(metaObject(obj), checkConflicts); }

    bool isSecure() const { return _secure; }
    void dumpProxyStats();

public slots:
    void detachObject(QObject *obj);
    void detachSignals(QObject *sender);
    void detachSlots(QObject *receiver);

protected:
    void customEvent(QEvent *event);
    void sync_call__(const SyncableObject *obj, ProxyMode modeType, const char *funcname, va_list ap);
    void renameObject(const SyncableObject *obj, const QString &newname, const QString &oldname);

private slots:
    void dataAvailable();
    void removePeerBySender();
    void objectRenamed(const QByteArray &classname, const QString &newname, const QString &oldname);
    void sendHeartBeat();
    void receiveHeartBeat(AbstractPeer *peer, const QVariantList &params);
    void receiveHeartBeatReply(AbstractPeer *peer, const QVariantList &params);

    void updateSecureState();

signals:
    void peerRemoved(QIODevice *dev);
    void connected();
    void disconnected();
    void objectInitialized(SyncableObject *);
    void lagUpdated(int lag);
    void securityChanged(bool);
    void secureStateChanged(bool);

private:
    void init();
    void initServer();
    void initClient();

    static const QMetaObject *metaObject(const QObject *obj);

    void dispatchSignal(QIODevice *receiver, const RequestType &requestType, const QVariantList &params);
    void dispatchSignal(const RequestType &requestType, const QVariantList &params);

    void receivePackedFunc(AbstractPeer *sender, const QVariant &packedFunc);
    void receivePeerSignal(AbstractPeer *sender, const RequestType &requestType, const QVariantList &params);
    void receivePeerSignal(SignalProxy *sender, const RequestType &requestType, const QVariantList &params);
    void handleSync(AbstractPeer *sender, QVariantList params);
    void handleInitRequest(AbstractPeer *sender, const QVariantList &params);
    void handleInitData(AbstractPeer *sender, const QVariantList &params);
    void handleSignal(const QVariantList &data);

    bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params, QVariant &returnValue);
    bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params = QVariantList());

    void requestInit(SyncableObject *obj);
    QVariantMap initData(SyncableObject *obj) const;
    void setInitData(SyncableObject *obj, const QVariantMap &properties);

    void updateLag(IODevicePeer *peer, int lag);

public:
    void dumpSyncMap(SyncableObject *object);
    inline int peerCount() const { return _peers.size(); }

private:
    static void disconnectDevice(QIODevice *dev, const QString &reason = QString());

    // a Hash of the actual used communication object to it's corresponding peer
    // currently a communication object can either be an arbitrary QIODevice or another SignalProxy
    typedef QHash<QObject *, AbstractPeer *> PeerHash;
    PeerHash _peers;

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
    QTimer _heartBeatTimer;
    int _heartBeatInterval;
    int _maxHeartBeatCount;

    bool _secure; // determines if all connections are in a secured state (using ssl or internal connections)

    friend class SignalRelay;
    friend class SyncableObject;
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
//  Peers
// ==================================================
class SignalProxy::AbstractPeer
{
public:
    enum PeerType {
        NotAPeer = 0,
        IODevicePeer = 1,
        SignalProxyPeer = 2
    };
    AbstractPeer() : _type(NotAPeer) {}
    AbstractPeer(PeerType type) : _type(type) {}
    virtual ~AbstractPeer() {}
    inline PeerType type() const { return _type; }
    virtual void dispatchSignal(const RequestType &requestType, const QVariantList &params) = 0;
    virtual bool isSecure() const = 0;
private:
    PeerType _type;
};


class SignalProxy::IODevicePeer : public SignalProxy::AbstractPeer
{
public:
    IODevicePeer(QIODevice *device, bool compress) : AbstractPeer(AbstractPeer::IODevicePeer), _device(device), byteCount(0), usesCompression(compress), sentHeartBeats(0), lag(0) {}
    virtual void dispatchSignal(const RequestType &requestType, const QVariantList &params);
    virtual bool isSecure() const;
    inline void dispatchPackedFunc(const QVariant &packedFunc) { SignalProxy::writeDataToDevice(_device, packedFunc, usesCompression); }
    QString address() const;
    inline bool isOpen() const { return _device->isOpen(); }
    inline void close() const { _device->close(); }
    inline bool readData(QVariant &item) { return SignalProxy::readDataFromDevice(_device, byteCount, item, usesCompression); }
private:
    QIODevice *_device;
    quint32 byteCount;
    bool usesCompression;
public:
    int sentHeartBeats;
    int lag;
};


class SignalProxy::SignalProxyPeer : public SignalProxy::AbstractPeer
{
public:
    SignalProxyPeer(SignalProxy *sender, SignalProxy *receiver) : AbstractPeer(AbstractPeer::SignalProxyPeer), sender(sender), receiver(receiver) {}
    virtual void dispatchSignal(const RequestType &requestType, const QVariantList &params);
    virtual inline bool isSecure() const { return true; }
private:
    SignalProxy *sender;
    SignalProxy *receiver;
};


#endif

/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <QDebug>
#include <QEvent>
#include <QMetaMethod>
#include <QSet>
#include <QThread>

#include "funchelpers.h"
#include "protocol.h"
#include "types.h"

struct QMetaObject;
class QIODevice;

class Peer;
class SyncableObject;

class COMMON_EXPORT SignalProxy : public QObject
{
    Q_OBJECT

    template<typename Slot, typename Callable = typename FunctionTraits<Slot>::FunctionType>
    class SlotObject;
    class SlotObjectBase;

public:
    enum ProxyMode
    {
        Server,
        Client
    };

    enum EventType
    {
        RemovePeerEvent = QEvent::User
    };

    SignalProxy(QObject* parent);
    SignalProxy(ProxyMode mode, QObject* parent);
    ~SignalProxy() override;

    void setProxyMode(ProxyMode mode);
    inline ProxyMode proxyMode() const { return _proxyMode; }

    void setHeartBeatInterval(int secs);
    inline int heartBeatInterval() const { return _heartBeatInterval; }
    void setMaxHeartBeatCount(int max);
    inline int maxHeartBeatCount() const { return _maxHeartBeatCount; }

    bool addPeer(Peer* peer);

    /**
     * Attaches a signal for remote emission.
     *
     * After calling this method, whenever the sender emits the given signal, an RpcCall message is sent to connected peers.
     * On the other end, a slot can be attached to handle this message by calling attachSlot().

     * By default, the signal name being sent is as if the SIGNAL() macro had been used, i.e. the normalized signature prefixed with a '2'.
     * This can be overridden by explicitly providing the signalName argument.
     *
     * @sa attachSlot
     *
     * @param sender The sender of the signal
     * @param signal The signal itself (given as a member function pointer)
     * @param signalName Optional string to be used instead of the actual signal name. Will be normalized.
     * @returns true if attaching the signal was successful
     */
    template<typename Signal>
    bool attachSignal(const typename FunctionTraits<Signal>::ClassType* sender, Signal signal, const QByteArray& signalName = {});

    /**
     * Attaches a slot to a remote signal.
     *
     * After calling this method, upon receipt of an RpcCall message with a signalName matching the signalName parameter, the given slot
     * is called with the parameters contained in the message. This is intended to be used in conjunction with attachSignal() on the other
     * end of the connection.
     *
     * Normally, the signalName should be given using the SIGNAL() macro; it will be normalized automatically.
     *
     * @sa attachSignal
     *
     * @param signalName Name of the signal as stored in the RpcCall message
     * @param receiver Receiver of the signal
     * @param slot     Slot to be called (given as a member function pointer)
     * @returns true if attaching the slot was successful
     */
    template<typename Slot, typename = std::enable_if_t<std::is_member_function_pointer<Slot>::value>>
    bool attachSlot(const QByteArray& signalName, typename FunctionTraits<Slot>::ClassType* receiver, Slot slot);

    /**
     * @overload
     *
     * Attaches a functor to a remote signal.
     *
     * After calling this method, upon receipt of an RpcCall message with a signalName matching the signalName parameter, the given functor
     * is invoked with the parameters contained in the message. This is intended to be used in conjunction with attachSignal() on the other
     * end of the connection. This overload can be used, for example, with a lambda that accepts arguments matching the RpcCall parameter
     * list.
     *
     * The context parameter controls the lifetime of the connection; if the context is deleted, the functor is deleted as well.
     *
     * @sa attachSignal
     *
     * @param signalName Name of the signal as stored in the RpcCall message
     * @param context QObject context controlling the lifetime of the callable
     * @param slot    The functor to be invoked
     * @returns true if attaching the functor was successful
     */
    template<typename Slot, typename = std::enable_if_t<!std::is_member_function_pointer<Slot>::value>>
    bool attachSlot(const QByteArray& signalName, const QObject* context, Slot slot);

    void synchronize(SyncableObject* obj);
    void stopSynchronize(SyncableObject* obj);

    class ExtendedMetaObject;
    ExtendedMetaObject* extendedMetaObject(const QMetaObject* meta) const;
    ExtendedMetaObject* createExtendedMetaObject(const QMetaObject* meta, bool checkConflicts = false);
    inline ExtendedMetaObject* extendedMetaObject(const QObject* obj) const { return extendedMetaObject(metaObject(obj)); }
    inline ExtendedMetaObject* createExtendedMetaObject(const QObject* obj, bool checkConflicts = false)
    {
        return createExtendedMetaObject(metaObject(obj), checkConflicts);
    }

    bool isSecure() const { return _secure; }
    void dumpProxyStats();
    void dumpSyncMap(SyncableObject* object);

    static SignalProxy* current();

    /**@{*/
    /**
     * This method allows to send a signal only to a limited set of peers
     * @param peers A list of peers that should receive it
     * @param closure Code you want to execute within of that restricted environment
     */
    void restrictTargetPeers(QSet<Peer*> peers, std::function<void()> closure);
    void restrictTargetPeers(Peer* peer, std::function<void()> closure)
    {
        QSet<Peer*> set;
        set.insert(peer);
        restrictTargetPeers(set, std::move(closure));
    }

    // A better version, but only implemented on Qt5 if Initializer Lists exist
#ifdef Q_COMPILER_INITIALIZER_LISTS
    void restrictTargetPeers(std::initializer_list<Peer*> peers, std::function<void()> closure)
    {
        restrictTargetPeers(QSet<Peer*>(peers), std::move(closure));
    }
#endif
    /**}@*/

    inline int peerCount() const { return _peerMap.size(); }
    QVariantList peerData();

    Peer* peerById(int peerId);

    /**
     * @return If handling a signal, the Peer from which the current signal originates
     */
    Peer* sourcePeer();
    void setSourcePeer(Peer* sourcePeer);

    /**
     * @return If sending a signal, the Peer to which the current signal is directed
     */
    Peer* targetPeer();
    void setTargetPeer(Peer* targetPeer);

protected:
    void customEvent(QEvent* event) override;
    void sync_call__(const SyncableObject* obj, ProxyMode modeType, const char* funcname, va_list ap);
    void renameObject(const SyncableObject* obj, const QString& newname, const QString& oldname);

private slots:
    void removePeerBySender();
    void objectRenamed(const QByteArray& classname, const QString& newname, const QString& oldname);
    void updateSecureState();

signals:
    void peerRemoved(Peer* peer);
    void connected();
    void disconnected();
    void objectInitialized(SyncableObject*);
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

    static const QMetaObject* metaObject(const QObject* obj);

    void removePeer(Peer* peer);
    void removeAllPeers();

    int nextPeerId() { return _lastPeerId++; }

    /**
     * Attaches a SlotObject for the given signal name.
     *
     * @param signalName Signal name to be associated with the SlotObject
     * @param slotObject The SlotObject instance to be invoked for incoming and matching RpcCall messages
     */
    void attachSlotObject(const QByteArray& signalName, std::unique_ptr<SlotObjectBase> slotObject);

    /**
     * Deletes all SlotObjects associated with the given context.
     *
     * @param context The context
     */
    void detachSlotObjects(const QObject* context);

    /**
     * Dispatches an RpcMessage for the given signal and parameters.
     *
     * @param signalName The signal
     * @param params     The parameters
     */
    void dispatchSignal(QByteArray signalName, QVariantList params);

    template<class T>
    void dispatch(const T& protoMessage);
    template<class T>
    void dispatch(Peer* peer, const T& protoMessage);

    void handle(Peer* peer, const Protocol::SyncMessage& syncMessage);
    void handle(Peer* peer, const Protocol::RpcCall& rpcCall);
    void handle(Peer* peer, const Protocol::InitRequest& initRequest);
    void handle(Peer* peer, const Protocol::InitData& initData);

    template<class T>
    void handle(Peer*, T)
    {
        Q_ASSERT(0);
    }

    bool invokeSlot(QObject* receiver, int methodId, const QVariantList& params, QVariant& returnValue, Peer* peer = nullptr);
    bool invokeSlot(QObject* receiver, int methodId, const QVariantList& params = QVariantList(), Peer* peer = nullptr);

    void requestInit(SyncableObject* obj);
    QVariantMap initData(SyncableObject* obj) const;
    void setInitData(SyncableObject* obj, const QVariantMap& properties);

    static void disconnectDevice(QIODevice* dev, const QString& reason = QString());

private:
    QHash<int, Peer*> _peerMap;

    // containg a list of argtypes for fast access
    QHash<const QMetaObject*, ExtendedMetaObject*> _extendedMetaObjects;

    std::unordered_multimap<QByteArray, std::unique_ptr<SlotObjectBase>, Hash<QByteArray>> _attachedSlots;  ///< Attached slot objects

    // slaves for sync
    using ObjectId = QHash<QString, SyncableObject*>;
    QHash<QByteArray, ObjectId> _syncSlave;

    ProxyMode _proxyMode;
    int _heartBeatInterval;
    int _maxHeartBeatCount;

    bool _secure;  // determines if all connections are in a secured state (using ssl or internal connections)

    int _lastPeerId = 0;

    QSet<Peer*> _restrictedTargets;
    bool _restrictMessageTarget = false;

    Peer* _sourcePeer = nullptr;
    Peer* _targetPeer = nullptr;

    friend class SyncableObject;
    friend class Peer;
};

// ---- Template function implementations ---------------------------------------

template<typename Signal>
bool SignalProxy::attachSignal(const typename FunctionTraits<Signal>::ClassType* sender, Signal signal, const QByteArray& signalName)
{
    static_assert(std::is_member_function_pointer<Signal>::value, "Signal must be given as member function pointer");

    // Determine the signalName to be stored in the RpcCall
    QByteArray name;
    if (signalName.isEmpty()) {
        auto method = QMetaMethod::fromSignal(signal);
        if (!method.isValid()) {
            qWarning().nospace() << Q_FUNC_INFO << ": Function pointer is not a signal";
            return false;
        }
        name = "2" + method.methodSignature();  // SIGNAL() prefixes the signature with "2"
    }
    else {
        name = QMetaObject::normalizedSignature(signalName.constData());
    }

    // Upon signal emission, marshall the signal's arguments into a QVariantList and dispatch an RpcCall message
    connect(sender, signal, this, [this, signalName = std::move(name)](auto&&... args) {
        this->dispatchSignal(std::move(signalName), {QVariant::fromValue(args)...});
    });

    return true;
}

template<typename Slot, typename>
bool SignalProxy::attachSlot(const QByteArray& signalName, typename FunctionTraits<Slot>::ClassType* receiver, Slot slot)
{
    // Create a wrapper function that invokes the member function pointer for the receiver instance
    attachSlotObject(signalName, std::make_unique<SlotObject<Slot>>(receiver, [receiver, slot = std::move(slot)](auto&&... args) {
        (receiver->*slot)(std::forward<decltype(args)>(args)...);
    }));
    return true;
}

template<typename Slot, typename>
bool SignalProxy::attachSlot(const QByteArray& signalName, const QObject* context, Slot slot)
{
    static_assert(!std::is_same<Slot, const char*>::value, "Old-style slots not supported");

    attachSlotObject(signalName, std::make_unique<SlotObject<Slot>>(context, std::move(slot)));
    return true;
}

/**
 * Base object for storing a slot (or functor) to be invoked with a list of arguments.
 *
 * @note Having this untemplated base class for SlotObject allows for handling slots in the implementation rather than in the header.
 */
class COMMON_EXPORT SignalProxy::SlotObjectBase
{
public:
    virtual ~SlotObjectBase() = default;

    /**
     * @returns The context associated with the slot
     */
    const QObject* context() const;

    /**
     * Invokes the slot with the given list of parameters.
     *
     * If the parameters cannot all be converted to the slot's argument types, or there is a mismatch in argument count,
     * the invocation will fail.
     *
     * @param params List of arguments marshalled as QVariants
     * @returns true if the invocation was successful
     */
    virtual bool invoke(const QVariantList& params) const = 0;

protected:
    SlotObjectBase(const QObject* context);

private:
    const QObject* _context;
};

/**
 * Specialization of SlotObjectBase for a particular type of slot.
 *
 * Callable may be a function wrapper around a member function pointer of type Slot,
 * or a functor that can be invoked directly.
 *
 * @tparam Slot     Type of the slot, used for determining the callable's signature
 * @tparam Callable Type of the actual callable to be invoked
 */
template<typename Slot, typename Callable>
class SignalProxy::SlotObject : public SlotObjectBase
{
public:
    /**
     * Constructs a SlotObject for the given callable, whose lifetime is controlled by context.
     *
     * @param context  Context object; if destroyed, the slot object will be destroyed as well by SignalProxy.
     * @param callable Callable to be invoked
     */
    SlotObject(const QObject* context, Callable callable)
        : SlotObjectBase(context)
        , _callable(std::move(callable))
    {}

    // See base class
    bool invoke(const QVariantList& params) const override
    {
        if (QThread::currentThread() != context()->thread()) {
            qWarning() << "Cannot call slot in different thread!";
            return false;
        }
        return invokeWithArgsList(_callable, params) ? true : false;
    }

private:
    Callable _callable;
};

// ==================================================
//  ExtendedMetaObject
// ==================================================
class SignalProxy::ExtendedMetaObject
{
    class MethodDescriptor
    {
    public:
        MethodDescriptor(const QMetaMethod& method);
        MethodDescriptor() = default;

        inline const QByteArray& methodName() const { return _methodName; }
        inline const QList<int>& argTypes() const { return _argTypes; }
        inline int returnType() const { return _returnType; }
        inline int minArgCount() const { return _minArgCount; }
        inline SignalProxy::ProxyMode receiverMode() const { return _receiverMode; }

    private:
        QByteArray _methodName;
        QList<int> _argTypes;
        int _returnType{-1};
        int _minArgCount{-1};
        SignalProxy::ProxyMode _receiverMode{
            SignalProxy::Client};  // Only acceptable as a Sync Call if the receiving SignalProxy is in this mode.
    };

public:
    ExtendedMetaObject(const QMetaObject* meta, bool checkConflicts);

    inline const QByteArray& methodName(int methodId) { return methodDescriptor(methodId).methodName(); }
    inline const QList<int>& argTypes(int methodId) { return methodDescriptor(methodId).argTypes(); }
    inline int returnType(int methodId) { return methodDescriptor(methodId).returnType(); }
    inline int minArgCount(int methodId) { return methodDescriptor(methodId).minArgCount(); }
    inline SignalProxy::ProxyMode receiverMode(int methodId) { return methodDescriptor(methodId).receiverMode(); }

    inline int methodId(const QByteArray& methodName) { return _methodIds.contains(methodName) ? _methodIds[methodName] : -1; }

    inline int updatedRemotelyId() { return _updatedRemotelyId; }

    inline const QHash<QByteArray, int>& slotMap() { return _methodIds; }
    const QHash<int, int>& receiveMap();

    const QMetaObject* metaObject() const { return _meta; }

    static QByteArray methodName(const QMetaMethod& method);
    static QString methodBaseName(const QMetaMethod& method);

private:
    const MethodDescriptor& methodDescriptor(int methodId);

    const QMetaObject* _meta;
    int _updatedRemotelyId;  // id of the updatedRemotely() signal - makes things faster

    QHash<int, MethodDescriptor> _methods;
    QHash<QByteArray, int> _methodIds;
    QHash<int, int> _receiveMap;  // if slot x is called then hand over the result to slot y
};

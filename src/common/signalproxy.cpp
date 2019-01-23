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

#include <algorithm>
#include <utility>

#include <QCoreApplication>
#include <QHostAddress>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QThread>

#ifdef HAVE_SSL
#    include <QSslSocket>
#endif

#include "peer.h"
#include "protocol.h"
#include "signalproxy.h"
#include "syncableobject.h"
#include "types.h"
#include "util.h"

using namespace Protocol;

class RemovePeerEvent : public QEvent
{
public:
    RemovePeerEvent(Peer* peer)
        : QEvent(QEvent::Type(SignalProxy::RemovePeerEvent))
        , peer(peer)
    {}
    Peer* peer;
};

// ---- PropertyRelay ----------------------------------------------------------------------------------------------------------------------

/**
 * Relays notify signals of the given property through SyncMessages dispatched by the given SignalProxy.
 *
 * Such a relay is required because Qt allows accessing a property's notify signal only in the form of a
 * QMetaMethod, so pointer-to-member-function connections are unfortunately not possible; thus we cannot
 * wrap the necessary information in a lambda with direct invocation. Instead, all metadata is stored in
 * the relay object.
 * Because we have to use an actual slot, we can't determine the value type at compile time, and thus have
 * to go through the property's read method to access the value at dispatch time rather than rely on the
 * value provided by the signal. On the upside, this approach supports value-less notify signals; this is
 * useful for classes that use a shared changed signal for several properties.
 *
 * @todo Once we can break protocol, properties should be synced explicitly rather than (ab)using SyncMessages.
 *       This would allow to just store the property name in the message, and directly use the property's
 *       read and write methods.
 */
class SignalProxy::PropertyRelay : public QObject
{
    Q_OBJECT

public:
    PropertyRelay(QByteArray className, QByteArray slotName, const QMetaProperty& property, const QMetaMethod& notifySignal, SignalProxy* proxy)
        : QObject(proxy)
        , _property(property)
        , _notifySignal(notifySignal)
        , _dispatchSlot{staticMetaObject.method(staticMetaObject.indexOfMethod("dispatch()"))}
        , _proxy(proxy)
    {
        _syncMessage.className = std::move(className);
        _syncMessage.slotName = std::move(slotName);
    }

    // Cached, because Qt performs a lookup every time for QMetaProperty::notifySignal()
    QMetaMethod notifySignal() const
    {
        return _notifySignal;
    }

    // Cached, because Qt performs a lookup every time when requesting a slot
    QMetaMethod dispatchSlot() const
    {
        return _dispatchSlot;
    }

public slots:
    void dispatch()
    {
        _syncMessage.objectName = sender()->objectName();
        _syncMessage.params = {_property.read(sender())};
        _proxy->dispatch(_syncMessage);
    }

private:
    QMetaProperty _property;
    QMetaMethod _notifySignal;
    QMetaMethod _dispatchSlot;
    SyncMessage _syncMessage;
    SignalProxy* _proxy;
};

// ---- SignalProxy ------------------------------------------------------------------------------------------------------------------------

namespace {
thread_local SignalProxy* _current{nullptr};
}

SignalProxy::SignalProxy(QObject* parent)
    : QObject(parent)
{
    setProxyMode(Client);
    init();
}

SignalProxy::SignalProxy(ProxyMode mode, QObject* parent)
    : QObject(parent)
{
    setProxyMode(mode);
    init();
}

SignalProxy::~SignalProxy()
{
    QHash<QByteArray, ObjectId>::iterator classIter = _syncSlave.begin();
    while (classIter != _syncSlave.end()) {
        ObjectId::iterator objIter = classIter->begin();
        while (objIter != classIter->end()) {
            SyncableObject* obj = objIter.value();
            objIter = classIter->erase(objIter);
            obj->stopSynchronize(this);
        }
        ++classIter;
    }
    _syncSlave.clear();

    removeAllPeers();

    // Ensure that we don't try to clean up while destroying ourselves
    disconnect(this, &QObject::destroyed, this, &SignalProxy::detachSlotObjects);

    _current = nullptr;
}

SignalProxy* SignalProxy::current()
{
    return _current;
}

void SignalProxy::setProxyMode(ProxyMode mode)
{
    if (!_peerMap.empty()) {
        qWarning() << Q_FUNC_INFO << "Cannot change proxy mode while connected";
        return;
    }

    _proxyMode = mode;
    if (mode == Server)
        initServer();
    else
        initClient();
}

void SignalProxy::init()
{
    _heartBeatInterval = 0;
    _maxHeartBeatCount = 0;
    setHeartBeatInterval(30);
    setMaxHeartBeatCount(2);
    _secure = false;
    _current = this;
    updateSecureState();
}

void SignalProxy::initServer() {}

void SignalProxy::initClient()
{
    attachSlot("__objectRenamed__", this, &SignalProxy::objectRenamed);
}

void SignalProxy::setHeartBeatInterval(int secs)
{
    if (_heartBeatInterval != secs) {
        _heartBeatInterval = secs;
        emit heartBeatIntervalChanged(secs);
    }
}

void SignalProxy::setMaxHeartBeatCount(int max)
{
    if (_maxHeartBeatCount != max) {
        _maxHeartBeatCount = max;
        emit maxHeartBeatCountChanged(max);
    }
}

bool SignalProxy::addPeer(Peer* peer)
{
    if (!peer)
        return false;

    if (_peerMap.values().contains(peer))
        return true;

    if (!peer->isOpen()) {
        qWarning("SignalProxy: peer needs to be open!");
        return false;
    }

    if (proxyMode() == Client) {
        if (!_peerMap.isEmpty()) {
            qWarning("SignalProxy: only one peer allowed in client mode!");
            return false;
        }
        connect(peer, &Peer::lagUpdated, this, &SignalProxy::lagUpdated);
    }

    connect(peer, &Peer::disconnected, this, &SignalProxy::removePeerBySender);
    connect(peer, &Peer::secureStateChanged, this, &SignalProxy::updateSecureState);

    if (!peer->parent())
        peer->setParent(this);

    if (peer->id() < 0) {
        peer->setId(nextPeerId());
        peer->setConnectedSince(QDateTime::currentDateTimeUtc());
    }

    _peerMap[peer->id()] = peer;

    peer->setSignalProxy(this);

    if (peerCount() == 1)
        emit connected();

    updateSecureState();
    return true;
}

void SignalProxy::removeAllPeers()
{
    Q_ASSERT(proxyMode() == Server || peerCount() <= 1);
    // wee need to copy that list since we modify it in the loop
    QList<Peer*> peers = _peerMap.values();
    for (auto peer : peers) {
        removePeer(peer);
    }
}

void SignalProxy::removePeer(Peer* peer)
{
    if (!peer) {
        qWarning() << Q_FUNC_INFO << "Trying to remove a null peer!";
        return;
    }

    if (_peerMap.isEmpty()) {
        qWarning() << "SignalProxy::removePeer(): No peers in use!";
        return;
    }

    if (!_peerMap.values().contains(peer)) {
        qWarning() << "SignalProxy: unknown Peer" << peer;
        return;
    }

    disconnect(peer, nullptr, this, nullptr);
    peer->setSignalProxy(nullptr);

    _peerMap.remove(peer->id());
    emit peerRemoved(peer);

    if (peer->parent() == this)
        peer->deleteLater();

    updateSecureState();

    if (_peerMap.isEmpty())
        emit disconnected();
}

void SignalProxy::removePeerBySender()
{
    removePeer(qobject_cast<Peer*>(sender()));
}

void SignalProxy::renameObject(const SyncableObject* obj, const QString& newname, const QString& oldname)
{
    if (proxyMode() == Client)
        return;

    const QMetaObject* meta = obj->syncMetaObject();
    const QByteArray className(meta->className());
    objectRenamed(className, newname, oldname);

    dispatch(RpcCall("__objectRenamed__", QVariantList() << className << newname << oldname));
}

void SignalProxy::objectRenamed(const QByteArray& classname, const QString& newname, const QString& oldname)
{
    if (newname != oldname) {
        if (_syncSlave.contains(classname) && _syncSlave[classname].contains(oldname)) {
            SyncableObject* obj = _syncSlave[classname][newname] = _syncSlave[classname].take(oldname);
            obj->setObjectName(newname);
            requestInit(obj);
        }
    }
}

const QMetaObject* SignalProxy::metaObject(const QObject* obj)
{
    if (const auto* syncObject = qobject_cast<const SyncableObject*>(obj))
        return syncObject->syncMetaObject();
    else
        return obj->metaObject();
}

SignalProxy::ExtendedMetaObject* SignalProxy::extendedMetaObject(const QMetaObject* meta) const
{
    if (_extendedMetaObjects.contains(meta))
        return _extendedMetaObjects[meta];
    else
        return nullptr;
}

SignalProxy::ExtendedMetaObject* SignalProxy::createExtendedMetaObject(const QMetaObject* meta, bool checkConflicts)
{
    if (!_extendedMetaObjects.contains(meta)) {
        _extendedMetaObjects[meta] = new ExtendedMetaObject(meta, checkConflicts);
    }
    return _extendedMetaObjects[meta];
}

void SignalProxy::attachSlotObject(const QByteArray& signalName, std::unique_ptr<SlotObjectBase> slotObject)
{
    // Remove all attached slots related to the context upon its destruction
    connect(slotObject->context(), &QObject::destroyed, this, &SignalProxy::detachSlotObjects, Qt::UniqueConnection);

    _attachedSlots.emplace(QMetaObject::normalizedSignature(signalName.constData()), std::move(slotObject));
}

void SignalProxy::detachSlotObjects(const QObject *context)
{
    for (auto&& it = _attachedSlots.begin(); it != _attachedSlots.end(); ) {
        if (it->second->context() == context) {
            it = _attachedSlots.erase(it);
        }
        else {
            ++it;
        }
    }
}

void SignalProxy::synchronize(SyncableObject* obj)
{
    createExtendedMetaObject(obj, true);

    // attaching as slave to receive sync Calls
    QByteArray className(obj->syncMetaObject()->className());
    _syncSlave[className][obj->objectName()] = obj;

    if (proxyMode() == Server) {
        attachProperties(obj);
        obj->setInitialized();
        emit objectInitialized(obj);
    }
    else {
        if (obj->isInitialized())
            emit objectInitialized(obj);
        else
            requestInit(obj);
    }

    obj->synchronize(this);
}

void SignalProxy::stopSynchronize(SyncableObject* obj)
{
    // we can't use a className here, since it might be effed up, if we receive the call as a result of a decon
    // gladly the objectName() is still valid. So we have only to iterate over the classes not each instance! *sigh*
    QHash<QByteArray, ObjectId>::iterator classIter = _syncSlave.begin();
    while (classIter != _syncSlave.end()) {
        if (classIter->contains(obj->objectName()) && classIter.value()[obj->objectName()] == obj) {
            classIter->remove(obj->objectName());
            break;
        }
        ++classIter;
    }
    obj->stopSynchronize(this);
}

void SignalProxy::attachProperties(const SyncableObject* syncObject)
{
    auto meta = syncObject->syncMetaObject();
    QByteArray className = meta->className();
    auto it = _attachedProperties.find(className);
    // Since the sender instance is determined dynamically, we need only one relay instance per class per property.
    // Cache this to avoid the expensive construction for every instance.
    if (it == _attachedProperties.end()) {
        auto& relays = _attachedProperties[className];
        auto count = meta->propertyCount();
        for (int i = meta->propertyOffset(); i < count; ++i) {
            auto property = meta->property(i);
            auto signal = property.notifySignal();
            if (signal.isValid()) {
                QByteArray setter = SyncableObject::propertySetter(signal);
                if (!setter.isEmpty()) {
                    auto relay = std::make_unique<PropertyRelay>(className, std::move(setter), property, signal, this);
                    connect(syncObject, signal, relay.get(), relay->dispatchSlot());
                    relays.emplace_back(std::move(relay));
                }
                else {
                    qWarning() << "Unsupported NOTIFY signal" << signal.name() << "for property" << property.name() << "of class" << className;
                }
            }
        }
    }
    else {
        for (auto&& relay : it->second) {
            connect(syncObject, relay->notifySignal(), relay.get(), relay->dispatchSlot());
        }
    }
}

void SignalProxy::dispatchSignal(QByteArray sigName, QVariantList params)
{
    RpcCall rpcCall{std::move(sigName), std::move(params)};
    if (_restrictMessageTarget) {
        for (auto&& peer : _restrictedTargets) {
            dispatch(peer, rpcCall);
        }
    }
    else {
        dispatch(rpcCall);
    }
}

template<class T>
void SignalProxy::dispatch(const T& protoMessage)
{
    for (auto&& peer : _peerMap.values()) {
        dispatch(peer, protoMessage);
    }
}

template<class T>
void SignalProxy::dispatch(Peer* peer, const T& protoMessage)
{
    _targetPeer = peer;

    if (peer && peer->isOpen())
        peer->dispatch(protoMessage);
    else
        QCoreApplication::postEvent(this, new ::RemovePeerEvent(peer));

    _targetPeer = nullptr;
}

void SignalProxy::handle(Peer* peer, const SyncMessage& syncMessage)
{
    if (!_syncSlave.contains(syncMessage.className) || !_syncSlave[syncMessage.className].contains(syncMessage.objectName)) {
        qWarning() << QString("no registered receiver for sync call: %1::%2 (objectName=\"%3\"). Params are:")
                          .arg(syncMessage.className, syncMessage.slotName, syncMessage.objectName)
                   << syncMessage.params;
        return;
    }

    SyncableObject* receiver = _syncSlave[syncMessage.className][syncMessage.objectName];

    // In client mode, check if the message contains a property update and directly set its value instead of invoking the slot
    if (proxyMode() == ProxyMode::Client && syncMessage.params.size() == 1) {
        if (receiver->syncProperty(syncMessage.slotName, syncMessage.params.first())) {
            return;
        }
    }

    auto result = receiver->invokeSyncMethod(syncMessage.slotName, syncMessage.params);
    if (result) {
        if (!result->slotName.isEmpty()) {  // Request method provides a result for a matching receive method
            _targetPeer = peer;
            peer->dispatch(*result);
            _targetPeer = nullptr;
        }
        return;
    }

    qDebug().noquote() << QString{"Using legacy sync method invocation for %1::%2 (objectName=\"%3\")"}
                                   .arg(syncMessage.className, syncMessage.slotName, syncMessage.objectName);

#if 1
    ExtendedMetaObject* eMeta = extendedMetaObject(receiver);
    if (!eMeta->slotMap().contains(syncMessage.slotName)) {
        qWarning() << QString("no matching slot for sync call: %1::%2 (objectName=\"%3\"). Params are:")
                          .arg(syncMessage.className, syncMessage.slotName, syncMessage.objectName)
                   << syncMessage.params;
        return;
    }

    int slotId = eMeta->slotMap()[syncMessage.slotName];
    if (proxyMode() != eMeta->receiverMode(slotId)) {
        qWarning("SignalProxy::handleSync(): invokeMethod for \"%s\" failed. Wrong ProxyMode!", eMeta->methodName(slotId).constData());
        return;
    }

    // We can no longer construct a QVariant from QMetaType::Void
    QVariant returnValue;
    int returnType = eMeta->returnType(slotId);
    if (returnType != QMetaType::Void)
        returnValue = QVariant(static_cast<QVariant::Type>(returnType));

    if (!invokeSlot(receiver, slotId, syncMessage.params, returnValue, peer)) {
        qWarning("SignalProxy::handleSync(): invokeMethod for \"%s\" failed ", eMeta->methodName(slotId).constData());
        return;
    }

    if (returnValue.type() != QVariant::Invalid && eMeta->receiveMap().contains(slotId)) {
        int receiverId = eMeta->receiveMap()[slotId];
        QVariantList returnParams;
        if (eMeta->argTypes(receiverId).count() > 1)
            returnParams << syncMessage.params;
        returnParams << returnValue;
        _targetPeer = peer;
        peer->dispatch(SyncMessage(syncMessage.className, syncMessage.objectName, eMeta->methodName(receiverId), returnParams));
        _targetPeer = nullptr;
    }

    // send emit update signal
    invokeSlot(receiver, eMeta->updatedRemotelyId());
#endif
}

void SignalProxy::handle(Peer* peer, const RpcCall& rpcCall)
{
    Q_UNUSED(peer)

    auto range = _attachedSlots.equal_range(rpcCall.signalName);
    std::for_each(range.first, range.second, [&rpcCall](const auto& p) {
        if (!p.second->invoke(rpcCall.params)) {
            qWarning() << "Could not invoke slot for remote signal" << rpcCall.signalName;
        }
    });
}

void SignalProxy::handle(Peer* peer, const InitRequest& initRequest)
{
    if (!_syncSlave.contains(initRequest.className)) {
        qWarning() << "SignalProxy::handleInitRequest() received initRequest for unregistered Class:" << initRequest.className;
        return;
    }

    if (!_syncSlave[initRequest.className].contains(initRequest.objectName)) {
        qWarning() << "SignalProxy::handleInitRequest() received initRequest for unregistered Object:" << initRequest.className
                   << initRequest.objectName;
        return;
    }

    SyncableObject* obj = _syncSlave[initRequest.className][initRequest.objectName];
    _targetPeer = peer;
    peer->dispatch(InitData(initRequest.className, initRequest.objectName, initData(obj)));
    _targetPeer = nullptr;
}

void SignalProxy::handle(Peer* peer, const InitData& initData)
{
    Q_UNUSED(peer)

    if (!_syncSlave.contains(initData.className)) {
        qWarning() << "SignalProxy::handleInitData() received initData for unregistered Class:" << initData.className;
        return;
    }

    if (!_syncSlave[initData.className].contains(initData.objectName)) {
        qWarning() << "SignalProxy::handleInitData() received initData for unregistered Object:" << initData.className << initData.objectName;
        return;
    }

    SyncableObject* obj = _syncSlave[initData.className][initData.objectName];
    setInitData(obj, initData.initData);
}

bool SignalProxy::invokeSlot(QObject* receiver, int methodId, const QVariantList& params, QVariant& returnValue, Peer* peer)
{
    ExtendedMetaObject* eMeta = extendedMetaObject(receiver);
    const QList<int> args = eMeta->argTypes(methodId);
    const int numArgs = params.count() < args.count() ? params.count() : args.count();

    if (eMeta->minArgCount(methodId) > params.count()) {
        qWarning() << "SignalProxy::invokeSlot(): not enough params to invoke" << eMeta->methodName(methodId);
        return false;
    }

    void* _a[] = {nullptr,  // return type...
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr,  // and 10 args - that's the max size qt can handle with signals and slots
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr,
                  nullptr};

    // check for argument compatibility and build params array
    for (int i = 0; i < numArgs; i++) {
        if (!params[i].isValid()) {
            qWarning() << "SignalProxy::invokeSlot(): received invalid data for argument number" << i << "of method"
                       << QString("%1::%2()")
                              .arg(receiver->metaObject()->className())
                              .arg(receiver->metaObject()->method(methodId).methodSignature().constData());
            qWarning() << "                            - make sure all your data types are known by the Qt MetaSystem";
            return false;
        }
        if (args[i] != QMetaType::type(params[i].typeName())) {
            qWarning() << "SignalProxy::invokeSlot(): incompatible param types to invoke" << eMeta->methodName(methodId);
            return false;
        }

        _a[i + 1] = const_cast<void*>(params[i].constData());
    }

    if (returnValue.type() != QVariant::Invalid)
        _a[0] = const_cast<void*>(returnValue.constData());

    Qt::ConnectionType type = QThread::currentThread() == receiver->thread() ? Qt::DirectConnection : Qt::QueuedConnection;

    if (type == Qt::DirectConnection) {
        _sourcePeer = peer;
        auto result = receiver->qt_metacall(QMetaObject::InvokeMetaMethod, methodId, _a) < 0;
        _sourcePeer = nullptr;
        return result;
    }
    else {
        qWarning() << "Queued Connections are not implemented yet";
        // note to self: qmetaobject.cpp:990 ff
        return false;
    }
}

bool SignalProxy::invokeSlot(QObject* receiver, int methodId, const QVariantList& params, Peer* peer)
{
    QVariant ret;
    return invokeSlot(receiver, methodId, params, ret, peer);
}

void SignalProxy::requestInit(SyncableObject* obj)
{
    if (proxyMode() == Server || obj->isInitialized())
        return;

    dispatch(InitRequest(obj->syncMetaObject()->className(), obj->objectName()));
}

QVariantMap SignalProxy::initData(SyncableObject* obj) const
{
    return obj->toVariantMap();
}

void SignalProxy::setInitData(SyncableObject* obj, const QVariantMap& properties)
{
    if (obj->isInitialized())
        return;
    obj->fromVariantMap(properties);
    obj->setInitialized();
    emit objectInitialized(obj);
    invokeSlot(obj, extendedMetaObject(obj)->updatedRemotelyId());
}

void SignalProxy::customEvent(QEvent* event)
{
    switch ((int)event->type()) {
    case RemovePeerEvent: {
        auto* e = static_cast<::RemovePeerEvent*>(event);
        removePeer(e->peer);
        event->accept();
        break;
    }

    default:
        qWarning() << Q_FUNC_INFO << "Received unknown custom event:" << event->type();
        return;
    }
}

void SignalProxy::sync_call__(const SyncableObject* obj, SignalProxy::ProxyMode modeType, const char* funcname, va_list ap)
{
    // qDebug() << obj << modeType << "(" << _proxyMode << ")" << funcname;
    if (modeType != _proxyMode)
        return;

    ExtendedMetaObject* eMeta = extendedMetaObject(obj);

    QVariantList params;

    const QList<int>& argTypes = eMeta->argTypes(eMeta->methodId(QByteArray(funcname)));

    for (int i = 0; i < argTypes.size(); i++) {
        if (argTypes[i] == 0) {
            qWarning() << Q_FUNC_INFO << "received invalid data for argument number" << i << "of signal"
                       << QString("%1::%2").arg(eMeta->metaObject()->className()).arg(funcname);
            qWarning() << "        - make sure all your data types are known by the Qt MetaSystem";
            return;
        }
        params << QVariant(argTypes[i], va_arg(ap, void*));
    }

    if (_restrictMessageTarget) {
        for (auto peer : _restrictedTargets) {
            if (peer != nullptr)
                dispatch(peer, SyncMessage(eMeta->metaObject()->className(), obj->objectName(), QByteArray(funcname), params));
        }
    }
    else
        dispatch(SyncMessage(eMeta->metaObject()->className(), obj->objectName(), QByteArray(funcname), params));
}

void SignalProxy::disconnectDevice(QIODevice* dev, const QString& reason)
{
    if (!reason.isEmpty())
        qWarning() << qPrintable(reason);
    auto* sock = qobject_cast<QAbstractSocket*>(dev);
    if (sock)
        qWarning() << qPrintable(tr("Disconnecting")) << qPrintable(sock->peerAddress().toString());
    dev->close();
}

void SignalProxy::dumpProxyStats()
{
    QString mode;
    if (proxyMode() == Server)
        mode = "Server";
    else
        mode = "Client";

    int slaveCount = 0;
    foreach (ObjectId oid, _syncSlave.values())
        slaveCount += oid.count();

    qDebug() << this;
    qDebug() << "              Proxy Mode:" << mode;
    qDebug() << "          attached Slots:" << _attachedSlots.size();
    qDebug() << " number of synced Slaves:" << slaveCount;
    qDebug() << "number of Classes cached:" << _extendedMetaObjects.count();
}

void SignalProxy::updateSecureState()
{
    bool wasSecure = _secure;

    _secure = !_peerMap.isEmpty();
    for (auto peer : _peerMap.values()) {
        _secure &= peer->isSecure();
    }

    if (wasSecure != _secure)
        emit secureStateChanged(_secure);
}

QVariantList SignalProxy::peerData()
{
    QVariantList result;
    for (auto&& peer : _peerMap.values()) {
        QVariantMap data;
        data["id"] = peer->id();
        data["clientVersion"] = peer->clientVersion();
        // We explicitly rename this, as, due to the Debian reproducability changes, buildDate isn’t actually the build
        // date anymore, but on newer clients the date of the last git commit
        data["clientVersionDate"] = peer->buildDate();
        data["remoteAddress"] = peer->address();
        data["connectedSince"] = peer->connectedSince();
        data["secure"] = peer->isSecure();
        data["features"] = static_cast<quint32>(peer->features().toLegacyFeatures());
        data["featureList"] = peer->features().toStringList();
        result << data;
    }
    return result;
}

Peer* SignalProxy::peerById(int peerId)
{
    // We use ::value() here instead of the [] operator because the latter has the side-effect
    // of automatically inserting a null value with the passed key into the map.  See
    // https://doc.qt.io/qt-5/qhash.html#operator-5b-5d and https://doc.qt.io/qt-5/qhash.html#value.
    return _peerMap.value(peerId);
}

void SignalProxy::restrictTargetPeers(QSet<Peer*> peers, std::function<void()> closure)
{
    auto previousRestrictMessageTarget = _restrictMessageTarget;
    auto previousRestrictedTargets = _restrictedTargets;
    _restrictMessageTarget = true;
    _restrictedTargets = peers;

    closure();

    _restrictMessageTarget = previousRestrictMessageTarget;
    _restrictedTargets = previousRestrictedTargets;
}

Peer* SignalProxy::sourcePeer()
{
    return _sourcePeer;
}

void SignalProxy::setSourcePeer(Peer* sourcePeer)
{
    _sourcePeer = sourcePeer;
}

Peer* SignalProxy::targetPeer()
{
    return _targetPeer;
}

void SignalProxy::setTargetPeer(Peer* targetPeer)
{
    _targetPeer = targetPeer;
}

// ---- SlotObjectBase ---------------------------------------------------------------------------------------------------------------------

SignalProxy::SlotObjectBase::SlotObjectBase(const QObject* context)
    : _context{context}
{}

const QObject* SignalProxy::SlotObjectBase::context() const
{
    return _context;
}

//  ---- ExtendedMetaObject ----------------------------------------------------------------------------------------------------------------

SignalProxy::ExtendedMetaObject::ExtendedMetaObject(const QMetaObject* meta, bool checkConflicts)
    : _meta(meta)
    , _updatedRemotelyId(_meta->indexOfSignal("updatedRemotely()"))
{
    for (int i = 0; i < _meta->methodCount(); i++) {
        if (_meta->method(i).methodType() != QMetaMethod::Slot)
            continue;

        if (_meta->method(i).methodSignature().contains('*'))
            continue;  // skip methods with ptr params

        QByteArray method = methodName(_meta->method(i));
        if (method.startsWith("init"))
            continue;  // skip initializers

        if (_methodIds.contains(method)) {
            /* funny... moc creates for methods containing default parameters multiple metaMethod with separate methodIds.
               we don't care... we just need the full fledged version
             */
            const QMetaMethod& current = _meta->method(_methodIds[method]);
            const QMetaMethod& candidate = _meta->method(i);
            if (current.parameterTypes().count() > candidate.parameterTypes().count()) {
                int minCount = candidate.parameterTypes().count();
                QList<QByteArray> commonParams = current.parameterTypes().mid(0, minCount);
                if (commonParams == candidate.parameterTypes())
                    continue;  // we already got the full featured version
            }
            else {
                int minCount = current.parameterTypes().count();
                QList<QByteArray> commonParams = candidate.parameterTypes().mid(0, minCount);
                if (commonParams == current.parameterTypes()) {
                    _methodIds[method] = i;  // use the new one
                    continue;
                }
            }
            if (checkConflicts) {
                qWarning() << "class" << meta->className() << "contains overloaded methods which is currently not supported!";
                qWarning() << " - " << _meta->method(i).methodSignature() << "conflicts with"
                           << _meta->method(_methodIds[method]).methodSignature();
            }
            continue;
        }
        _methodIds[method] = i;
    }
}

const SignalProxy::ExtendedMetaObject::MethodDescriptor& SignalProxy::ExtendedMetaObject::methodDescriptor(int methodId)
{
    if (!_methods.contains(methodId)) {
        _methods[methodId] = MethodDescriptor(_meta->method(methodId));
    }
    return _methods[methodId];
}

const QHash<int, int>& SignalProxy::ExtendedMetaObject::receiveMap()
{
    if (_receiveMap.isEmpty()) {
        QHash<int, int> receiveMap;

        QMetaMethod requestSlot;
        QByteArray returnTypeName;
        QByteArray signature;
        QByteArray methodName;
        QByteArray params;
        int paramsPos;
        int receiverId;
        const int methodCount = _meta->methodCount();
        for (int i = 0; i < methodCount; i++) {
            requestSlot = _meta->method(i);
            if (requestSlot.methodType() != QMetaMethod::Slot)
                continue;

            returnTypeName = requestSlot.typeName();
            if (QMetaType::Void == (QMetaType::Type)returnType(i))
                continue;

            signature = requestSlot.methodSignature();
            if (!signature.startsWith("request"))
                continue;

            paramsPos = signature.indexOf('(');
            if (paramsPos == -1)
                continue;

            methodName = signature.left(paramsPos);
            params = signature.mid(paramsPos);

            methodName = methodName.replace("request", "receive");
            params = params.left(params.count() - 1) + ", " + returnTypeName + ")";

            signature = QMetaObject::normalizedSignature(methodName + params);
            receiverId = _meta->indexOfSlot(signature);

            if (receiverId == -1) {
                signature = QMetaObject::normalizedSignature(methodName + "(" + returnTypeName + ")");
                receiverId = _meta->indexOfSlot(signature);
            }

            if (receiverId != -1) {
                receiveMap[i] = receiverId;
            }
        }
        _receiveMap = receiveMap;
    }
    return _receiveMap;
}

QByteArray SignalProxy::ExtendedMetaObject::methodName(const QMetaMethod& method)
{
    QByteArray sig(method.methodSignature());
    return sig.left(sig.indexOf("("));
}

SignalProxy::ExtendedMetaObject::MethodDescriptor::MethodDescriptor(const QMetaMethod& method)
    : _methodName(SignalProxy::ExtendedMetaObject::methodName(method))
    , _returnType(QMetaType::type(method.typeName()))
{
    // determine argTypes
    QList<QByteArray> paramTypes = method.parameterTypes();
    QList<int> argTypes;
    for (int i = 0; i < paramTypes.count(); i++) {
        argTypes.append(QMetaType::type(paramTypes[i]));
    }
    _argTypes = argTypes;

    // determine minArgCount
    QString signature(method.methodSignature());
    _minArgCount = method.parameterTypes().count() - signature.count("=");

    _receiverMode = (_methodName.startsWith("request")) ? SignalProxy::Server : SignalProxy::Client;
}

#include "signalproxy.moc"

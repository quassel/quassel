/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "signalproxy.h"

#include <QObject>
#include <QIODevice>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QSet>
#include <QDebug>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QRegExp>
#ifdef HAVE_SSL
#include <QSslSocket>
#endif
#include <QThread>
#include <QTime>
#include <QEvent>
#include <QCoreApplication>

#include "syncableobject.h"
#include "util.h"

// ==================================================
//  PeerSignalEvent
// ==================================================
class PeerSignalEvent : public QEvent {
public:
  PeerSignalEvent(SignalProxy *sender, SignalProxy::RequestType requestType, const QVariantList &params) : QEvent(QEvent::Type(SignalProxy::PeerSignal)), sender(sender), requestType(requestType), params(params) {}
  SignalProxy *sender;
  SignalProxy::RequestType requestType;
  QVariantList params;
};

class RemovePeerEvent : public QEvent {
public:
  RemovePeerEvent(QObject *peer) : QEvent(QEvent::Type(SignalProxy::RemovePeer)), peer(peer) {}
  QObject *peer;
};

// ==================================================
//  Relays
// ==================================================
class SignalProxy::Relay : public QObject {
/* Q_OBJECT is not necessary or even allowed, because we implement
   qt_metacall ourselves (and don't use any other features of the meta
   object system)
*/
public:
  Relay(SignalProxy *parent) : QObject(parent), _proxy(parent) {}

  inline SignalProxy *proxy() const { return _proxy; }

private:
  SignalProxy *_proxy;
};

class SignalProxy::SyncRelay : public SignalProxy::Relay {
public:
  SyncRelay(SignalProxy::ExtendedMetaObject *eMeta, SignalProxy *parent);

  int qt_metacall(QMetaObject::Call _c, int _id, void **_a);

  void synchronize(SyncableObject *object);

  int signalCount() const { return _signalCount; }

private:
  ExtendedMetaObject *_eMeta;
  QSet<int> _signalIds;
  int _signalCount;
};

class SignalProxy::SignalRelay : public SignalProxy::Relay {
public:
  SignalRelay(SignalProxy *parent) : Relay(parent) {}

  int qt_metacall(QMetaObject::Call _c, int _id, void **_a);

  void attachSignal(QObject *sender, int signalId, const QByteArray &funcName);
  void detachSignal(QObject *sender, int signalId = -1);

private:
  struct Signal {
    QObject *sender;
    int signalId;
    QByteArray signature;
    Signal(QObject *sender, int sigId, const QByteArray &signature) : sender(sender), signalId(sigId), signature(signature) {}
    Signal() : sender(0), signalId(-1) {}
  };

  QHash<int, Signal> _slots;
};

SignalProxy::SyncRelay::SyncRelay(SignalProxy::ExtendedMetaObject *eMeta, SignalProxy *parent)
  : Relay(parent),
    _eMeta(eMeta)
{
  QByteArray signature;
  for(int i = 0; i < eMeta->metaObject()->methodCount(); i++ ) {
    signature = eMeta->metaObject()->method(i).signature();
    if(!eMeta->syncMap().contains(signature))
      continue;

    if((proxy()->proxyMode() == SignalProxy::Server && !signature.contains("Requested")) ||
       (proxy()->proxyMode() == SignalProxy::Client && signature.contains("Requested"))) {
      _signalIds << i;
    }
  }
  _signalCount = _signalIds.count();
}

void SignalProxy::SyncRelay::synchronize(SyncableObject *object) {
  if(object->syncMetaObject() != _eMeta->metaObject()) {
    qWarning() << Q_FUNC_INFO << "can't use SyncRelay of class" << _eMeta->metaObject()->className()
	       << "for SyncableObject" << object << "with syncMetaObject of class" << object->syncMetaObject()->className();
    return;
  }

  QSet<int>::const_iterator sigIter = _signalIds.constBegin();
  while(sigIter != _signalIds.constEnd()) {
    QMetaObject::connect(object, *sigIter, this, QObject::staticMetaObject.methodCount() + *sigIter);
    sigIter++;
  }
}

int SignalProxy::SyncRelay::qt_metacall(QMetaObject::Call _c, int _id, void **_a) {
  _id = QObject::qt_metacall(_c, _id, _a);
  if(_id < 0)
    return _id;

  if(_c == QMetaObject::InvokeMetaMethod) {
    if(!_signalIds.contains(_id)) {
      qWarning() << Q_FUNC_INFO << "_id" << _id << "is not a valid SyncMethod!";
      return _id;
    }

    QVariantList params;

    params << _eMeta->metaObject()->className()
	   << sender()->objectName()
	   << _eMeta->metaObject()->method(_id).signature();

    const QList<int> &argTypes = _eMeta->argTypes(_id);
    for(int i = 0; i < argTypes.size(); i++) {
      if(argTypes[i] == 0) {
	qWarning() << Q_FUNC_INFO << "received invalid data for argument number" << i << "of signal" << QString("%1::%2").arg(_eMeta->metaObject()->className()).arg(_eMeta->metaObject()->method(_id).signature());
	qWarning() << "        - make sure all your data types are known by the Qt MetaSystem";
	return _id;
      }
      params << QVariant(argTypes[i], _a[i+1]);
    }

    proxy()->dispatchSignal(SignalProxy::Sync, params);
  }
  _id -= _signalCount;
  return _id;
}


void SignalProxy::SignalRelay::attachSignal(QObject *sender, int signalId, const QByteArray &funcName) {
  // we ride without safetybelts here... all checking for valid method etc pp has to be done by the caller
  // all connected methodIds are offset by the standard methodCount of QObject
  int slotId;
  for(int i = 0;; i++) {
    if(!_slots.contains(i)) {
      slotId = i;
      break;
    }
  }

  QByteArray fn;
  if(!funcName.isEmpty()) {
    fn = QMetaObject::normalizedSignature(funcName);
  } else {
    fn = SIGNAL();
    fn += sender->metaObject()->method(signalId).signature();
  }

  _slots[slotId] = Signal(sender, signalId, fn);

  QMetaObject::connect(sender, signalId, this, QObject::staticMetaObject.methodCount() + slotId);
}

void SignalProxy::SignalRelay::detachSignal(QObject *sender, int signalId) {
  QHash<int, Signal>::iterator slotIter = _slots.begin();
  while(slotIter != _slots.end()) {
    if(slotIter->sender == sender && (signalId == -1 || slotIter->signalId == signalId)) {
      slotIter = _slots.erase(slotIter);
      if(signalId != -1)
	break;
    } else {
      slotIter++;
    }
  }
}

int SignalProxy::SignalRelay::qt_metacall(QMetaObject::Call _c, int _id, void **_a) {
  _id = QObject::qt_metacall(_c, _id, _a);
  if(_id < 0)
    return _id;

  if(_c == QMetaObject::InvokeMetaMethod) {
    if(_slots.contains(_id)) {
      QObject *caller = sender();

      SignalProxy::ExtendedMetaObject *eMeta = proxy()->extendedMetaObject(caller->metaObject());
      Q_ASSERT(eMeta);

      const Signal &signal = _slots[_id];

      QVariantList params;
      params << signal.signature;

      const QList<int> &argTypes = eMeta->argTypes(signal.signalId);
      for(int i = 0; i < argTypes.size(); i++) {
	if(argTypes[i] == 0) {
	  qWarning() << "SignalRelay::qt_metacall(): received invalid data for argument number" << i << "of signal" << QString("%1::%2").arg(caller->metaObject()->className()).arg(caller->metaObject()->method(_id).signature());
	  qWarning() << "                            - make sure all your data types are known by the Qt MetaSystem";
	  return _id;
	}
	params << QVariant(argTypes[i], _a[i+1]);
      }

      proxy()->dispatchSignal(SignalProxy::RpcCall, params);
    }
    _id -= _slots.count();
  }
  return _id;
}

// ==================================================
//  Peers
// ==================================================
void SignalProxy::IODevicePeer::dispatchSignal(const RequestType &requestType, const QVariantList &params) {
  QVariantList packedFunc;
  packedFunc << (qint16)requestType
	     << params;
  dispatchPackedFunc(QVariant(packedFunc));
}

bool SignalProxy::IODevicePeer::isSecure() const {
#ifdef HAVE_SSL
  QSslSocket *sslSocket = qobject_cast<QSslSocket *>(_device);
  if(sslSocket)
    return sslSocket->isEncrypted() || sslSocket->localAddress() == QHostAddress::LocalHost || sslSocket->localAddress() == QHostAddress::LocalHostIPv6;
#endif

  QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(_device);
  if(socket)
    return socket->localAddress() == QHostAddress::LocalHost || socket->localAddress() == QHostAddress::LocalHostIPv6;

  return false;
}

QString SignalProxy::IODevicePeer::address() const {
  QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(_device);
  if(socket)
    return socket->peerAddress().toString();
  else
    return QString();
}

void SignalProxy::SignalProxyPeer::dispatchSignal(const RequestType &requestType, const QVariantList &params) {
  Qt::ConnectionType type = QThread::currentThread() == receiver->thread()
    ? Qt::DirectConnection
    : Qt::QueuedConnection;

  if(type == Qt::DirectConnection) {
    receiver->receivePeerSignal(sender, requestType, params);
  } else {
    QCoreApplication::postEvent(receiver, new PeerSignalEvent(sender, requestType, params));
  }
}

// ==================================================
//  SignalProxy
// ==================================================
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

SignalProxy::SignalProxy(ProxyMode mode, QIODevice* device, QObject* parent)
  : QObject(parent)
{
  setProxyMode(mode);
  addPeer(device);
  init();
}

SignalProxy::~SignalProxy() {
  removeAllPeers();
}

void SignalProxy::setProxyMode(ProxyMode mode) {
  PeerHash::iterator peer = _peers.begin();
  while(peer != _peers.end()) {
    if((*peer)->type() != AbstractPeer::IODevicePeer) {
      IODevicePeer *ioPeer = static_cast<IODevicePeer *>(*peer);
      if(ioPeer->isOpen()) {
	qWarning() << "SignalProxy: Cannot change proxy mode while connected";
	return;
      }
    }
    if((*peer)->type() != AbstractPeer::SignalProxyPeer) {
      qWarning() << "SignalProxy: Cannot change proxy mode while connected to another internal SignalProxy";
      return;
    }
    peer++;
  }

  _proxyMode = mode;
  if(mode == Server)
    initServer();
  else
    initClient();
}

void SignalProxy::init() {
  _signalRelay = new SignalRelay(this);
  connect(&_heartBeatTimer, SIGNAL(timeout()), this, SLOT(sendHeartBeat()));
  _heartBeatTimer.start(30 * 1000);
  _secure = false;
  updateSecureState();
}

void SignalProxy::initServer() {
}

void SignalProxy::initClient() {
  attachSlot("__objectRenamed__", this, SLOT(objectRenamed(QByteArray, QString, QString)));
}

bool SignalProxy::addPeer(QIODevice* iodev) {
  if(!iodev)
    return false;

  if(_peers.contains(iodev))
    return true;

  if(proxyMode() == Client && !_peers.isEmpty()) {
    qWarning("SignalProxy: only one peer allowed in client mode!");
    return false;
  }

  if(!iodev->isOpen()) {
    qWarning("SignalProxy::addPeer(QIODevice *iodev): iodev needs to be open!");
    return false;
  }

  connect(iodev, SIGNAL(disconnected()), this, SLOT(removePeerBySender()));
  connect(iodev, SIGNAL(readyRead()), this, SLOT(dataAvailable()));

#ifdef HAVE_SSL
  QSslSocket *sslSocket = qobject_cast<QSslSocket *>(iodev);
  if(sslSocket) {
    connect(iodev, SIGNAL(encrypted()), this, SLOT(updateSecureState()));
  }
#endif

  if(!iodev->parent())
    iodev->setParent(this);

  _peers[iodev] = new IODevicePeer(iodev, iodev->property("UseCompression").toBool());

  if(_peers.count() == 1)
    emit connected();

  updateSecureState();
  return true;
}

bool SignalProxy::addPeer(SignalProxy* proxy) {
  if(!proxy)
    return false;

  if(proxyMode() == proxy->proxyMode()) {
    qWarning() << "SignalProxy::addPeer(): adding a SignalProxy as peer requires one proxy to be server and one client!";
    return false;
  }

  if(_peers.contains(proxy)) {
    return true;
  }

  if(proxyMode() == Client && !_peers.isEmpty()) {
    qWarning("SignalProxy: only one peer allowed in client mode!");
    return false;
  }

  _peers[proxy] = new SignalProxyPeer(this, proxy);
  proxy->addPeer(this);

  if(_peers.count() == 1)
    emit connected();

  updateSecureState();
  return true;
}

void SignalProxy::removeAllPeers() {
  Q_ASSERT(proxyMode() == Server || _peers.count() <= 1);
  // wee need to copy that list since we modify it in the loop
  QList<QObject *> peers = _peers.keys();
  foreach(QObject *peer, peers) {
    removePeer(peer);
  }
}

void SignalProxy::removePeer(QObject* dev) {
  if(_peers.isEmpty()) {
    qWarning() << "SignalProxy::removePeer(): No peers in use!";
    return;
  }

  Q_ASSERT(dev);
  if(!_peers.contains(dev)) {
    qWarning() << "SignalProxy: unknown Peer" << dev;
    return;
  }

  AbstractPeer *peer = _peers[dev];
  _peers.remove(dev);

  disconnect(dev, 0, this, 0);
  if(peer->type() == AbstractPeer::IODevicePeer)
    emit peerRemoved(static_cast<QIODevice *>(dev));

  if(peer->type() == AbstractPeer::SignalProxyPeer) {
    SignalProxy *proxy = static_cast<SignalProxy *>(dev);
    if(proxy->_peers.contains(this))
      proxy->removePeer(this);
  }

  if(dev->parent() == this)
    dev->deleteLater();

  delete peer;

  updateSecureState();

  if(_peers.isEmpty())
    emit disconnected();
}

void SignalProxy::removePeerBySender() {
  removePeer(sender());
}

void SignalProxy::objectRenamed(const QString &newname, const QString &oldname) {
  SyncableObject *syncObject = qobject_cast<SyncableObject *>(sender());
  const QMetaObject *meta = syncObject->syncMetaObject();
  const QByteArray className(meta->className());
  objectRenamed(className, newname, oldname);

  if(proxyMode() == Client)
    return;

  QVariantList params;
  params << "__objectRenamed__" << className << newname << oldname;
  dispatchSignal(RpcCall, params);
}

void SignalProxy::objectRenamed(const QByteArray &classname, const QString &newname, const QString &oldname) {
  if(_syncSlave.contains(classname) && _syncSlave[classname].contains(oldname) && oldname != newname) {
    SyncableObject *obj = _syncSlave[classname][newname] = _syncSlave[classname].take(oldname);
    requestInit(obj);
  }
}

const QMetaObject *SignalProxy::metaObject(const QObject *obj) {
  if(const SyncableObject *syncObject = qobject_cast<const SyncableObject *>(obj))
    return syncObject->syncMetaObject();
  else
    return obj->metaObject();
}

SignalProxy::ExtendedMetaObject *SignalProxy::extendedMetaObject(const QMetaObject *meta) const {
  if(_extendedMetaObjects.contains(meta))
    return _extendedMetaObjects[meta];
  else
    return 0;
}

SignalProxy::ExtendedMetaObject *SignalProxy::createExtendedMetaObject(const QMetaObject *meta) {
  if(!_extendedMetaObjects.contains(meta)) {
    _extendedMetaObjects[meta] = new ExtendedMetaObject(meta);
  }
  return _extendedMetaObjects[meta];
}

bool SignalProxy::attachSignal(QObject *sender, const char* signal, const QByteArray& sigName) {
  const QMetaObject* meta = sender->metaObject();
  QByteArray sig(meta->normalizedSignature(signal).mid(1));
  int methodId = meta->indexOfMethod(sig.constData());
  if(methodId == -1 || meta->method(methodId).methodType() != QMetaMethod::Signal) {
    qWarning() << "SignalProxy::attachSignal(): No such signal" << signal;
    return false;
  }

  createExtendedMetaObject(meta);
  _signalRelay->attachSignal(sender, methodId, sigName);

  disconnect(sender, SIGNAL(destroyed(QObject *)), this, SLOT(detachObject(QObject *)));
  connect(sender, SIGNAL(destroyed(QObject *)), this, SLOT(detachObject(QObject *)));
  return true;
}

bool SignalProxy::attachSlot(const QByteArray& sigName, QObject* recv, const char* slot) {
  const QMetaObject* meta = recv->metaObject();
  int methodId = meta->indexOfMethod(meta->normalizedSignature(slot).mid(1));
  if(methodId == -1 || meta->method(methodId).methodType() == QMetaMethod::Method) {
    qWarning() << "SignalProxy::attachSlot(): No such slot" << slot;
    return false;
  }

  createExtendedMetaObject(meta);

  QByteArray funcName = QMetaObject::normalizedSignature(sigName.constData());
  _attachedSlots.insert(funcName, qMakePair(recv, methodId));

  disconnect(recv, SIGNAL(destroyed(QObject *)), this, SLOT(detachObject(QObject *)));
  connect(recv, SIGNAL(destroyed(QObject *)), this, SLOT(detachObject(QObject *)));
  return true;
}

void SignalProxy::synchronize(SyncableObject *obj) {
  ExtendedMetaObject *eMeta = createExtendedMetaObject(obj);

  SyncRelay *relay;
  if(!_syncRelays.contains(obj->syncMetaObject())) {
    _syncRelays[obj->syncMetaObject()] = new SyncRelay(eMeta, this);
  }
  relay = _syncRelays[obj->syncMetaObject()];

  relay->synchronize(obj);

  // attaching as slave to receive sync Calls
  QByteArray className(obj->syncMetaObject()->className());
  _syncSlave[className][obj->objectName()] = obj;
  disconnect(obj, SIGNAL(destroyed(QObject *)), this, SLOT(detachObject(QObject *)));
  connect(obj, SIGNAL(destroyed(QObject *)), this, SLOT(detachObject(QObject *)));

  if(proxyMode() == Server) {
    connect(obj, SIGNAL(objectRenamed(QString, QString)), this, SLOT(objectRenamed(QString, QString)));
    obj->setInitialized();
    emit objectInitialized(obj);
  } else {
    if(obj->isInitialized())
      emit objectInitialized(obj);
    else
      requestInit(obj);
  }
}

void SignalProxy::detachObject(QObject *obj) {
  stopSync(obj);
  detachSignals(obj);
  detachSlots(obj);
}

void SignalProxy::detachSignals(QObject *sender) {
  _signalRelay->detachSignal(sender);
}

void SignalProxy::detachSlots(QObject* receiver) {
  SlotHash::iterator slotIter = _attachedSlots.begin();
  while(slotIter != _attachedSlots.end()) {
    if(slotIter.value().first == receiver) {
      slotIter = _attachedSlots.erase(slotIter);
    } else
      slotIter++;
  }
}

void SignalProxy::stopSync(QObject *obj) {
  SyncableObject *syncableObject = qobject_cast<SyncableObject *>(obj);
  if(syncableObject && _syncRelays.contains(syncableObject->syncMetaObject())) {
    QObject::disconnect(syncableObject, 0, _syncRelays[syncableObject->syncMetaObject()], 0);
  }

  // we can't use a className here, since it might be effed up, if we receive the call as a result of a decon
  // gladly the objectName() is still valid. So we have only to iterate over the classes not each instance! *sigh*
  QHash<QByteArray, ObjectId>::iterator classIter = _syncSlave.begin();
  while(classIter != _syncSlave.end()) {
    if(classIter->contains(obj->objectName()) && classIter.value()[obj->objectName()] == obj) {
      classIter->remove(obj->objectName());
      break;
    }
    classIter++;
  }
}

void SignalProxy::dispatchSignal(const RequestType &requestType, const QVariantList &params) {
  QVariant packedFunc(QVariantList() << (qint16)requestType << params);
  PeerHash::iterator peer = _peers.begin();
  while(peer != _peers.end()) {
    switch((*peer)->type()) {
    case AbstractPeer::IODevicePeer:
      {
	IODevicePeer *ioPeer = static_cast<IODevicePeer *>(*peer);
	if(ioPeer->isOpen())
	  ioPeer->dispatchPackedFunc(packedFunc);
	else
	  QCoreApplication::postEvent(this, new RemovePeerEvent(peer.key()));
      }
      break;
    case AbstractPeer::SignalProxyPeer:
      (*peer)->dispatchSignal(requestType, params);
      break;
    default:
      Q_ASSERT(false); // there shouldn't be any peers with wrong / unknown type
    }
    peer++;
  }
}

void SignalProxy::receivePackedFunc(AbstractPeer *sender, const QVariant &packedFunc) {
  QVariantList params(packedFunc.toList());

  if(params.isEmpty()) {
    qWarning() << "SignalProxy::receivePeerSignal(): received incompatible Data:" << packedFunc;
    return;
  }

  RequestType requestType = (RequestType)params.takeFirst().value<int>();
  receivePeerSignal(sender, requestType, params);
}

void SignalProxy::receivePeerSignal(AbstractPeer *sender, const RequestType &requestType, const QVariantList &params) {
  switch(requestType) {
  case RpcCall:
    if(params.empty())
      qWarning() << "SignalProxy::receivePeerSignal(): received empty RPC-Call";
    else
      handleSignal(params);
      //handleSignal(params.takeFirst().toByteArray(), params);
    break;

  case Sync:
    handleSync(sender, params);
    break;

  case InitRequest:
    handleInitRequest(sender, params);
    break;

  case InitData:
    handleInitData(sender, params);
    break;

  case HeartBeat:
    receiveHeartBeat(sender, params);
    break;

  case HeartBeatReply:
    receiveHeartBeatReply(sender, params);
    break;

  default:
    qWarning() << "SignalProxy::receivePeerSignal(): received undefined CallType" << requestType << params;
  }
}

void SignalProxy::receivePeerSignal(SignalProxy *sender, const RequestType &requestType, const QVariantList &params) {
  if(!_peers.contains(sender)) {
    // we output only the pointer value. otherwise Qt would try to pretty print. As the object might already been destroyed, this is not a good idea.
    qWarning() << "SignalProxy::receivePeerSignal(): received Signal from unknown Proxy" << reinterpret_cast<void *>(sender);
    return;
  }
  receivePeerSignal(_peers[sender], requestType, params);
}

void SignalProxy::handleSync(AbstractPeer *sender, QVariantList params) {
  if(params.count() < 3) {
    qWarning() << "received invalid Sync call" << params;
    return;
  }

  QByteArray className = params.takeFirst().toByteArray();
  QString objectName = params.takeFirst().toString();
  QByteArray signal = params.takeFirst().toByteArray();

  if(!_syncSlave.contains(className) || !_syncSlave[className].contains(objectName)) {
    qWarning() << QString("no registered receiver for sync call: %1::%2 (objectName=\"%3\"). Params are:").arg(QString(className)).arg(QString(signal)).arg(objectName)
	       << params;
    return;
  }

  SyncableObject *receiver = _syncSlave[className][objectName];
  ExtendedMetaObject *eMeta = extendedMetaObject(receiver);
  if(!eMeta->syncMap().contains(signal)) {
    qWarning() << QString("no matching slot for sync call: %1::%2 (objectName=\"%3\"). Params are:").arg(QString(className)).arg(QString(signal)).arg(objectName)
	       << params;
    return;
  }

  int slotId = eMeta->syncMap()[signal];

  QVariant returnValue((QVariant::Type)eMeta->returnType(slotId));
  if(!invokeSlot(receiver, slotId, params, returnValue)) {
    qWarning("SignalProxy::handleSync(): invokeMethod for \"%s\" failed ", eMeta->methodName(slotId).constData());
    return;
  }

  if(returnValue.type() != QVariant::Invalid && eMeta->receiveMap().contains(slotId)) {
    int receiverId = eMeta->receiveMap()[slotId];
    QVariantList returnParams;
    returnParams << className
		 << objectName
		 << QByteArray(receiver->metaObject()->method(receiverId).signature());
    if(eMeta->argTypes(receiverId).count() > 1)
      returnParams << params;
    returnParams << returnValue;
    sender->dispatchSignal(Sync, returnParams);
  }

  // send emit update signal
  invokeSlot(receiver, eMeta->updatedRemotelyId());
}

void SignalProxy::handleInitRequest(AbstractPeer *sender, const QVariantList &params) {
  if(params.count() != 2) {
    qWarning() << "SignalProxy::handleInitRequest() received initRequest with invalid param Count:"
	       << params;
    return;
  }

  QByteArray className(params[0].toByteArray());
  QString objectName(params[1].toString());

  if(!_syncSlave.contains(className)) {
    qWarning() << "SignalProxy::handleInitRequest() received initRequest for unregistered Class:"
	       << className;
    return;
  }

  if(!_syncSlave[className].contains(objectName)) {
    qWarning() << "SignalProxy::handleInitRequest() received initRequest for unregistered Object:"
	       << className << objectName;
    return;
  }

  SyncableObject *obj = _syncSlave[className][objectName];

  QVariantList params_;
  params_ << className
	  << objectName
	  << initData(obj);

  sender->dispatchSignal(InitData, params_);
}

void SignalProxy::handleInitData(AbstractPeer *sender, const QVariantList &params) {
  Q_UNUSED(sender)
  if(params.count() != 3) {
    qWarning() << "SignalProxy::handleInitData() received initData with invalid param Count:"
	       << params;
    return;
  }

  QByteArray className(params[0].toByteArray());
  QString objectName(params[1].toString());
  QVariantMap propertyMap(params[2].toMap());

  if(!_syncSlave.contains(className)) {
    qWarning() << "SignalProxy::handleInitData() received initData for unregistered Class:"
	       << className;
    return;
  }

  if(!_syncSlave[className].contains(objectName)) {
    qWarning() << "SignalProxy::handleInitData() received initData for unregistered Object:"
	       << className << objectName;
    return;
  }

  SyncableObject *obj = _syncSlave[className][objectName];
  setInitData(obj, propertyMap);
}

//void SignalProxy::handleSignal(const QByteArray &funcName, const QVariantList &params) {
void SignalProxy::handleSignal(const QVariantList &data) {
  QVariantList params = data;
  QByteArray funcName = params.takeFirst().toByteArray();

  QObject* receiver;
  int methodId;
  SlotHash::const_iterator slot = _attachedSlots.constFind(funcName);
  while(slot != _attachedSlots.constEnd() && slot.key() == funcName) {
    receiver = (*slot).first;
    methodId = (*slot).second;
    if(!invokeSlot(receiver, methodId, params)) {
      ExtendedMetaObject *eMeta = extendedMetaObject(receiver);
      qWarning("SignalProxy::handleSignal(): invokeMethod for \"%s\" failed ", eMeta->methodName(methodId).constData());
    }
    slot++;
  }
}

bool SignalProxy::invokeSlot(QObject *receiver, int methodId, const QVariantList &params, QVariant &returnValue) {
  ExtendedMetaObject *eMeta = extendedMetaObject(receiver);
  const QList<int> args = eMeta->argTypes(methodId);
  const int numArgs = params.count() < args.count()
    ? params.count()
    : args.count();

  if(eMeta->minArgCount(methodId) > params.count()) {
      qWarning() << "SignalProxy::invokeSlot(): not enough params to invoke" << eMeta->methodName(methodId);
      return false;
  }

  void *_a[] = {0,              // return type...
		0, 0, 0, 0 , 0, // and 10 args - that's the max size qt can handle with signals and slots
		0, 0, 0, 0 , 0};

  // check for argument compatibility and build params array
  for(int i = 0; i < numArgs; i++) {
    if(!params[i].isValid()) {
      qWarning() << "SignalProxy::invokeSlot(): received invalid data for argument number" << i << "of method" << QString("%1::%2()").arg(receiver->metaObject()->className()).arg(receiver->metaObject()->method(methodId).signature());
      qWarning() << "                            - make sure all your data types are known by the Qt MetaSystem";
      return false;
    }
    if(args[i] != QMetaType::type(params[i].typeName())) {
      qWarning() << "SignalProxy::invokeSlot(): incompatible param types to invoke" << eMeta->methodName(methodId);
      return false;
    }
    _a[i+1] = const_cast<void *>(params[i].constData());
  }

  if(returnValue.type() != QVariant::Invalid)
    _a[0] = const_cast<void *>(returnValue.constData());

  Qt::ConnectionType type = QThread::currentThread() == receiver->thread()
    ? Qt::DirectConnection
    : Qt::QueuedConnection;

  if(type == Qt::DirectConnection) {
    return receiver->qt_metacall(QMetaObject::InvokeMetaMethod, methodId, _a) < 0;
  } else {
    qWarning() << "Queued Connections are not implemented yet";
    // note to self: qmetaobject.cpp:990 ff
    return false;
  }

}

bool SignalProxy::invokeSlot(QObject *receiver, int methodId, const QVariantList &params) {
  QVariant ret;
  return invokeSlot(receiver, methodId, params, ret);
}

void SignalProxy::dataAvailable() {
  // yet again. it's a private slot. no need for checks.
  QIODevice* ioDev = qobject_cast<QIODevice* >(sender());
  Q_ASSERT(_peers.contains(ioDev) && _peers[ioDev]->type() == AbstractPeer::IODevicePeer);
  IODevicePeer *peer = static_cast<IODevicePeer *>(_peers[ioDev]);
  QVariant var;
  while(peer->readData(var))
    receivePackedFunc(peer, var);
}

void SignalProxy::writeDataToDevice(QIODevice *dev, const QVariant &item, bool compressed) {
  QAbstractSocket* sock  = qobject_cast<QAbstractSocket*>(dev);
  if(!dev->isOpen() || (sock && sock->state()!=QAbstractSocket::ConnectedState)) {
    qWarning("SignalProxy: Can't call write on a closed device");
    return;
  }

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_2);
  out << (quint32)0;

  if(compressed) {
    QByteArray rawItem;
    QDataStream itemStream(&rawItem, QIODevice::WriteOnly);

    itemStream.setVersion(QDataStream::Qt_4_2);
    itemStream << item;

    rawItem = qCompress(rawItem);

    out << rawItem;
  } else {
    out << item;
  }

  out.device()->seek(0);
  out << (quint32)(block.size() - sizeof(quint32));

  dev->write(block);
}

bool SignalProxy::readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item, bool compressed) {
  QDataStream in(dev);
  in.setVersion(QDataStream::Qt_4_2);

  if(blockSize == 0) {
    if(dev->bytesAvailable() < (int)sizeof(quint32)) return false;
    in >> blockSize;
  }

  if(blockSize > 1 << 22) {
    disconnectDevice(dev, tr("Client tried to send package larger than max package size!"));
    return false;
  }

  if(blockSize == 0) {
    disconnectDevice(dev, tr("Client tried to send 0 byte package!"));
    return false;
  }

  if(dev->bytesAvailable() < blockSize)
    return false;

  blockSize = 0;

  if(compressed) {
    QByteArray rawItem;
    in >> rawItem;

    int nbytes = rawItem.size();
    if(nbytes <= 4) {
      const char *data = rawItem.constData();
      if(nbytes < 4 || (data[0]!=0 || data[1]!=0 || data[2]!=0 || data[3]!=0)) {
	disconnectDevice(dev, tr("Client sent corrupted compressed data!"));
	return false;
      }
    }

    rawItem = qUncompress(rawItem);

    QDataStream itemStream(&rawItem, QIODevice::ReadOnly);
    itemStream.setVersion(QDataStream::Qt_4_2);
    itemStream >> item;
  } else {
    in >> item;
  }

  if(!item.isValid()) {
    disconnectDevice(dev, tr("Client sent corrupt data: unable to load QVariant!"));
    return false;
  }

  return true;
}

void SignalProxy::requestInit(SyncableObject *obj) {
  if(proxyMode() == Server || obj->isInitialized())
    return;

  QVariantList params;
  params << obj->syncMetaObject()->className()
	 << obj->objectName();
  dispatchSignal(InitRequest, params);
}

QVariantMap SignalProxy::initData(SyncableObject *obj) const {
  return obj->toVariantMap();
}

void SignalProxy::setInitData(SyncableObject *obj, const QVariantMap &properties) {
  if(obj->isInitialized())
    return;
  obj->fromVariantMap(properties);
  obj->setInitialized();
  emit objectInitialized(obj);
  invokeSlot(obj, extendedMetaObject(obj)->updatedRemotelyId());
}

void SignalProxy::sendHeartBeat() {
  QVariantList heartBeatParams;
  heartBeatParams << QTime::currentTime();
  PeerHash::iterator peer = _peers.begin();
  while(peer != _peers.end()) {
    if((*peer)->type() == AbstractPeer::IODevicePeer) {
      IODevicePeer *ioPeer = static_cast<IODevicePeer *>(*peer);
      ioPeer->dispatchSignal(SignalProxy::HeartBeat, heartBeatParams);
      if(ioPeer->sentHeartBeats > 0) {
	updateLag(ioPeer, ioPeer->sentHeartBeats * _heartBeatTimer.interval());
      }
      if(ioPeer->sentHeartBeats > 1) {
	qWarning() << "SignalProxy: Disconnecting peer:" << ioPeer->address()
		   << "(didn't receive a heartbeat for over" << ioPeer->sentHeartBeats * _heartBeatTimer.interval() / 1000 << "seconds)";
	ioPeer->close();
      } else {
	ioPeer->sentHeartBeats++;
      }
    }
    peer++;
  }
}

void SignalProxy::receiveHeartBeat(AbstractPeer *peer, const QVariantList &params) {
  peer->dispatchSignal(SignalProxy::HeartBeatReply, params);
}

void SignalProxy::receiveHeartBeatReply(AbstractPeer *peer, const QVariantList &params) {
  if(peer->type() != AbstractPeer::IODevicePeer) {
    qWarning() << "SignalProxy::receiveHeartBeatReply: received heart beat from a non IODevicePeer!";
    return;
  }

  IODevicePeer *ioPeer = static_cast<IODevicePeer *>(peer);
  ioPeer->sentHeartBeats = 0;

  if(params.isEmpty()) {
    qWarning() << "SignalProxy: received heart beat reply with less params then sent from:" << ioPeer->address();
    return;
  }

  QTime sendTime = params[0].value<QTime>();
  updateLag(ioPeer, sendTime.msecsTo(QTime::currentTime()) / 2);
}

void SignalProxy::customEvent(QEvent *event) {
  switch(event->type()) {
  case PeerSignal:
    {
      PeerSignalEvent *e = static_cast<PeerSignalEvent *>(event);
      receivePeerSignal(e->sender, e->requestType, e->params);
    }
    event->accept();
    break;
  case RemovePeer:
    {
      RemovePeerEvent *e = static_cast<RemovePeerEvent *>(event);
      removePeer(e->peer);
    }
    event->accept();
  default:
    return;
  }
}

void SignalProxy::disconnectDevice(QIODevice *dev, const QString &reason) {
  if(!reason.isEmpty())
    qWarning() << qPrintable(reason);
  QAbstractSocket *sock  = qobject_cast<QAbstractSocket *>(dev);
  if(sock)
    qWarning() << qPrintable(tr("Disconnecting")) << qPrintable(sock->peerAddress().toString());
  dev->close();
}

void SignalProxy::updateLag(IODevicePeer *peer, int lag) {
  peer->lag = lag;
  if(proxyMode() == Client) {
    emit lagUpdated(lag);
  }
}

void SignalProxy::dumpProxyStats() {
  QString mode;
  if(proxyMode() == Server)
    mode = "Server";
  else
    mode = "Client";

  int sigCount = 0;
  foreach(SyncRelay *relay, _syncRelays.values())
    sigCount += relay->signalCount();

  int slaveCount = 0;
  foreach(ObjectId oid, _syncSlave.values())
    slaveCount += oid.count();

  qDebug() << this;
  qDebug() << "              Proxy Mode:" << mode;
  // qDebug() << "attached sending Objects:" << _relayHash.count();
  qDebug() << "       number of Signals:" << sigCount;
  qDebug() << "          attached Slots:" << _attachedSlots.count();
  qDebug() << " number of synced Slaves:" << slaveCount;
  qDebug() << "number of Classes cached:" << _extendedMetaObjects.count();
}

void SignalProxy::dumpSyncMap(SyncableObject *object) {
  const QMetaObject *meta = object->metaObject();
  ExtendedMetaObject *eMeta = extendedMetaObject(object);
  qDebug() << "SignalProxy: SyncMap for Class" << meta->className();

  QHash<QByteArray, int> syncMap_ = eMeta->syncMap();
  QHash<QByteArray, int>::const_iterator iter = syncMap_.constBegin();
  while(iter != syncMap_.constEnd()) {
    qDebug() << qPrintable(QString("%1 --> %2 %3").arg(QString(iter.key()), 40).arg(iter.value()).arg(QString(meta->method(iter.value()).signature())));
    iter++;
  }
}

void SignalProxy::updateSecureState() {
  bool wasSecure = _secure;

  _secure = !_peers.isEmpty();
  PeerHash::const_iterator peerIter;
  for(peerIter = _peers.constBegin(); peerIter != _peers.constEnd(); peerIter++) {
    _secure &= (*peerIter)->isSecure();
  }

  if(wasSecure != _secure)
    emit secureStateChanged(_secure);
}


// ==================================================
//  ExtendedMetaObject
// ==================================================
SignalProxy::ExtendedMetaObject::ExtendedMetaObject(const QMetaObject *meta)
  : _meta(meta),
    _updatedRemotelyId(-1)
{
}

const QList<int> &SignalProxy::ExtendedMetaObject::argTypes(int methodId) {
  if(!_argTypes.contains(methodId)) {
    QList<QByteArray> paramTypes = _meta->method(methodId).parameterTypes();
    QList<int> argTypes;
    for(int i = 0; i < paramTypes.count(); i++) {
      argTypes.append(QMetaType::type(paramTypes[i]));
    }
    _argTypes[methodId] = argTypes;
  }
  return _argTypes[methodId];
}

const int &SignalProxy::ExtendedMetaObject::returnType(int methodId) {
  if(!_returnType.contains(methodId)) {
    _returnType[methodId] = QMetaType::type(_meta->method(methodId).typeName());
  }
  return _returnType[methodId];
}

const int &SignalProxy::ExtendedMetaObject::minArgCount(int methodId) {
  if(!_minArgCount.contains(methodId)) {
    QString signature(_meta->method(methodId).signature());
    _minArgCount[methodId] = _meta->method(methodId).parameterTypes().count() - signature.count("=");
  }
  return _minArgCount[methodId];
}

const QByteArray &SignalProxy::ExtendedMetaObject::methodName(int methodId) {
  if(!_methodNames.contains(methodId)) {
    _methodNames[methodId] = methodName(_meta->method(methodId));
  }
  return _methodNames[methodId];
}

const QHash<QByteArray, int> &SignalProxy::ExtendedMetaObject::syncMap() {
  if(_syncMap.isEmpty()) {
    QHash<QByteArray, int> syncMap;

    QList<int> slotIndexes;
    for(int i = 0; i < _meta->methodCount(); i++) {
      if(_meta->method(i).methodType() == QMetaMethod::Slot)
	slotIndexes << i;
    }

    // we're faking sync pairs for sync replies
    // --> we deliver to every slot starting with "receive"
    QByteArray slotSignature;
    QList<int>::iterator slotIter = slotIndexes.begin();
    while(slotIter != slotIndexes.end()) {
      slotSignature = QByteArray(_meta->method(*slotIter).signature());
      if(slotSignature.startsWith("receive")) {
	syncMap[slotSignature] = *slotIter;
	slotIter = slotIndexes.erase(slotIter);
      } else {
	slotIter++;
      }
    }

    // find the proper sig -> slot matches
    QMetaMethod signal, slot;
    int matchIdx;
    for(int signalIdx = 0; signalIdx < _meta->methodCount(); signalIdx++) {
      signal = _meta->method(signalIdx);
      if(signal.methodType() != QMetaMethod::Signal)
	continue;

      matchIdx = -1;
      foreach(int slotIdx, slotIndexes) {
	slot = _meta->method(slotIdx);
	if(methodsMatch(signal, slot)) {
	  matchIdx = slotIdx;
	  break;
	}
      }
      if(matchIdx != -1) {
	slotIndexes.removeAll(matchIdx);
	syncMap[QByteArray(signal.signature())] = matchIdx;
      }
    }
    _syncMap = syncMap;
  }
  return _syncMap;
}

const QHash<int, int> &SignalProxy::ExtendedMetaObject::receiveMap() {
  if(_receiveMap.isEmpty()) {
    QHash<int, int> receiveMap;

    QMetaMethod requestSlot;
    QByteArray returnTypeName;
    QByteArray signature;
    QByteArray methodName;
    QByteArray params;
    int paramsPos;
    int receiverId;
    const int methodCount = _meta->methodCount();
    for(int i = 0; i < methodCount; i++) {
      requestSlot = _meta->method(i);
      if(requestSlot.methodType() != QMetaMethod::Slot)
	continue;

      returnTypeName = requestSlot.typeName();
      if(QMetaType::Void == (QMetaType::Type)returnType(i))
	continue;

      signature = QByteArray(requestSlot.signature());
      if(!signature.startsWith("request"))
	continue;

      paramsPos = signature.indexOf('(');
      if(paramsPos == -1)
	continue;

      methodName = signature.left(paramsPos);
      params = signature.mid(paramsPos);

      methodName = methodName.replace("request", "receive");
      params = params.left(params.count() - 1) + ", " + returnTypeName + ")";

      signature = QMetaObject::normalizedSignature(methodName + params);
      receiverId = _meta->indexOfSlot(signature);

      if(receiverId == -1) {
	signature = QMetaObject::normalizedSignature(methodName + "(" + returnTypeName + ")");
	receiverId = _meta->indexOfSlot(signature);
      }

      if(receiverId != -1)
	receiveMap[i] = receiverId;
    }
    _receiveMap = receiveMap;
  }
  return _receiveMap;
}

int SignalProxy::ExtendedMetaObject::updatedRemotelyId() {
  if(_updatedRemotelyId == -1) {
    _updatedRemotelyId = _meta->indexOfSignal("updatedRemotely()");
  }
  return _updatedRemotelyId;
}

QByteArray SignalProxy::ExtendedMetaObject::methodName(const QMetaMethod &method) {
  QByteArray sig(method.signature());
  return sig.left(sig.indexOf("("));
}

bool SignalProxy::ExtendedMetaObject::methodsMatch(const QMetaMethod &signal, const QMetaMethod &slot) {
  // if we don't even have the same basename it's a sure NO
  QString baseName = methodBaseName(signal);
  if(baseName != methodBaseName(slot))
    return false;

  // are the signatures compatible?
  if(!QObject::staticMetaObject.checkConnectArgs(signal.signature(), slot.signature()))
    return false;

  // we take an educated guess if the signals and slots match
  QString signalsuffix = methodName(signal);
  QString slotprefix = methodName(slot);
  if(!baseName.isEmpty()) {
    signalsuffix = signalsuffix.mid(baseName.count()).toLower();
    slotprefix = slotprefix.left(slotprefix.count() - baseName.count()).toLower();
  }

  uint sizediff = qAbs(slotprefix.size() - signalsuffix.size());
  int ratio = editingDistance(slotprefix, signalsuffix) - sizediff;
  return (ratio < 2);
}

QString SignalProxy::ExtendedMetaObject::methodBaseName(const QMetaMethod &method) {
  QString methodname = QString(method.signature()).section("(", 0, 0);

  // determine where we have to chop:
  int upperCharPos;
  if(method.methodType() == QMetaMethod::Slot) {
    // we take evertyhing from the first uppercase char if it's slot
    upperCharPos = methodname.indexOf(QRegExp("[A-Z]"));
    if(upperCharPos == -1)
      return QString();
    methodname = methodname.mid(upperCharPos);
  } else {
    // and if it's a signal we discard everything from the last uppercase char
    upperCharPos = methodname.lastIndexOf(QRegExp("[A-Z]"));
    if(upperCharPos == -1)
      return QString();
    methodname = methodname.left(upperCharPos);
  }

  methodname[0] = methodname[0].toUpper();

  return methodname;
}


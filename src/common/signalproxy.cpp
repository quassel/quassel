/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtNetwork module of the Qt eXTension library
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of th Common Public License, version 1.0, as published by
 ** IBM.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL along with this file.
 ** See the LICENSE file and the cpl1.0.txt file included with the source
 ** distribution for more information. If you did not receive a copy of the
 ** license, contact the Qxt Foundation.
 **
 ** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
 **
 ****************************************************************************/

#include "signalproxy.h"
#include <QObject>
#include <QIODevice>
#include <QAbstractSocket>
#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QSet>
#include <QDebug>
#include <QMetaMethod>

class ClassIntrospector: public QObject {
// This class MANUALLY implements the necessary parts of QObject.
// Do NOT add the Q_OBJECT macro. As this class isn't intended
// for direct use, it doesn't offer any sort of useful meta-object.
public:
  ClassIntrospector(SignalProxy* parent, QObject* source);
  int qt_metacall(QMetaObject::Call _c, int _id, void **_a);

  void attachSignal(int methodId, const QByteArray &func);
  
private:
  SignalProxy* proxy;
  QObject* caller;
  QMultiHash<int, QByteArray> rpcFunction;
};

ClassIntrospector::ClassIntrospector(SignalProxy* parent, QObject* source)
  : QObject(parent),
    proxy(parent),
    caller(source)
{
  QObject::connect(source, SIGNAL(destroyed()), parent, SLOT(detachSender()));
}

int ClassIntrospector::qt_metacall(QMetaObject::Call _c, int _id, void **_a) {
  _id = QObject::qt_metacall(_c, _id, _a);
  if(_id < 0)
    return _id;
  if(_c == QMetaObject::InvokeMetaMethod) {
    if(rpcFunction.contains(_id)) {
      const QList<int> &argTypes = proxy->argTypes(caller, _id);
      QVariantList params;
      int n = argTypes.size();
      for(int i=0; i<n; i++)
	params.append(QVariant(argTypes[i], _a[i+1]));
      QMultiHash<int, QByteArray>::const_iterator funcIter = rpcFunction.constFind(_id);
      while(funcIter != rpcFunction.constEnd() && funcIter.key() == _id) {
	proxy->call(funcIter.value(), params);
	funcIter++;
      }
    }
    _id -= 1;
  }
  return _id;
}

void ClassIntrospector::attachSignal(int methodId, const QByteArray &func) {
  // we ride without safetybelts here... all checking for valid method etc pp has to be done by the caller
  // all connected methodIds are offset by the standard methodCount of QObject
  if(!rpcFunction.contains(methodId))
    QMetaObject::connect(caller, methodId, this, QObject::staticMetaObject.methodCount() + methodId);

  QByteArray fn;
  if(!func.isEmpty()) {
    fn = QMetaObject::normalizedSignature(func);
  } else {
    fn = QByteArray("2") + caller->metaObject()->method(methodId).signature();
  }
  rpcFunction.insert(methodId, fn);
}
// ====================
// END INTROSPECTOR
// ====================


// ====================
//  SignalProxy
// ====================
SignalProxy::SignalProxy(QObject* parent)
  : QObject(parent),
    _rpcType(Peer),
    _maxClients(-1)
{
}

SignalProxy::SignalProxy(RPCTypes type, QObject* parent)
  : QObject(parent),
    _rpcType(type),
    _maxClients(-1)
{
}

SignalProxy::SignalProxy(RPCTypes type, QIODevice* device, QObject* parent)
  : QObject(parent),
    _rpcType(type),
    _maxClients(-1)
{
  addPeer(device);
}  

SignalProxy::~SignalProxy() {
  QList<QObject*> senders = _specHash.keys();
  foreach(QObject* sender, senders)
    detachObject(sender);
}

void SignalProxy::setRPCType(RPCTypes type) {
  foreach(QIODevice* peer, _peerByteCount.keys()) {
    if(peer->isOpen()) {
      qWarning() << "SignalProxy: Cannot change RPC types while connected";
      return;
    }
  }
  _rpcType = type;
}


SignalProxy::RPCTypes SignalProxy::rpcType() const {
  return (RPCTypes)(_rpcType);
}

bool SignalProxy::maxPeersReached() {
  if(_peerByteCount.empty())
    return false;
  if(rpcType() != Server)
    return true;
  if(_maxClients == -1)
    return false;

  return (_maxClients <= _peerByteCount.count());
}

bool SignalProxy::addPeer(QIODevice* iodev) {
  if(!iodev)
    return false;
  
  if(_peerByteCount.contains(iodev))
    return true;

  if(maxPeersReached()) {
    qWarning("SignalProxy: max peercount reached");
    return false;
  }
     
  if(!iodev->isOpen())
    qWarning("SignalProxy::the device you passed is not open!");
  
  connect(iodev, SIGNAL(disconnected()), this, SLOT(removePeerBySender()));
  connect(iodev, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
  
  QAbstractSocket* sock  = qobject_cast<QAbstractSocket*>(iodev);
  if(sock) {
    connect(sock, SIGNAL(disconnected()), this, SLOT(removePeerBySender()));
  }

  _peerByteCount[iodev] = 0;

  if(_peerByteCount.count() == 1)
    emit connected();
  
  return true;
}

void SignalProxy::removePeerBySender() {
  // OK we're brutal here... but since it's a private slot we know what we've got connected to it...
  QIODevice *ioDev = (QIODevice *)(sender());
  removePeer(ioDev);
}

void SignalProxy::removePeer(QIODevice* iodev) {
  if(_peerByteCount.isEmpty()) {
    qWarning() << "No Peers in use!";
    return;
  }
     
  if(_rpcType == Server && !iodev) {
    // disconnect all
    QList<QIODevice *> peers = _peerByteCount.keys();
    foreach(QIODevice *peer, peers)
      removePeer(peer);
  }

  if(_rpcType != Server && !iodev)
    iodev = _peerByteCount.keys().first();
  
  Q_ASSERT(iodev);
     
  if(!_peerByteCount.contains(iodev)) {
    qWarning() << "SignalProxy: unknown QIODevice" << iodev;
    return;
  }
     
  // take a last gasp
  while(true) {
    QVariant var;
    if(readDataFromDevice(iodev, _peerByteCount[iodev], var))
      receivePeerSignal(var);
    else
      break;
  }
  _peerByteCount.remove(iodev);

  disconnect(iodev, 0, this, 0);
  emit peerRemoved(iodev);

  if(_peerByteCount.isEmpty())
    emit disconnected();
}

void SignalProxy::setArgTypes(QObject* obj, int methodId) {
  QList<QByteArray> p = obj->metaObject()->method(methodId).parameterTypes();
  QList<int> argTypes;
  int ct = p.count();
  for(int i=0; i<ct; i++)
    argTypes.append(QMetaType::type(p.value(i)));

  const QByteArray &className(obj->metaObject()->className());
  Q_ASSERT(_classInfo.contains(className));
  Q_ASSERT(!_classInfo[className]->argTypes.contains(methodId));
  _classInfo[className]->argTypes[methodId] = argTypes;
}

const QList<int> &SignalProxy::argTypes(QObject *obj, int methodId) {
  const QByteArray &className(obj->metaObject()->className());
  Q_ASSERT(_classInfo.contains(className));
  if(!_classInfo[className]->argTypes.contains(methodId))
    setArgTypes(obj, methodId);
  return _classInfo[className]->argTypes[methodId];
}

void SignalProxy::setMethodName(QObject *obj, int methodId) {
  const QByteArray &className(obj->metaObject()->className());
  QByteArray method = obj->metaObject()->method(methodId).signature();
  method = method.left(method.indexOf('('));

  Q_ASSERT(_classInfo.contains(className));
  Q_ASSERT(!_classInfo[className]->methodNames.contains(methodId));
  _classInfo[className]->methodNames[methodId] = method;
}

const QByteArray &SignalProxy::methodName(QObject *obj, int methodId) {
  QByteArray className(obj->metaObject()->className());
  Q_ASSERT(_classInfo.contains(className));
  if(!_classInfo[className]->methodNames.contains(methodId))
    setMethodName(obj, methodId);
  return _classInfo[className]->methodNames[methodId];
}


void SignalProxy::createClassInfo(QObject *obj) {
  QByteArray className(obj->metaObject()->className());
  if(!_classInfo.contains(className))
    _classInfo[className] = new ClassInfo();
}

bool SignalProxy::attachSignal(QObject* sender, const char* signal, const QByteArray& rpcFunction) {
  const QMetaObject* meta = sender->metaObject();
  QByteArray sig(meta->normalizedSignature(signal).mid(1));
  int methodId = meta->indexOfMethod(sig.constData());
  if(methodId == -1 || meta->method(methodId).methodType() != QMetaMethod::Signal) {
    qWarning() << "SignalProxy::attachSignal: No such signal " << signal;
    return false;
  }

  createClassInfo(sender);

  ClassIntrospector* spec;
  if(_specHash.contains(sender))
    spec = _specHash[sender];
  else
    spec = _specHash[sender] = new ClassIntrospector(this, sender);

  spec->attachSignal(methodId, rpcFunction);

  return true;
}


bool SignalProxy::attachSlot(const QByteArray& rpcFunction, QObject* recv, const char* slot) {
  const QMetaObject* meta = recv->metaObject();
  int methodId = meta->indexOfMethod(meta->normalizedSignature(slot).mid(1));
  if(methodId == -1 || meta->method(methodId).methodType() == QMetaMethod::Method) {
    qWarning() << "SignalProxy::attachSlot: No such slot " << slot;
    return false;
  }

  createClassInfo(recv);
  
  QByteArray funcName = QMetaObject::normalizedSignature(rpcFunction.constData());
  _attachedSlots.insert(funcName, qMakePair(recv, methodId));

  QObject::disconnect(recv, SIGNAL(destroyed()), this, SLOT(detachSender()));
  QObject::connect(recv, SIGNAL(destroyed()), this, SLOT(detachSender()));
  return true;
}


void SignalProxy::detachSender() {
  // this is a slot so we can bypass the QueuedConnection
  _detachSignals(sender());
  _detachSlots(sender());
}

// detachObject/Signals/Slots() can be called as a result of an incomming call
// this might destroy our the iterator used for delivery
// thus we wrap the actual disconnection by using QueuedConnections
void SignalProxy::detachObject(QObject* obj) {
  detachSignals(obj);
  detachSlots(obj);
}

void SignalProxy::detachSignals(QObject* sender) {
  QMetaObject::invokeMethod(this, "_detachSignals",
			    Qt::QueuedConnection,
			    Q_ARG(QObject*, sender));
}

void SignalProxy::_detachSignals(QObject* sender) {
  if(!_specHash.contains(sender))
    return;
  _specHash.take(sender)->deleteLater();
}

void SignalProxy::detachSlots(QObject* receiver) {
  QMetaObject::invokeMethod(this, "_detachSlots",
			    Qt::QueuedConnection,
			    Q_ARG(QObject*, receiver));
}

void SignalProxy::_detachSlots(QObject* receiver) {
  SlotHash::iterator slotIter = _attachedSlots.begin();
  while(slotIter != _attachedSlots.end()) {
    if(slotIter.value().first == receiver) {
      slotIter = _attachedSlots.erase(slotIter);
    } else
      slotIter++;
  }
}


void SignalProxy::call(const char*  signal , QVariant p1, QVariant p2, QVariant p3, QVariant p4, QVariant p5, QVariant p6, QVariant p7, QVariant p8, QVariant p9) {
  QByteArray sig=QMetaObject::normalizedSignature(signal);
  QVariantList params;
  params << p1 << p2 << p3 << p4 << p5
	 << p6 << p7 << p8 << p9;
  call(sig, params);
}

void SignalProxy::call(const QByteArray &funcName, const QVariantList &params) {
  QVariantList packedFunc;
  packedFunc << funcName;
  packedFunc << params;
  foreach(QIODevice* dev, _peerByteCount.keys())
    writeDataToDevice(dev, QVariant(packedFunc));
}

void SignalProxy::receivePeerSignal(const QVariant &packedFunc) {
  QVariantList params(packedFunc.toList());
  QByteArray funcName = params.takeFirst().toByteArray();
  int numParams, methodId;
  QObject* receiver;

  SlotHash::const_iterator slot = _attachedSlots.constFind(funcName);
  while(slot != _attachedSlots.constEnd() && slot.key() == funcName) {
    receiver = (*slot).first;
    methodId = (*slot).second;
    numParams = argTypes(receiver, methodId).count();
    QGenericArgument args[9];
    for(int i = 0; i < numParams; i++)
      args[i] = QGenericArgument(params[i].typeName(), params[i].constData());
    if(!QMetaObject::invokeMethod(receiver, methodName(receiver, methodId),
				  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8])) {
      qWarning("SignalProxy::receivePeerSignal: invokeMethod for \"%s\" failed ", methodName(receiver, methodId).constData());
    }
    slot++;
  }
}

void SignalProxy::dataAvailable() {
  QIODevice* ioDev = qobject_cast<QIODevice* >(sender());
  Q_ASSERT(ioDev);
  if(!_peerByteCount.contains(ioDev)) {
    qWarning() << "SignalProxy: Unrecognized client object connected to dataAvailable";
    return;
  } else {
    QVariant var;
    while(readDataFromDevice(ioDev, _peerByteCount[ioDev], var))
      receivePeerSignal(var);
  }
}

void SignalProxy::writeDataToDevice(QIODevice *dev, const QVariant &item) {
  QAbstractSocket* sock  = qobject_cast<QAbstractSocket*>(dev);
  if(!dev->isOpen() || (sock && sock->state()!=QAbstractSocket::ConnectedState)) {
    qWarning("can't call on a closed device");
    return;
  }
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_2);
  out << (quint32)0 << item;
  out.device()->seek(0);
  out << (quint32)(block.size() - sizeof(quint32));
  dev->write(block);
}

bool SignalProxy::readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item) {
  QDataStream in(dev);
  in.setVersion(QDataStream::Qt_4_2);

  if(blockSize == 0) {
    if(dev->bytesAvailable() < (int)sizeof(quint32)) return false;
    in >> blockSize;
  }

  if(dev->bytesAvailable() < blockSize)
    return false;
  in >> item;
  blockSize = 0;
  return true;
}



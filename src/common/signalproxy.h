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

#ifndef _SIGNALPROXY_H_
#define _SIGNALPROXY_H_

#include <QObject>
#include <QList>
#include <QHash>
#include <QVariant>
#include <QPair>
#include <QString>
#include <QByteArray>

class ClassIntrospector;

class SignalProxy : public QObject {
  Q_OBJECT

public:
  enum RPCTypes {
    Server,
    Client,
    Peer
  };

  SignalProxy(QObject* parent);
  SignalProxy(RPCTypes type, QObject* parent);
  SignalProxy(RPCTypes type, QIODevice* device, QObject* parent);
  virtual ~SignalProxy();

  void setRPCType(RPCTypes type);
  RPCTypes rpcType() const;

  bool maxPeersReached();
  
  bool addPeer(QIODevice *iodev);
  void removePeer(QIODevice *iodev = 0);

  bool attachSignal(QObject* sender, const char* signal, const QByteArray& rpcFunction = QByteArray());
  bool attachSlot(const QByteArray& rpcFunction, QObject* recv, const char* slot);

  void detachObject(QObject *obj);
  void detachSignals(QObject *sender);
  void detachSlots(QObject *receiver);
  
  void call(const char *signal , QVariant p1, QVariant p2, QVariant p3, QVariant p4,
	    QVariant p5, QVariant p6, QVariant p7, QVariant p8, QVariant p9);
  void call(const QByteArray &funcName, const QVariantList &params);

  static void writeDataToDevice(QIODevice *dev, const QVariant &item);
  static bool readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item);
  
  const QList<int> &argTypes(QObject* obj, int methodId);
  const QByteArray &methodName(QObject* obj, int methodId);
  
  typedef QHash<int, QList<int> > ArgHash;
  typedef QHash<int, QByteArray> MethodNameHash;
  struct ClassInfo {
    ArgHash argTypes;
    MethodNameHash methodNames;
    // QHash<int, int> syncMap
  };
  
private slots:
  void dataAvailable();
  void detachSender();
  void removePeerBySender();

signals:
  void peerRemoved(QIODevice* obj);
  void connected();
  void disconnected();
  
private:
  void createClassInfo(QObject *obj);
  void setArgTypes(QObject* obj, int methodId);
  void setMethodName(QObject* obj, int methodId);
  
  void receivePeerSignal(const QVariant &packedFunc);

  void _detachSignals(QObject *sender);
  void _detachSlots(QObject *receiver);

  // containg a list of argtypes for fast access
  QHash<QByteArray, ClassInfo*> _classInfo;
  
  // we use one introSpector per QObject
  QHash<QObject*, ClassIntrospector*> _specHash;

  // RPC function -> (object, slot ID)
  typedef QPair<QObject*, int> MethodId;
  typedef QMultiHash<QByteArray, MethodId> SlotHash;
  SlotHash _attachedSlots;
  

  // Hash of used QIODevices
  QHash<QIODevice*, quint32> _peerByteCount;

  int _rpcType;
  int _maxClients;
};




#endif

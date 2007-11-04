/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
 ***************************************************************************
 *   SignalProxy has been inspired by QxtRPCPeer, part of libqxt,          *
 *   the Qt eXTension Library <http://www.libqxt.org>. We would like to    *
 *   thank Arvid "aep" Picciani and Adam "ahigerd" Higerd for providing    *
 *   QxtRPCPeer, valuable input and the genius idea to (ab)use Qt's        *
 *   Meta Object System for transmitting signals over the network.         *
 *                                                                         *
 *   To make contribution back into libqxt possible, redistribution and    *
 *   modification of this file is additionally allowed under the terms of  *
 *   the Common Public License, version 1.0, as published by IBM.          *
 ***************************************************************************/

#ifndef _SIGNALPROXY_H_
#define _SIGNALPROXY_H_

#include <QList>
#include <QHash>
#include <QVariant>
#include <QPair>
#include <QString>
#include <QByteArray>

class SignalRelay;

class SignalProxy : public QObject {
  Q_OBJECT

public:
  enum ProxyMode {
    Server,
    Client
  };

  SignalProxy(QObject* parent);
  SignalProxy(ProxyMode mode, QObject* parent);
  SignalProxy(ProxyMode mode, QIODevice* device, QObject* parent);
  virtual ~SignalProxy();

  void setProxyMode(ProxyMode mode);
  ProxyMode proxyMode() const;

  bool addPeer(QIODevice *iodev);
  void removePeer(QIODevice *iodev = 0);

  bool attachSignal(QObject* sender, const char* signal, const QByteArray& sigName = QByteArray());
  bool attachSlot(const QByteArray& sigName, QObject* recv, const char* slot);

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

  void dumpProxyStats();
  
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

  // we use one SignalRelay per QObject
  QHash<QObject*, SignalRelay *> _relayHash;

  // RPC function -> (object, slot ID)
  typedef QPair<QObject*, int> MethodId;
  typedef QMultiHash<QByteArray, MethodId> SlotHash;
  SlotHash _attachedSlots;
  

  // Hash of used QIODevices
  QHash<QIODevice*, quint32> _peerByteCount;

  ProxyMode _proxyMode;
};




#endif

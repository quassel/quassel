/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _SIGNALPROXY_H_
#define _SIGNALPROXY_H_

#include <QList>
#include <QHash>
#include <QVariant>
#include <QVariantMap>
#include <QPair>
#include <QString>
#include <QByteArray>

class SignalRelay;
class QMetaObject;

class SignalProxy : public QObject {
  Q_OBJECT

public:
  enum ProxyMode {
    Server,
    Client
  };

  enum RequestType {
    Sync = 0,
    InitRequest,
    InitData
  };

  SignalProxy(QObject *parent);
  SignalProxy(ProxyMode mode, QObject *parent);
  SignalProxy(ProxyMode mode, QIODevice *device, QObject *parent);
  virtual ~SignalProxy();

  void setProxyMode(ProxyMode mode);
  ProxyMode proxyMode() const;

  bool addPeer(QIODevice *iodev);
  void removePeer(QIODevice *iodev = 0);

  bool attachSignal(QObject *sender, const char *signal, const QByteArray& sigName = QByteArray());
  bool attachSlot(const QByteArray& sigName, QObject *recv, const char *slot);

  void synchronize(QObject *obj);
  void synchronizeAsMaster(QObject *obj);
  void synchronizeAsSlave(QObject *obj);

  void setInitialized(QObject *obj);
  bool initialized(QObject *obj);
  void requestInit(QObject *obj);
  
  void detachObject(QObject *obj);
  void detachSignals(QObject *sender);
  void detachSlots(QObject *receiver);
  
  static void writeDataToDevice(QIODevice *dev, const QVariant &item);
  static bool readDataFromDevice(QIODevice *dev, quint32 &blockSize, QVariant &item);

  static QString methodBaseName(const QMetaMethod &method);
  
  const QList<int> &argTypes(QObject *obj, int methodId);
  const QByteArray &methodName(QObject *obj, int methodId);
  const QHash<int, int> &syncMap(QObject *obj);

  typedef QHash<int, QList<int> > ArgHash;
  typedef QHash<int, QByteArray> MethodNameHash;
  struct ClassInfo {
    ArgHash argTypes;
    MethodNameHash methodNames;
    QHash<int, int> syncMap;
  };

  void dumpProxyStats();
  
private slots:
  void dataAvailable();
  void detachSender();
  void removePeerBySender();
  void objectRenamed(QString oldname, QString newname);
  void objectRenamed(QByteArray classname, QString oldname, QString newname);

signals:
  void peerRemoved(QIODevice *obj);
  void connected();
  void disconnected();
  
private:
  void initServer();
  void initClient();
  
  void createClassInfo(QObject *obj);
  void setArgTypes(QObject *obj, int methodId);
  void setMethodName(QObject *obj, int methodId);
  void setSyncMap(QObject *obj);

  bool methodsMatch(const QMetaMethod &signal, const QMetaMethod &slot) const;

  void dispatchSignal(QIODevice *receiver, const QVariant &identifier, const QVariantList &params);
  void dispatchSignal(const QVariant &identifier, const QVariantList &params);
  
  void receivePeerSignal(QIODevice *sender, const QVariant &packedFunc);
  void handleSync(QVariantList params);
  void handleInitRequest(QIODevice *sender, const QVariantList &params);
  void handleInitData(QIODevice *sender, const QVariantList &params);
  void handleSignal(const QByteArray &funcName, const QVariantList &params);

  bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params);

  QVariantMap initData(QObject *obj) const;
  void setInitData(QObject *obj, const QVariantMap &properties);
  bool setInitValue(QObject *obj, const QString &property, const QVariant &value);

  void _detachSignals(QObject *sender);
  void _detachSlots(QObject *receiver);

  // containg a list of argtypes for fast access
  QHash<const QMetaObject *, ClassInfo*> _classInfo;

  // we use one SignalRelay per QObject
  QHash<QObject*, SignalRelay *> _relayHash;

  // RPC function -> (object, slot ID)
  typedef QPair<QObject*, int> MethodId;
  typedef QMultiHash<QByteArray, MethodId> SlotHash;
  SlotHash _attachedSlots;

  // slaves for sync
  typedef QHash<QString, QObject *> ObjectId;
  QHash<QByteArray, ObjectId> _syncSlave;

  // Hash of used QIODevices
  QHash<QIODevice*, quint32> _peerByteCount;

  ProxyMode _proxyMode;

  friend class SignalRelay;
};

#endif

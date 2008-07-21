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

#ifndef _SIGNALPROXY_H_
#define _SIGNALPROXY_H_

#include <QList>
#include <QHash>
#include <QVariant>
#include <QVariantMap>
#include <QPair>
#include <QString>
#include <QByteArray>
#include <QTimer>

class SignalRelay;
class SyncableObject;
struct QMetaObject;

class SignalProxy : public QObject {
  Q_OBJECT

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

  SignalProxy(QObject *parent);
  SignalProxy(ProxyMode mode, QObject *parent);
  SignalProxy(ProxyMode mode, QIODevice *device, QObject *parent);
  virtual ~SignalProxy();

  void setProxyMode(ProxyMode mode);
  inline ProxyMode proxyMode() const { return _proxyMode; }

  bool addPeer(QIODevice *iodev);
  void removePeer(QIODevice *iodev = 0);

  bool attachSignal(QObject *sender, const char *signal, const QByteArray& sigName = QByteArray());
  bool attachSlot(const QByteArray& sigName, QObject *recv, const char *slot);

  void synchronize(SyncableObject *obj);

//   void setInitialized(SyncableObject *obj);
//   bool isInitialized(SyncableObject *obj) const;
  void requestInit(SyncableObject *obj);

  void detachObject(QObject *obj);
  void detachSignals(QObject *sender);
  void detachSlots(QObject *receiver);
  void stopSync(SyncableObject *obj);

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

  static QString methodBaseName(const QMetaMethod &method);

  const QList<int> &argTypes(QObject *obj, int methodId);
  const int &returnType(QObject *obj, int methodId);
  const int &minArgCount(QObject *obj, int methodId);
  const QByteArray &methodName(QObject *obj, int methodId);
  const QHash<QByteArray, int> &syncMap(SyncableObject *obj);
  const QHash<int, int> &receiveMap(SyncableObject *obj);
  int updatedRemotelyId(SyncableObject *obj);

  typedef QHash<int, QList<int> > ArgHash;
  typedef QHash<int, QByteArray> MethodNameHash;
  struct ClassInfo {
    ArgHash argTypes;
    QHash<int, int> returnType;
    QHash<int, int> minArgCount;
    MethodNameHash methodNames;
    int updatedRemotelyId; // id of the updatedRemotely() signal - makes things faster
    QHash<QByteArray, int> syncMap;
    QHash<int, int> receiveMap;
  };

  void dumpProxyStats();
  
private slots:
  void dataAvailable();
  void detachSender();
  void removePeerBySender();
  void objectRenamed(const QString &newname, const QString &oldname);
  void objectRenamed(const QByteArray &classname, const QString &newname, const QString &oldname);
  void sendHeartBeat();
  void receiveHeartBeat(QIODevice *dev, const QVariantList &params);
  void receiveHeartBeatReply(QIODevice *dev, const QVariantList &params);
  
signals:
  void peerRemoved(QIODevice *dev);
  void connected();
  void disconnected();
  void objectInitialized(SyncableObject *);
  void lagUpdated(int lag);
  
private:
  void init();
  void initServer();
  void initClient();
  
  const QMetaObject *metaObject(QObject *obj);
  void createClassInfo(QObject *obj);
  void setArgTypes(QObject *obj, int methodId);
  void setReturnType(QObject *obj, int methodId);
  void setMinArgCount(QObject *obj, int methodId);
  void setMethodName(QObject *obj, int methodId);
  void setSyncMap(SyncableObject *obj);
  void setReceiveMap(SyncableObject *obj);
  void setUpdatedRemotelyId(SyncableObject *obj);

  bool methodsMatch(const QMetaMethod &signal, const QMetaMethod &slot) const;

  void dispatchSignal(QIODevice *receiver, const RequestType &requestType, const QVariantList &params);
  void dispatchSignal(const RequestType &requestType, const QVariantList &params);
  
  void receivePeerSignal(QIODevice *sender, const QVariant &packedFunc);
  void handleSync(QIODevice *sender, QVariantList params);
  void handleInitRequest(QIODevice *sender, const QVariantList &params);
  void handleInitData(QIODevice *sender, const QVariantList &params);
  void handleSignal(const QByteArray &funcName, const QVariantList &params);

  bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params, QVariant &returnValue);
  bool invokeSlot(QObject *receiver, int methodId, const QVariantList &params = QVariantList());

  QVariantMap initData(SyncableObject *obj) const;
  void setInitData(SyncableObject *obj, const QVariantMap &properties);

  void updateLag(QIODevice *dev, int lag);

public:
  void dumpSyncMap(SyncableObject *object);
  inline int peerCount() const { return _peers.size(); }
  
private:
  // Hash of used QIODevices
  struct peerInfo {
    quint32 byteCount;
    bool usesCompression;
    int sentHeartBeats;
    int lag;
    peerInfo() : byteCount(0), usesCompression(false), sentHeartBeats(0) {}
  };
  //QHash<QIODevice*, peerInfo> _peerByteCount;
  QHash<QIODevice*, peerInfo> _peers;

  // containg a list of argtypes for fast access
  QHash<const QMetaObject *, ClassInfo*> _classInfo;

  // we use one SignalRelay per QObject
  QHash<QObject*, SignalRelay *> _relayHash;

  // RPC function -> (object, slot ID)
  typedef QPair<QObject*, int> MethodId;
  typedef QMultiHash<QByteArray, MethodId> SlotHash;
  SlotHash _attachedSlots;

  // slaves for sync
  typedef QHash<QString, SyncableObject *> ObjectId;
  QHash<QByteArray, ObjectId> _syncSlave;


  ProxyMode _proxyMode;
  QTimer _heartBeatTimer;
  
  friend class SignalRelay;
};

#endif

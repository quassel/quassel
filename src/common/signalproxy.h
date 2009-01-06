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

class SignalProxy : public QObject {
  Q_OBJECT

  class AbstractPeer;
  class IODevicePeer;
  class SignalProxyPeer;

  class Relay;
  class SignalRelay;
  class SyncRelay;

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

  bool addPeer(QIODevice *iodev);
  bool addPeer(SignalProxy *proxy);
  void removePeer(QObject *peer);
  void removeAllPeers();

  bool attachSignal(QObject *sender, const char *signal, const QByteArray& sigName = QByteArray());
  bool attachSlot(const QByteArray& sigName, QObject *recv, const char *slot);

  void synchronize(SyncableObject *obj);

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
  ExtendedMetaObject *createExtendedMetaObject(const QMetaObject *meta);
  inline ExtendedMetaObject *extendedMetaObject(const QObject *obj) const { return extendedMetaObject(metaObject(obj)); }
  inline ExtendedMetaObject *createExtendedMetaObject(const QObject *obj) { return createExtendedMetaObject(metaObject(obj)); }

  bool isSecure() const { return _secure; }
  void dumpProxyStats();

public slots:
  void detachObject(QObject *obj);
  void detachSignals(QObject *sender);
  void detachSlots(QObject *receiver);
  void stopSync(QObject *obj);

protected:
  void customEvent(QEvent *event);

private slots:
  void dataAvailable();
  void removePeerBySender();
  void objectRenamed(const QString &newname, const QString &oldname);
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
  // one SyncRelay per class
  QHash<const QMetaObject *, SyncRelay *> _syncRelays;

  // RPC function -> (object, slot ID)
  typedef QPair<QObject*, int> MethodId;
  typedef QMultiHash<QByteArray, MethodId> SlotHash;
  SlotHash _attachedSlots;

  // slaves for sync
  typedef QHash<QString, SyncableObject *> ObjectId;
  QHash<QByteArray, ObjectId> _syncSlave;


  ProxyMode _proxyMode;
  QTimer _heartBeatTimer;

  bool _secure; // determines if all connections are in a secured state (using ssl or internal connections)

  friend class SignalRelay;
};


// ==================================================
//  ExtendedMetaObject
// ==================================================
class SignalProxy::ExtendedMetaObject {
public:
  ExtendedMetaObject(const QMetaObject *meta);

  const QList<int> &argTypes(int methodId);
  const int &returnType(int methodId);
  const int &minArgCount(int methodId);
  const QByteArray &methodName(int methodId);
  const QHash<QByteArray, int> &syncMap();
  const QHash<int, int> &receiveMap();
  int updatedRemotelyId();

  const QMetaObject *metaObject() const { return _meta; }

  static QByteArray methodName(const QMetaMethod &method);
  static bool methodsMatch(const QMetaMethod &signal, const QMetaMethod &slot);
  static QString methodBaseName(const QMetaMethod &method);

private:
  typedef QHash<int, QList<int> > ArgHash;
  typedef QHash<int, QByteArray> MethodNameHash;

  const QMetaObject *_meta;
  ArgHash _argTypes;
  QHash<int, int> _returnType;
  QHash<int, int> _minArgCount;
  MethodNameHash _methodNames;
  int _updatedRemotelyId; // id of the updatedRemotely() signal - makes things faster
  QHash<QByteArray, int> _syncMap;
  QHash<int, int> _receiveMap;
};


// ==================================================
//  Peers
// ==================================================
class SignalProxy::AbstractPeer {
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

class SignalProxy::IODevicePeer : public SignalProxy::AbstractPeer {
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

class SignalProxy::SignalProxyPeer : public SignalProxy::AbstractPeer {
public:
  SignalProxyPeer(SignalProxy *sender, SignalProxy *receiver) : AbstractPeer(AbstractPeer::SignalProxyPeer), sender(sender), receiver(receiver) {}
  virtual void dispatchSignal(const RequestType &requestType, const QVariantList &params);
  virtual inline bool isSecure() const { return true; }
private:
  SignalProxy *sender;
  SignalProxy *receiver;
};

#endif

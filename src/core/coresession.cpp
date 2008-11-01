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

#include <QtScript>

#include "core.h"
#include "coresession.h"
#include "networkconnection.h"
#include "userinputhandler.h"

#include "signalproxy.h"
#include "buffersyncer.h"
#include "corebacklogmanager.h"
#include "corebufferviewmanager.h"
#include "coreirclisthelper.h"
#include "storage.h"

#include "corenetwork.h"
#include "ircuser.h"
#include "ircchannel.h"
#include "identity.h"

#include "util.h"
#include "coreusersettings.h"
#include "logger.h"

CoreSession::CoreSession(UserId uid, bool restoreState, QObject *parent)
  : QObject(parent),
    _user(uid),
    _signalProxy(new SignalProxy(SignalProxy::Server, 0, this)),
    _aliasManager(this),
    _bufferSyncer(new BufferSyncer(this)),
    _backlogManager(new CoreBacklogManager(this)),
    _bufferViewManager(new CoreBufferViewManager(_signalProxy, this)),
    _ircListHelper(new CoreIrcListHelper(this)),
    _coreInfo(this),
    scriptEngine(new QScriptEngine(this))
{

  SignalProxy *p = signalProxy();
  connect(p, SIGNAL(peerRemoved(QIODevice *)), this, SLOT(removeClient(QIODevice *)));

  connect(p, SIGNAL(connected()), this, SLOT(clientsConnected()));
  connect(p, SIGNAL(disconnected()), this, SLOT(clientsDisconnected()));

  //p->attachSlot(SIGNAL(disconnectFromNetwork(NetworkId)), this, SLOT(disconnectFromNetwork(NetworkId))); // FIXME
  p->attachSlot(SIGNAL(sendInput(BufferInfo, QString)), this, SLOT(msgFromClient(BufferInfo, QString)));
  p->attachSignal(this, SIGNAL(displayMsg(Message)));
  p->attachSignal(this, SIGNAL(displayStatusMsg(QString, QString)));
  p->attachSignal(this, SIGNAL(bufferInfoUpdated(BufferInfo)));

  p->attachSignal(this, SIGNAL(identityCreated(const Identity &)));
  p->attachSignal(this, SIGNAL(identityRemoved(IdentityId)));
  p->attachSlot(SIGNAL(createIdentity(const Identity &)), this, SLOT(createIdentity(const Identity &)));
  p->attachSlot(SIGNAL(removeIdentity(IdentityId)), this, SLOT(removeIdentity(IdentityId)));

  p->attachSignal(this, SIGNAL(networkCreated(NetworkId)));
  p->attachSignal(this, SIGNAL(networkRemoved(NetworkId)));
  p->attachSlot(SIGNAL(createNetwork(const NetworkInfo &)), this, SLOT(createNetwork(const NetworkInfo &)));
  p->attachSlot(SIGNAL(removeNetwork(NetworkId)), this, SLOT(removeNetwork(NetworkId)));

  loadSettings();
  initScriptEngine();

  // init BufferSyncer
  QHash<BufferId, MsgId> lastSeenHash = Core::bufferLastSeenMsgIds(user());
  foreach(BufferId id, lastSeenHash.keys())
    _bufferSyncer->requestSetLastSeenMsg(id, lastSeenHash[id]);

  connect(_bufferSyncer, SIGNAL(lastSeenMsgSet(BufferId, MsgId)), this, SLOT(storeBufferLastSeenMsg(BufferId, MsgId)));
  connect(_bufferSyncer, SIGNAL(removeBufferRequested(BufferId)), this, SLOT(removeBufferRequested(BufferId)));
  connect(this, SIGNAL(bufferRemoved(BufferId)), _bufferSyncer, SLOT(removeBuffer(BufferId)));
  connect(this, SIGNAL(bufferRenamed(BufferId, QString)), _bufferSyncer, SLOT(renameBuffer(BufferId, QString)));
  p->synchronize(_bufferSyncer);


  // init alias manager
  p->synchronize(&aliasManager());

  // init BacklogManager
  p->synchronize(_backlogManager);

  // init IrcListHelper
  p->synchronize(ircListHelper());

  // init CoreInfo
  p->synchronize(&_coreInfo);

  // Restore session state
  if(restoreState) restoreSessionState();

  emit initialized();
}

CoreSession::~CoreSession() {
  saveSessionState();
  foreach(NetworkConnection *conn, _connections.values()) {
    delete conn;
  }
  foreach(CoreNetwork *net, _networks.values()) {
    delete net;
  }
}

CoreNetwork *CoreSession::network(NetworkId id) const {
  if(_networks.contains(id)) return _networks[id];
  return 0;
}

NetworkConnection *CoreSession::networkConnection(NetworkId id) const {
  if(_connections.contains(id)) return _connections[id];
  return 0;
}

Identity *CoreSession::identity(IdentityId id) const {
  if(_identities.contains(id)) return _identities[id];
  return 0;
}

void CoreSession::loadSettings() {
  CoreUserSettings s(user());

  foreach(IdentityId id, s.identityIds()) {
    Identity *i = new Identity(s.identity(id), this);
    if(!i->isValid()) {
      qWarning() << "Invalid identity! Removing...";
      s.removeIdentity(id);
      delete i;
      continue;
    }
    if(_identities.contains(i->id())) {
      qWarning() << "Duplicate identity, ignoring!";
      delete i;
      continue;
    }
    connect(i, SIGNAL(updated(const QVariantMap &)), this, SLOT(identityUpdated(const QVariantMap &)));
    _identities[i->id()] = i;
    signalProxy()->synchronize(i);
  }
  if(!_identities.count()) {
    Identity i(1);
    i.setToDefaults();
    i.setIdentityName(tr("Default Identity"));
    createIdentity(i);
  }

  foreach(NetworkInfo info, Core::networks(user())) {
    createNetwork(info);
  }
}

void CoreSession::saveSessionState() const {

}

void CoreSession::restoreSessionState() {
  QList<NetworkId> nets = Core::connectedNetworks(user());
  foreach(NetworkId id, nets) {
    connectToNetwork(id);
  }
}

void CoreSession::updateBufferInfo(UserId uid, const BufferInfo &bufinfo) {
  if(uid == user()) emit bufferInfoUpdated(bufinfo);
}

void CoreSession::connectToNetwork(NetworkId id) {
  CoreNetwork *net = network(id);
  if(!net) {
    qWarning() << "Connect to unknown network requested! net:" << id << "user:" << user();
    return;
  }

  NetworkConnection *conn = networkConnection(id);
  if(!conn) {
    conn = new NetworkConnection(net, this);
    _connections[id] = conn;
    attachNetworkConnection(conn);
  }
  conn->connectToIrc();
}

void CoreSession::attachNetworkConnection(NetworkConnection *conn) {
  connect(conn, SIGNAL(connected(NetworkId)), this, SLOT(networkConnected(NetworkId)));
  connect(conn, SIGNAL(quitRequested(NetworkId)), this, SLOT(networkDisconnected(NetworkId)));

  // I guess we don't need these anymore, client-side can just connect the network's signals directly
  //signalProxy()->attachSignal(conn, SIGNAL(connected(NetworkId)), SIGNAL(networkConnected(NetworkId)));
  //signalProxy()->attachSignal(conn, SIGNAL(disconnected(NetworkId)), SIGNAL(networkDisconnected(NetworkId)));

  connect(conn, SIGNAL(displayMsg(Message::Type, BufferInfo::Type, QString, QString, QString, Message::Flags)),
	  this, SLOT(recvMessageFromServer(Message::Type, BufferInfo::Type, QString, QString, QString, Message::Flags)));
  connect(conn, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));

  connect(conn, SIGNAL(nickChanged(const NetworkId &, const QString &, const QString &)),
	  this, SLOT(renameBuffer(const NetworkId &, const QString &, const QString &)));
  connect(conn, SIGNAL(channelJoined(NetworkId, const QString &, const QString &)),
          this, SLOT(channelJoined(NetworkId, const QString &, const QString &)));
  connect(conn, SIGNAL(channelParted(NetworkId, const QString &)),
          this, SLOT(channelParted(NetworkId, const QString &)));
}

void CoreSession::disconnectFromNetwork(NetworkId id) {
  if(!_connections.contains(id))
    return;

  //_connections[id]->disconnectFromIrc();
  _connections[id]->userInputHandler()->handleQuit(BufferInfo(), QString());
}

void CoreSession::networkStateRequested() {
}

void CoreSession::addClient(QIODevice *device) {
  if(!device) {
    qCritical() << "Invoking CoreSession::addClient with a QObject that is not a QIODevice!";
  } else {
    // if the socket is an orphan, the signalProxy adopts it.
    // -> we don't need to care about it anymore
    device->setParent(0);
    signalProxy()->addPeer(device);
    QVariantMap reply;
    reply["MsgType"] = "SessionInit";
    reply["SessionState"] = sessionState();
    SignalProxy::writeDataToDevice(device, reply);
  }
}

void CoreSession::addClient(SignalProxy *proxy) {
  signalProxy()->addPeer(proxy);
  emit sessionState(sessionState());
}

void CoreSession::removeClient(QIODevice *iodev) {
  QTcpSocket *socket = qobject_cast<QTcpSocket *>(iodev);
  if(socket)
    quInfo() << qPrintable(tr("Client")) << qPrintable(socket->peerAddress().toString()) << qPrintable(tr("disconnected (UserId: %1).").arg(user().toInt()));
}

SignalProxy *CoreSession::signalProxy() const {
  return _signalProxy;
}

// FIXME we need a sane way for creating buffers!
void CoreSession::networkConnected(NetworkId networkid) {
  Core::bufferInfo(user(), networkid, BufferInfo::StatusBuffer); // create status buffer
  Core::setNetworkConnected(user(), networkid, true);
}

// called now only on /quit and requested disconnects, not on normal disconnects!
void CoreSession::networkDisconnected(NetworkId networkid) {
  // if the network has already been removed, we don't have a networkconnection left either, so we don't do anything
  // make sure to not depend on the network still existing when calling this function!
  if(_connections.contains(networkid)) {
    Core::setNetworkConnected(user(), networkid, false);
    _connections.take(networkid)->deleteLater();
  }
}

void CoreSession::channelJoined(NetworkId id, const QString &channel, const QString &key) {
  Core::setChannelPersistent(user(), id, channel, true);
  Core::setPersistentChannelKey(user(), id, channel, key);
}

void CoreSession::channelParted(NetworkId id, const QString &channel) {
  Core::setChannelPersistent(user(), id, channel, false);
}

QHash<QString, QString> CoreSession::persistentChannels(NetworkId id) const {
  return Core::persistentChannels(user(), id);
  return QHash<QString, QString>();
}

// FIXME switch to BufferId
void CoreSession::msgFromClient(BufferInfo bufinfo, QString msg) {
  NetworkConnection *conn = networkConnection(bufinfo.networkId());
  if(conn) {
    conn->userInput(bufinfo, msg);
  } else {
    qWarning() << "Trying to send to unconnected network:" << msg;
  }
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void CoreSession::recvMessageFromServer(Message::Type type, BufferInfo::Type bufferType,
                                        QString target, QString text, QString sender, Message::Flags flags) {
  NetworkConnection *netCon = qobject_cast<NetworkConnection*>(this->sender());
  Q_ASSERT(netCon);

  BufferInfo bufferInfo = Core::bufferInfo(user(), netCon->networkId(), bufferType, target);
  Message msg(bufferInfo, type, text, sender, flags);
  msg.setMsgId(Core::storeMessage(msg));
  Q_ASSERT(msg.msgId() != 0);
  emit displayMsg(msg);
}

void CoreSession::recvStatusMsgFromServer(QString msg) {
  NetworkConnection *s = qobject_cast<NetworkConnection*>(sender());
  Q_ASSERT(s);
  emit displayStatusMsg(s->networkName(), msg);
}

QList<BufferInfo> CoreSession::buffers() const {
  return Core::requestBuffers(user());
}


QVariant CoreSession::sessionState() {
  QVariantMap v;

  QVariantList bufs;
  foreach(BufferInfo id, buffers()) bufs << qVariantFromValue(id);
  v["BufferInfos"] = bufs;
  QVariantList networkids;
  foreach(NetworkId id, _networks.keys()) networkids << qVariantFromValue(id);
  v["NetworkIds"] = networkids;

  quint32 ircusercount = 0;
  quint32 ircchannelcount = 0;
  foreach(Network *net, _networks.values()) {
    ircusercount += net->ircUserCount();
    ircchannelcount += net->ircChannelCount();
  }
  v["IrcUserCount"] = ircusercount;
  v["IrcChannelCount"] = ircchannelcount;

  QList<QVariant> idlist;
  foreach(Identity *i, _identities.values()) idlist << qVariantFromValue(*i);
  v["Identities"] = idlist;

  //v["Payload"] = QByteArray(100000000, 'a');  // for testing purposes
  return v;
}

void CoreSession::storeBufferLastSeenMsg(BufferId buffer, const MsgId &msgId) {
  Core::setBufferLastSeenMsg(user(), buffer, msgId);
}

void CoreSession::initScriptEngine() {
  signalProxy()->attachSlot(SIGNAL(scriptRequest(QString)), this, SLOT(scriptRequest(QString)));
  signalProxy()->attachSignal(this, SIGNAL(scriptResult(QString)));

  // FIXME
  //QScriptValue storage_ = scriptEngine->newQObject(storage);
  //scriptEngine->globalObject().setProperty("storage", storage_);
}

void CoreSession::scriptRequest(QString script) {
  emit scriptResult(scriptEngine->evaluate(script).toString());
}

/*** Identity Handling ***/

void CoreSession::createIdentity(const Identity &id) {
  // find free ID
  int i;
  for(i = 1; i <= _identities.count(); i++) {
    if(!_identities.keys().contains(i)) break;
  }
  //qDebug() << "found free id" << i;
  Identity *newId = new Identity(id, this);
  newId->setId(i);
  _identities[i] = newId;
  signalProxy()->synchronize(newId);
  CoreUserSettings s(user());
  s.storeIdentity(*newId);
  connect(newId, SIGNAL(updated(const QVariantMap &)), this, SLOT(identityUpdated(const QVariantMap &)));
  emit identityCreated(*newId);
}

void CoreSession::removeIdentity(IdentityId id) {
  Identity *i = _identities.take(id);
  if(i) {
    emit identityRemoved(id);
    CoreUserSettings s(user());
    s.removeIdentity(id);
    i->deleteLater();
  }
}

void CoreSession::identityUpdated(const QVariantMap &data) {
  IdentityId id = data.value("identityId", 0).value<IdentityId>();
  if(!id.isValid() || !_identities.contains(id)) {
    qWarning() << "Update request for unknown identity received!";
    return;
  }
  CoreUserSettings s(user());
  s.storeIdentity(*_identities.value(id));
}

/*** Network Handling ***/

void CoreSession::createNetwork(const NetworkInfo &info_) {
  NetworkInfo info = info_;
  int id;

  if(!info.networkId.isValid())
    Core::createNetwork(user(), info);

  if(!info.networkId.isValid()) {
    qWarning() << qPrintable(tr("CoreSession::createNetwork(): Got invalid networkId from Core when trying to create network %1!").arg(info.networkName));
    return;
  }

  id = info.networkId.toInt();
  if(!_networks.contains(id)) {
    CoreNetwork *net = new CoreNetwork(id, this);
    connect(net, SIGNAL(connectRequested(NetworkId)), this, SLOT(connectToNetwork(NetworkId)));
    connect(net, SIGNAL(disconnectRequested(NetworkId)), this, SLOT(disconnectFromNetwork(NetworkId)));
    net->setNetworkInfo(info);
    net->setProxy(signalProxy());
    _networks[id] = net;
    signalProxy()->synchronize(net);
    emit networkCreated(id);
  } else {
    qWarning() << qPrintable(tr("CoreSession::createNetwork(): Trying to create a network that already exists, updating instead!"));
    _networks[info.networkId]->requestSetNetworkInfo(info);
  }
}

void CoreSession::removeNetwork(NetworkId id) {
  // Make sure the network is disconnected!
  NetworkConnection *conn = _connections.value(id, 0);
  if(conn) {
    if(conn->connectionState() != Network::Disconnected) {
      connect(conn, SIGNAL(disconnected(NetworkId)), this, SLOT(destroyNetwork(NetworkId)));
      conn->disconnectFromIrc();
    } else {
      _connections.take(id)->deleteLater();  // TODO make this saner
      destroyNetwork(id);
    }
  } else {
    destroyNetwork(id);
  }
}

void CoreSession::destroyNetwork(NetworkId id) {
  if(_connections.contains(id)) {
    // this can happen if the network was reconnecting while being removed
    _connections.take(id)->deleteLater();
  }
  QList<BufferId> removedBuffers = Core::requestBufferIdsForNetwork(user(), id);
  Network *net = _networks.take(id);
  if(net && Core::removeNetwork(user(), id)) {
    foreach(BufferId bufferId, removedBuffers) {
      _bufferSyncer->removeBuffer(bufferId);
    }
    emit networkRemoved(id);
    net->deleteLater();
  }
}

void CoreSession::removeBufferRequested(BufferId bufferId) {
  BufferInfo bufferInfo = Core::getBufferInfo(user(), bufferId);
  if(!bufferInfo.isValid()) {
    qWarning() << "CoreSession::removeBufferRequested(): invalid BufferId:" << bufferId << "for User:" << user();
    return;
  }

  if(bufferInfo.type() == BufferInfo::StatusBuffer) {
    qWarning() << "CoreSession::removeBufferRequested(): Status Buffers cannot be removed!";
    return;
  }

  if(bufferInfo.type() == BufferInfo::ChannelBuffer) {
    CoreNetwork *net = network(bufferInfo.networkId());
    if(!net) {
      qWarning() << "CoreSession::removeBufferRequested(): Received BufferInfo with unknown networkId!";
      return;
    }
    IrcChannel *chan = net->ircChannel(bufferInfo.bufferName());
    if(chan) {
      qWarning() << "CoreSession::removeBufferRequested(): Unable to remove Buffer for joined Channel:" << bufferInfo.bufferName();
      return;
    }
  }
  if(Core::removeBuffer(user(), bufferId))
    emit bufferRemoved(bufferId);
}

void CoreSession::renameBuffer(const NetworkId &networkId, const QString &newName, const QString &oldName) {
  BufferId bufferId = Core::renameBuffer(user(), networkId, newName, oldName);
  if(bufferId.isValid()) {
    emit bufferRenamed(bufferId, newName);
  }
}

void CoreSession::clientsConnected() {
  QHash<NetworkId, NetworkConnection *>::iterator conIter = _connections.begin();
  Identity *identity = 0;
  NetworkConnection *con = 0;
  Network *network = 0;
  IrcUser *me = 0;
  QString awayReason;
  while(conIter != _connections.end()) {
    con = *conIter;
    conIter++;

    if(!con->isConnected())
      continue;
    identity = con->identity();
    if(!identity)
      continue;
    network = con->network();
    if(!network)
      continue;
    me = network->me();
    if(!me)
      continue;

    if(identity->detachAwayEnabled() && me->isAway()) {
      con->userInputHandler()->handleAway(BufferInfo(), QString());
    }
  }
}

void CoreSession::clientsDisconnected() {
  QHash<NetworkId, NetworkConnection *>::iterator conIter = _connections.begin();
  Identity *identity = 0;
  NetworkConnection *con = 0;
  Network *network = 0;
  IrcUser *me = 0;
  QString awayReason;
  while(conIter != _connections.end()) {
    con = *conIter;
    conIter++;

    if(!con->isConnected())
      continue;
    identity = con->identity();
    if(!identity)
      continue;
    network = con->network();
    if(!network)
      continue;
    me = network->me();
    if(!me)
      continue;

    if(identity->detachAwayEnabled() && !me->isAway()) {
      if(identity->detachAwayReasonEnabled())
	awayReason = identity->detachAwayReason();
      else
	awayReason = identity->awayReason();
      network->setAutoAwayActive(true);
      con->userInputHandler()->handleAway(BufferInfo(), awayReason);
    }
  }
}

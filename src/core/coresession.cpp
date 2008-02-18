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

#include "signalproxy.h"
#include "buffersyncer.h"
#include "storage.h"

#include "network.h"
#include "ircuser.h"
#include "ircchannel.h"
#include "identity.h"

#include "util.h"
#include "coreusersettings.h"

CoreSession::CoreSession(UserId uid, bool restoreState, QObject *parent) : QObject(parent),
    _user(uid),
    _signalProxy(new SignalProxy(SignalProxy::Server, 0, this)),
    _bufferSyncer(new BufferSyncer(this)),
    scriptEngine(new QScriptEngine(this))
{

  SignalProxy *p = signalProxy();

  p->attachSlot(SIGNAL(requestConnect(QString)), this, SLOT(connectToNetwork(QString)));
  p->attachSlot(SIGNAL(disconnectFromNetwork(NetworkId)), this, SLOT(disconnectFromNetwork(NetworkId))); // FIXME
  p->attachSlot(SIGNAL(sendInput(BufferInfo, QString)), this, SLOT(msgFromClient(BufferInfo, QString)));
  p->attachSlot(SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)), this, SLOT(sendBacklog(BufferInfo, QVariant, QVariant)));
  p->attachSignal(this, SIGNAL(displayMsg(Message)));
  p->attachSignal(this, SIGNAL(displayStatusMsg(QString, QString)));
  p->attachSignal(this, SIGNAL(backlogData(BufferInfo, QVariantList, bool)));
  p->attachSignal(this, SIGNAL(bufferInfoUpdated(BufferInfo)));

  p->attachSignal(this, SIGNAL(identityCreated(const Identity &)));
  p->attachSignal(this, SIGNAL(identityRemoved(IdentityId)));
  p->attachSlot(SIGNAL(createIdentity(const Identity &)), this, SLOT(createIdentity(const Identity &)));
  p->attachSlot(SIGNAL(updateIdentity(const Identity &)), this, SLOT(updateIdentity(const Identity &)));
  p->attachSlot(SIGNAL(removeIdentity(IdentityId)), this, SLOT(removeIdentity(IdentityId)));

  p->attachSignal(this, SIGNAL(networkCreated(NetworkId)));
  p->attachSignal(this, SIGNAL(networkRemoved(NetworkId)));
  p->attachSlot(SIGNAL(createNetwork(const NetworkInfo &)), this, SLOT(createNetwork(const NetworkInfo &)));
  p->attachSlot(SIGNAL(updateNetwork(const NetworkInfo &)), this, SLOT(updateNetwork(const NetworkInfo &)));
  p->attachSlot(SIGNAL(removeNetwork(NetworkId)), this, SLOT(removeNetwork(NetworkId)));

  loadSettings();
  initScriptEngine();

  // init BufferSyncer
  QHash<BufferId, QDateTime> lastSeenHash = Core::bufferLastSeenDates(user());
  foreach(BufferId id, lastSeenHash.keys()) _bufferSyncer->requestSetLastSeen(id, lastSeenHash[id]);
  connect(_bufferSyncer, SIGNAL(lastSeenSet(BufferId, const QDateTime &)), this, SLOT(storeBufferLastSeen(BufferId, const QDateTime &)));
  connect(_bufferSyncer, SIGNAL(removeBufferRequested(BufferId)), this, SLOT(removeBufferRequested(BufferId)));
  connect(this, SIGNAL(bufferRemoved(BufferId)), _bufferSyncer, SLOT(removeBuffer(BufferId)));
  p->synchronize(_bufferSyncer);

  // Restore session state
  if(restoreState) restoreSessionState();

  emit initialized();
}

CoreSession::~CoreSession() {
  saveSessionState();
  foreach(NetworkConnection *conn, _connections.values()) {
    delete conn;
  }
  foreach(Network *net, _networks.values()) {
    delete net;
  }
}

UserId CoreSession::user() const {
  return _user;
}

Network *CoreSession::network(NetworkId id) const {
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
      qWarning() << QString("Invalid identity! Removing...");
      s.removeIdentity(id);
      delete i;
      continue;
    }
    if(_identities.contains(i->id())) {
      qWarning() << "Duplicate identity, ignoring!";
      delete i;
      continue;
    }
    _identities[i->id()] = i;
    signalProxy()->synchronize(i);
  }
  if(!_identities.count()) {
    Identity i(1);
    i.setToDefaults();
    i.setIdentityName(tr("Default Identity"));
    createIdentity(i);
  }


  // migration to pure DB storage
  QList<NetworkId> netIds = s.networkIds();
  if(!netIds.isEmpty()) {
    qDebug() << "Migrating Networksettings to DB Storage for User:" << user();
    foreach(NetworkId id, netIds) {
      NetworkInfo info = s.networkInfo(id);

      // default new options
      info.useRandomServer = false;
      info.useAutoReconnect = true;
      info.autoReconnectInterval = 60;
      info.autoReconnectRetries = 20;
      info.unlimitedReconnectRetries = false;
      info.useAutoIdentify = false;
      info.autoIdentifyService = "NickServ";
      info.rejoinChannels = true;

      Core::updateNetwork(user(), info);
      s.removeNetworkInfo(id);
    }
  }

  foreach(NetworkInfo info, Core::networks(user())) {
    createNetwork(info);
  }
}

void CoreSession::saveSessionState() const {
  QVariantMap res;
  QVariantList conn;
  foreach(NetworkConnection *net, _connections.values()) {
    QVariantMap m;
    m["NetworkId"] = QVariant::fromValue<NetworkId>(net->networkId());
    m["State"] = net->state();
    conn << m;
  }
  res["CoreBuild"] = Global::quasselBuild;
  res["ConnectedNetworks"] = conn;
  CoreUserSettings s(user());
  s.setSessionState(res);
}

void CoreSession::restoreSessionState() {
  CoreUserSettings s(user());
  uint build = s.sessionState().toMap()["CoreBuild"].toUInt();
  if(build < 362) {
    qWarning() << qPrintable(tr("Session state does not exist or is too old!"));
    return;
  }
  QVariantList conn = s.sessionState().toMap()["ConnectedNetworks"].toList();
  foreach(QVariant v, conn) {
    NetworkId id = v.toMap()["NetworkId"].value<NetworkId>();
    if(_networks.keys().contains(id)) connectToNetwork(id, v.toMap()["State"]);
  }
}

void CoreSession::updateBufferInfo(UserId uid, const BufferInfo &bufinfo) {
  if(uid == user()) emit bufferInfoUpdated(bufinfo);
}

// FIXME remove
void CoreSession::connectToNetwork(QString netname, const QVariant &previousState) {
  Network *net = 0;
  foreach(Network *n, _networks.values()) {
    if(n->networkName() == netname) {
      net = n; break;
    }
  }
  if(!net) {
    qWarning() << "Connect to unknown network requested, ignoring!";
    return;
  }
  connectToNetwork(net->networkId(), previousState);
}

void CoreSession::connectToNetwork(NetworkId id, const QVariant &previousState) {
  Network *net = network(id);
  if(!net) {
    qWarning() << "Connect to unknown network requested! net:" << id << "user:" << user();
    return;
  }

  NetworkConnection *conn = networkConnection(id);
  if(!conn) {
    conn = new NetworkConnection(net, this, previousState);
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

  connect(conn, SIGNAL(displayMsg(Message::Type, BufferInfo::Type, QString, QString, QString, quint8)),
	  this, SLOT(recvMessageFromServer(Message::Type, BufferInfo::Type, QString, QString, QString, quint8)));
  connect(conn, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));

}

void CoreSession::disconnectFromNetwork(NetworkId id) {
  if(!_connections.contains(id)) return;
  _connections[id]->disconnectFromIrc();
}

void CoreSession::networkStateRequested() {
}

void CoreSession::addClient(QObject *dev) { // this is QObject* so we can use it in signal connections
  QIODevice *device = qobject_cast<QIODevice *>(dev);
  if(!device) {
    qWarning() << "Invoking CoreSession::addClient with a QObject that is not a QIODevice!";
  } else {
    signalProxy()->addPeer(device);
    QVariantMap reply;
    reply["MsgType"] = "SessionInit";
    reply["SessionState"] = sessionState();
    SignalProxy::writeDataToDevice(device, reply);
  }
}

SignalProxy *CoreSession::signalProxy() const {
  return _signalProxy;
}

// FIXME we need a sane way for creating buffers!
void CoreSession::networkConnected(NetworkId networkid) {
  Core::bufferInfo(user(), networkid, BufferInfo::StatusBuffer); // create status buffer
}

// called now only on /quit and requested disconnects, not on normal disconnects!
void CoreSession::networkDisconnected(NetworkId networkid) {
  if(_connections.contains(networkid)) _connections.take(networkid)->deleteLater();
}

// FIXME switch to BufferId
void CoreSession::msgFromClient(BufferInfo bufinfo, QString msg) {
  NetworkConnection *conn = networkConnection(bufinfo.networkId());
  if(conn) {
    conn->userInput(bufinfo, msg);
  } else {
    qWarning() << "Trying to send to unconnected network!";
  }
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void CoreSession::recvMessageFromServer(Message::Type type, BufferInfo::Type bufferType, QString target, QString text, QString sender, quint8 flags) {
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

void CoreSession::storeBufferLastSeen(BufferId buffer, const QDateTime &lastSeen) {
  Core::setBufferLastSeen(user(), buffer, lastSeen);
}

void CoreSession::sendBacklog(BufferInfo id, QVariant v1, QVariant v2) {
  QList<QVariant> log;
  QList<Message> msglist;
  if(v1.type() == QVariant::DateTime) {


  } else {
    msglist = Core::requestMsgs(id, v1.toInt(), v2.toInt());
  }

  // Send messages out in smaller packages - we don't want to make the signal data too large!
  for(int i = 0; i < msglist.count(); i++) {
    log.append(qVariantFromValue(msglist[i]));
    if(log.count() >= 5) {
      emit backlogData(id, log, i >= msglist.count() - 1);
      log.clear();
    }
  }
  if(log.count() > 0) emit backlogData(id, log, true);
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
  emit identityCreated(*newId);
}

void CoreSession::updateIdentity(const Identity &id) {
  if(!_identities.contains(id.id())) {
    qWarning() << "Update request for unknown identity received!";
    return;
  }
  _identities[id.id()]->update(id);

  CoreUserSettings s(user());
  s.storeIdentity(id);
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

/*** Network Handling ***/

void CoreSession::createNetwork(const NetworkInfo &info_) {
  NetworkInfo info = info_;
  int id;

  if(!info.networkId.isValid())
    Core::createNetwork(user(), info);

  Q_ASSERT(info.networkId.isValid());

  id = info.networkId.toInt();
  Q_ASSERT(!_networks.contains(id));
  
  Network *net = new Network(id, this);
  connect(net, SIGNAL(connectRequested(NetworkId)), this, SLOT(connectToNetwork(NetworkId)));
  connect(net, SIGNAL(disconnectRequested(NetworkId)), this, SLOT(disconnectFromNetwork(NetworkId)));
  net->setNetworkInfo(info);
  net->setProxy(signalProxy());
  _networks[id] = net;
  signalProxy()->synchronize(net);
  emit networkCreated(id);
}

void CoreSession::updateNetwork(const NetworkInfo &info) {
  if(!_networks.contains(info.networkId)) {
    qWarning() << "Update request for unknown network received!";
    return;
  }
  _networks[info.networkId]->setNetworkInfo(info);
  Core::updateNetwork(user(), info);
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
  Q_ASSERT(!_connections.contains(id));
  Network *net = _networks.take(id);
  if(net && Core::removeNetwork(user(), id)) {
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
    Network *net = network(bufferInfo.networkId());
    Q_ASSERT(net);
    IrcChannel *chan = net->ircChannel(bufferInfo.bufferName());
    if(chan) {
      qWarning() << "CoreSession::removeBufferRequested(): Unable to remove Buffer for joined Channel:" << bufferInfo.bufferName();
      return;
    }
  }
  if(Core::removeBuffer(user(), bufferId))
    emit bufferRemoved(bufferId);
}

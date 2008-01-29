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

#include "core.h"
#include "coresession.h"
#include "networkconnection.h"

#include "signalproxy.h"
#include "storage.h"

#include "network.h"
#include "ircuser.h"
#include "ircchannel.h"
#include "identity.h"

#include "util.h"
#include "coreusersettings.h"

#include <QtScript>

CoreSession::CoreSession(UserId uid, bool restoreState, QObject *parent) : QObject(parent),
    _user(uid),
    _signalProxy(new SignalProxy(SignalProxy::Server, 0, this)),
    scriptEngine(new QScriptEngine(this))
{

  SignalProxy *p = signalProxy();

  CoreUserSettings s(user());
  sessionData = s.sessionData();

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
  }
  if(!_identities.count()) {
    Identity i(1);
    i.setToDefaults();
    i.setIdentityName(tr("Default Identity"));
    createIdentity(i);
  }

  //p->attachSlot(SIGNAL(requestNetworkStates()), this, SLOT(networkStateRequested()));
  p->attachSlot(SIGNAL(requestConnect(QString)), this, SLOT(connectToNetwork(QString)));
  p->attachSlot(SIGNAL(disconnectFromNetwork(NetworkId)), this, SLOT(disconnectFromNetwork(NetworkId))); // FIXME
  p->attachSlot(SIGNAL(sendInput(BufferInfo, QString)), this, SLOT(msgFromClient(BufferInfo, QString)));
  p->attachSlot(SIGNAL(requestBacklog(BufferInfo, QVariant, QVariant)), this, SLOT(sendBacklog(BufferInfo, QVariant, QVariant)));
  p->attachSignal(this, SIGNAL(displayMsg(Message)));
  p->attachSignal(this, SIGNAL(displayStatusMsg(QString, QString)));
  p->attachSignal(this, SIGNAL(backlogData(BufferInfo, QVariantList, bool)));
  p->attachSignal(this, SIGNAL(bufferInfoUpdated(BufferInfo)));

  p->attachSignal(this, SIGNAL(sessionDataChanged(const QString &, const QVariant &)), SIGNAL(coreSessionDataChanged(const QString &, const QVariant &)));
  p->attachSlot(SIGNAL(clientSessionDataChanged(const QString &, const QVariant &)), this, SLOT(storeSessionData(const QString &, const QVariant &)));

  p->attachSignal(this, SIGNAL(identityCreated(const Identity &)));
  p->attachSignal(this, SIGNAL(identityRemoved(IdentityId)));
  p->attachSlot(SIGNAL(createIdentity(const Identity &)), this, SLOT(createIdentity(const Identity &)));
  p->attachSlot(SIGNAL(updateIdentity(const Identity &)), this, SLOT(updateIdentity(const Identity &)));
  p->attachSlot(SIGNAL(removeIdentity(IdentityId)), this, SLOT(removeIdentity(IdentityId)));

  initScriptEngine();

  foreach(Identity *id, _identities.values()) {
    p->synchronize(id);
  }

  // Load and init networks.
  // FIXME For now we use the old info from sessionData...

  QVariantMap networks = retrieveSessionData("Networks").toMap();
  foreach(QString netname, networks.keys()) {
    QVariantMap network = networks[netname].toMap();
    NetworkId netid = Core::networkId(user(), netname);
    Network *net = new Network(netid, this);
    connect(net, SIGNAL(connectRequested(NetworkId)), this, SLOT(connectToNetwork(NetworkId)));
    net->setNetworkName(netname);
    net->setIdentity(1); // FIXME default identity for now
    net->setCodecForEncoding("ISO-8859-15"); // FIXME
    net->setCodecForDecoding("ISO-8859-15"); // FIXME
    QList<QVariantMap> slist;
    foreach(QVariant v, network["Servers"].toList()) {
      QVariantMap server;
      server["Host"] = v.toMap()["Address"];
      server["Port"] = v.toMap()["Port"];
      slist << server;
    }
    net->setServerList(slist);
    net->setProxy(p);
    _networks[netid] = net;
    p->synchronize(net);
  }

  // Restore session state
  if(restoreState) restoreSessionState();

  emit initialized();
}

CoreSession::~CoreSession() {
  saveSessionState();
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


void CoreSession::storeSessionData(const QString &key, const QVariant &data) {
  CoreUserSettings s(user());
  s.setSessionValue(key, data);
  sessionData[key] = data;
  emit sessionDataChanged(key, data);
  emit sessionDataChanged(key);
}

QVariant CoreSession::retrieveSessionData(const QString &key, const QVariant &def) {
  QVariant data;
  if(!sessionData.contains(key)) data = def;
  else data = sessionData[key];
  return data;
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
    conn->connectToIrc();
  }
}

void CoreSession::attachNetworkConnection(NetworkConnection *conn) {
  //connect(this, SIGNAL(connectToIrc(QString)), network, SLOT(connectToIrc(QString)));
  //connect(this, SIGNAL(disconnectFromIrc(QString)), network, SLOT(disconnectFromIrc(QString)));
  //connect(this, SIGNAL(msgFromGui(uint, QString, QString)), network, SLOT(userInput(uint, QString, QString)));

  connect(conn, SIGNAL(connected(NetworkId)), this, SLOT(networkConnected(NetworkId)));
  connect(conn, SIGNAL(disconnected(NetworkId)), this, SLOT(networkDisconnected(NetworkId)));
  signalProxy()->attachSignal(conn, SIGNAL(connected(NetworkId)), SIGNAL(networkConnected(NetworkId)));
  signalProxy()->attachSignal(conn, SIGNAL(disconnected(NetworkId)), SIGNAL(networkDisconnected(NetworkId)));

  connect(conn, SIGNAL(displayMsg(Message::Type, QString, QString, QString, quint8)), this, SLOT(recvMessageFromServer(Message::Type, QString, QString, QString, quint8)));
  connect(conn, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));

  // TODO add error handling
}

void CoreSession::disconnectFromNetwork(NetworkId id) {
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

void CoreSession::networkConnected(NetworkId networkid) {
  network(networkid)->setConnected(true);
  Core::bufferInfo(user(), networkConnection(networkid)->networkName()); // create status buffer
}

void CoreSession::networkDisconnected(NetworkId networkid) {
  // FIXME
  // connection should only go away on explicit /part, and handle reconnections etcpp internally otherwise
  network(networkid)->setConnected(false);

  Q_ASSERT(_connections.contains(networkid));
  _connections.take(networkid)->deleteLater();
  Q_ASSERT(!_connections.contains(networkid));
}

// FIXME switch to BufferId
void CoreSession::msgFromClient(BufferInfo bufinfo, QString msg) {
  NetworkConnection *conn = networkConnection(bufinfo.networkId());
  if(conn) {
    conn->userInput(bufinfo.buffer(), msg);
  } else {
    qWarning() << "Trying to send to unconnected network!";
  }
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void CoreSession::recvMessageFromServer(Message::Type type, QString target, QString text, QString sender, quint8 flags) {
  NetworkConnection *s = qobject_cast<NetworkConnection*>(this->sender());
  Q_ASSERT(s);
  BufferInfo buf;
  if((flags & Message::PrivMsg) && !(flags & Message::Self)) {
    buf = Core::bufferInfo(user(), s->networkName(), nickFromMask(sender));
  } else {
    buf = Core::bufferInfo(user(), s->networkName(), target);
  }
  Message msg(buf, type, text, sender, flags);
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

  v["SessionData"] = sessionData;

    //v["Payload"] = QByteArray(100000000, 'a');  // for testing purposes
  return v;
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
#include <QDebug>
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


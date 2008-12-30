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
#include "userinputhandler.h"
#include "signalproxy.h"
#include "corebuffersyncer.h"
#include "corebacklogmanager.h"
#include "corebufferviewmanager.h"
#include "coreirclisthelper.h"
#include "storage.h"

#include "coreidentity.h"
#include "corenetwork.h"
#include "ircuser.h"
#include "ircchannel.h"

#include "util.h"
#include "coreusersettings.h"
#include "logger.h"

CoreSession::CoreSession(UserId uid, bool restoreState, QObject *parent)
  : QObject(parent),
    _user(uid),
    _signalProxy(new SignalProxy(SignalProxy::Server, 0, this)),
    _aliasManager(this),
    _bufferSyncer(new CoreBufferSyncer(this)),
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

  p->attachSlot(SIGNAL(sendInput(BufferInfo, QString)), this, SLOT(msgFromClient(BufferInfo, QString)));
  p->attachSignal(this, SIGNAL(displayMsg(Message)));
  p->attachSignal(this, SIGNAL(displayStatusMsg(QString, QString)));

  p->attachSignal(this, SIGNAL(identityCreated(const Identity &)));
  p->attachSignal(this, SIGNAL(identityRemoved(IdentityId)));
  p->attachSlot(SIGNAL(createIdentity(const Identity &, const QVariantMap &)), this, SLOT(createIdentity(const Identity &, const QVariantMap &)));
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

  connect(&(Core::instance()->syncTimer()), SIGNAL(timeout()), _bufferSyncer, SLOT(storeDirtyIds()));
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
  foreach(CoreNetwork *net, _networks.values()) {
    delete net;
  }
}

CoreNetwork *CoreSession::network(NetworkId id) const {
  if(_networks.contains(id)) return _networks[id];
  return 0;
}

CoreIdentity *CoreSession::identity(IdentityId id) const {
  if(_identities.contains(id)) return _identities[id];
  return 0;
}

void CoreSession::loadSettings() {
  CoreUserSettings s(user());

  // migrate to db
  QList<IdentityId> ids = s.identityIds();
  QList<NetworkInfo> networkInfos = Core::networks(user());
  foreach(IdentityId id, ids) {
    CoreIdentity identity(s.identity(id));
    IdentityId newId = Core::createIdentity(user(), identity);
    QList<NetworkInfo>::iterator networkIter = networkInfos.begin();
    while(networkIter != networkInfos.end()) {
      if(networkIter->identity == id) {
	networkIter->identity = newId;
	Core::updateNetwork(user(), *networkIter);
	networkIter = networkInfos.erase(networkIter);
      } else {
	networkIter++;
      }
    }
    s.removeIdentity(id);
  }
  // end of migration

  foreach(CoreIdentity identity, Core::identities(user())) {
    createIdentity(identity);
  }
  if(!_identities.count()) {
    Identity identity;
    identity.setToDefaults();
    identity.setIdentityName(tr("Default Identity"));
    createIdentity(identity, QVariantMap());
  }

  foreach(NetworkInfo info, Core::networks(user())) {
    createNetwork(info);
  }
}

void CoreSession::saveSessionState() const {
  _bufferSyncer->storeDirtyIds();
}

void CoreSession::restoreSessionState() {
  QList<NetworkId> nets = Core::connectedNetworks(user());
  CoreNetwork *net = 0;
  foreach(NetworkId id, nets) {
    net = network(id);
    Q_ASSERT(net);
    net->connectToIrc();
  }
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

QHash<QString, QString> CoreSession::persistentChannels(NetworkId id) const {
  return Core::persistentChannels(user(), id);
  return QHash<QString, QString>();
}

// FIXME switch to BufferId
void CoreSession::msgFromClient(BufferInfo bufinfo, QString msg) {
  CoreNetwork *net = network(bufinfo.networkId());
  if(net) {
    net->userInput(bufinfo, msg);
  } else {
    qWarning() << "Trying to send to unconnected network:" << msg;
  }
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void CoreSession::recvMessageFromServer(Message::Type type, BufferInfo::Type bufferType,
                                        QString target, QString text, QString sender, Message::Flags flags) {
  CoreNetwork *net = qobject_cast<CoreNetwork*>(this->sender());
  Q_ASSERT(net);

  BufferInfo bufferInfo = Core::bufferInfo(user(), net->networkId(), bufferType, target);
  Message msg(bufferInfo, type, text, sender, flags);
  msg.setMsgId(Core::storeMessage(msg));
  Q_ASSERT(msg.msgId() != 0);
  emit displayMsg(msg);
}

void CoreSession::recvStatusMsgFromServer(QString msg) {
  CoreNetwork *net = qobject_cast<CoreNetwork*>(sender());
  Q_ASSERT(net);
  emit displayStatusMsg(net->networkName(), msg);
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
void CoreSession::createIdentity(const Identity &identity, const QVariantMap &additional) {
#ifndef HAVE_SSL
  Q_UNUSED(additional)
#endif

  CoreIdentity coreIdentity(identity);
#ifdef HAVE_SSL
  if(additional.contains("KeyPem"))
    coreIdentity.setSslKey(additional["KeyPem"].toByteArray());
  if(additional.contains("CertPem"))
    coreIdentity.setSslCert(additional["CertPem"].toByteArray());
#endif
  IdentityId id = Core::createIdentity(user(), coreIdentity);
  if(!id.isValid())
    return;
  else
    createIdentity(coreIdentity);
}

void CoreSession::createIdentity(const CoreIdentity &identity) {
  CoreIdentity *coreIdentity = new CoreIdentity(identity, this);
  _identities[identity.id()] = coreIdentity;
  // CoreIdentity has it's own synchronize method since it's "private" sslManager needs to be synced aswell
  coreIdentity->synchronize(signalProxy());
  connect(coreIdentity, SIGNAL(updated(const QVariantMap &)), this, SLOT(updateIdentityBySender()));
  emit identityCreated(*coreIdentity);
}

void CoreSession::updateIdentityBySender() {
  CoreIdentity *identity = qobject_cast<CoreIdentity *>(sender());
  if(!identity)
    return;
  Core::updateIdentity(user(), *identity);
}

void CoreSession::removeIdentity(IdentityId id) {
  CoreIdentity *identity = _identities.take(id);
  if(identity) {
    emit identityRemoved(id);
    Core::removeIdentity(user(), id);
    identity->deleteLater();
  }
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
    connect(net, SIGNAL(displayMsg(Message::Type, BufferInfo::Type, QString, QString, QString, Message::Flags)),
	    this, SLOT(recvMessageFromServer(Message::Type, BufferInfo::Type, QString, QString, QString, Message::Flags)));
    connect(net, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));

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
  CoreNetwork *net = network(id);
  if(!net)
    return;

  if(net->connectionState() != Network::Disconnected) {
    connect(net, SIGNAL(disconnected(NetworkId)), this, SLOT(destroyNetwork(NetworkId)));
    net->disconnectFromIrc();
  } else {
    destroyNetwork(id);
  }
}

void CoreSession::destroyNetwork(NetworkId id) {
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

void CoreSession::renameBuffer(const NetworkId &networkId, const QString &newName, const QString &oldName) {
  BufferInfo bufferInfo = Core::bufferInfo(user(), networkId, BufferInfo::QueryBuffer, oldName, false);
  if(bufferInfo.isValid()) {
    _bufferSyncer->renameBuffer(bufferInfo.bufferId(), newName);
  }
}

void CoreSession::clientsConnected() {
  QHash<NetworkId, CoreNetwork *>::iterator netIter = _networks.begin();
  Identity *identity = 0;
  CoreNetwork *net = 0;
  IrcUser *me = 0;
  QString awayReason;
  while(netIter != _networks.end()) {
    net = *netIter;
    netIter++;

    if(!net->isConnected())
      continue;
    identity = net->identityPtr();
    if(!identity)
      continue;
    me = net->me();
    if(!me)
      continue;

    if(identity->detachAwayEnabled() && me->isAway()) {
      net->userInputHandler()->handleAway(BufferInfo(), QString());
    }
  }
}

void CoreSession::clientsDisconnected() {
  QHash<NetworkId, CoreNetwork *>::iterator netIter = _networks.begin();
  Identity *identity = 0;
  CoreNetwork *net = 0;
  IrcUser *me = 0;
  QString awayReason;
  while(netIter != _networks.end()) {
    net = *netIter;
    netIter++;

    if(!net->isConnected())
      continue;
    identity = net->identityPtr();
    if(!identity)
      continue;
    me = net->me();
    if(!me)
      continue;

    if(identity->detachAwayEnabled() && !me->isAway()) {
      if(identity->detachAwayReasonEnabled())
	awayReason = identity->detachAwayReason();
      else
	awayReason = identity->awayReason();
      net->setAutoAwayActive(true);
      net->userInputHandler()->handleAway(BufferInfo(), awayReason);
    }
  }
}

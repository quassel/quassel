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

#include "client.h"

#include "bufferinfo.h"
#include "global.h"
#include "identity.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "message.h"
#include "network.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "nickmodel.h"
#include "quasselui.h"
#include "signalproxy.h"
#include "util.h"

QPointer<Client> Client::instanceptr = 0;

/*** Initialization/destruction ***/

Client *Client::instance() {
  if(!instanceptr)
    instanceptr = new Client();
  return instanceptr;
}

void Client::destroy() {
  delete instanceptr;
}

void Client::init(AbstractUi *ui) {
  instance()->mainUi = ui;
  instance()->init();
}

Client::Client(QObject *parent)
  : QObject(parent),
    socket(0),
    _signalProxy(new SignalProxy(SignalProxy::Client, this)),
    mainUi(0),
    _networkModel(0),
    _bufferModel(0),
    _nickModel(0),
    connectedToCore(false)
{
}

Client::~Client() {
}

void Client::init() {
  blockSize = 0;

  _networkModel = new NetworkModel(this);
  connect(this, SIGNAL(bufferUpdated(BufferInfo)),
          _networkModel, SLOT(bufferUpdated(BufferInfo)));

  _bufferModel = new BufferModel(_networkModel);
  _nickModel = new NickModel(_networkModel);
  
  SignalProxy *p = signalProxy();
  p->attachSignal(this, SIGNAL(sendSessionData(const QString &, const QVariant &)),
                  SIGNAL(clientSessionDataChanged(const QString &, const QVariant &)));
  p->attachSlot(SIGNAL(coreSessionDataChanged(const QString &, const QVariant &)),
                this, SLOT(recvSessionData(const QString &, const QVariant &)));
  p->attachSlot(SIGNAL(coreState(const QVariant &)),
                this, SLOT(recvCoreState(const QVariant &)));
  p->attachSlot(SIGNAL(networkConnected(uint)),
                this, SLOT(networkConnected(uint)));
  p->attachSlot(SIGNAL(networkDisconnected(uint)),
                this, SLOT(networkDisconnected(uint)));
  p->attachSlot(SIGNAL(displayMsg(const Message &)),
                this, SLOT(recvMessage(const Message &)));
  p->attachSlot(SIGNAL(displayStatusMsg(QString, QString)),
                this, SLOT(recvStatusMsg(QString, QString)));


  p->attachSlot(SIGNAL(backlogData(BufferInfo, const QVariantList &, bool)), this, SLOT(recvBacklogData(BufferInfo, const QVariantList &, bool)));
  p->attachSlot(SIGNAL(bufferInfoUpdated(BufferInfo)), this, SLOT(updateBufferInfo(BufferInfo)));
  p->attachSignal(this, SIGNAL(sendInput(BufferInfo, QString)));
  p->attachSignal(this, SIGNAL(requestNetworkStates()));

  p->attachSignal(this, SIGNAL(requestCreateIdentity(const Identity &)), SIGNAL(createIdentity(const Identity &)));
  p->attachSignal(this, SIGNAL(requestUpdateIdentity(const Identity &)), SIGNAL(updateIdentity(const Identity &)));
  p->attachSignal(this, SIGNAL(requestRemoveIdentity(IdentityId)), SIGNAL(removeIdentity(IdentityId)));
  p->attachSlot(SIGNAL(identityCreated(const Identity &)), this, SLOT(coreIdentityCreated(const Identity &)));
  p->attachSlot(SIGNAL(identityRemoved(IdentityId)), this, SLOT(coreIdentityRemoved(IdentityId)));

  connect(mainUi, SIGNAL(connectToCore(const QVariantMap &)), this, SLOT(connectToCore(const QVariantMap &)));
  connect(mainUi, SIGNAL(disconnectFromCore()), this, SLOT(disconnectFromCore()));
  connect(this, SIGNAL(connected()), mainUi, SLOT(connectedToCore()));
  connect(this, SIGNAL(disconnected()), mainUi, SLOT(disconnectedFromCore()));

  layoutTimer = new QTimer(this);
  layoutTimer->setInterval(0);
  layoutTimer->setSingleShot(false);
  connect(layoutTimer, SIGNAL(timeout()), this, SLOT(layoutMsg()));

}

/*** public static methods ***/


QList<Network *> Client::networks() {
  return instance()->_network.values();
}

Network *Client::network(uint networkid) {
  if(instance()->_network.contains(networkid))
    return instance()->_network[networkid];
  else
    return 0;
}

QList<BufferInfo> Client::allBufferInfos() {
  QList<BufferInfo> bufferids;
  foreach(Buffer *buffer, buffers()) {
    bufferids << buffer->bufferInfo();
  }
  return bufferids;
}

QList<Buffer *> Client::buffers() {
  return instance()->_buffers.values();
}

Buffer *Client::buffer(uint bufferUid) {
  if(instance()->_buffers.contains(bufferUid))
    return instance()->_buffers[bufferUid];
  else
    return 0;
}

Buffer *Client::buffer(BufferInfo id) {
  Buffer *buff = buffer(id.uid());

  if(!buff) {
    Client *client = Client::instance();
    buff = new Buffer(id, client);

    connect(buff, SIGNAL(userInput(BufferInfo, QString)),
	    client, SLOT(userInput(BufferInfo, QString)));
    connect(buff, SIGNAL(destroyed()),
	    client, SLOT(bufferDestroyed()));
    client->_buffers[id.uid()] = buff;
    emit client->bufferUpdated(id);
  }
  Q_ASSERT(buff);
  return buff;
}

NetworkModel *Client::networkModel() {
  return instance()->_networkModel;
}

BufferModel *Client::bufferModel() {
  return instance()->_bufferModel;
}

NickModel *Client::nickModel() {
  return instance()->_nickModel;
}


SignalProxy *Client::signalProxy() {
  return instance()->_signalProxy;
}


/*** Identity handling ***/

QList<IdentityId> Client::identityIds() {
  return instance()->_identities.keys();
}

const Identity * Client::identity(IdentityId id) {
  if(instance()->_identities.contains(id)) return instance()->_identities[id];
  else return 0;
}


void Client::createIdentity(const Identity &id) {
  emit instance()->requestCreateIdentity(id);
}

void Client::updateIdentity(const Identity &id) {
  emit instance()->requestUpdateIdentity(id);
}

void Client::removeIdentity(IdentityId id) {
  emit instance()->requestRemoveIdentity(id);
}

void Client::coreIdentityCreated(const Identity &other) {
  if(!_identities.contains(other.id())) {
    Identity *identity = new Identity(other, this);
    _identities[other.id()] = identity;
    identity->setInitialized();
    signalProxy()->synchronize(identity);
    emit identityCreated(other.id());
  } else {
    qWarning() << tr("Identity already exists in client!");
  }
}

void Client::coreIdentityRemoved(IdentityId id) {
  if(_identities.contains(id)) {
    emit identityRemoved(id);
    Identity *i = _identities.take(id);
    i->deleteLater();
  }
}

/***  ***/


bool Client::isConnected() {
  return instance()->connectedToCore;
}

void Client::fakeInput(uint bufferUid, QString message) {
  Buffer *buff = buffer(bufferUid);
  if(!buff)
    qWarning() << "No Buffer with uid" << bufferUid << "can't send Input" << message;
  else
    emit instance()->sendInput(buff->bufferInfo(), message);
}

void Client::fakeInput(BufferInfo bufferInfo, QString message) {
  fakeInput(bufferInfo, message);
}

void Client::connectToCore(const QVariantMap &conn) {
  // TODO implement SSL
  coreConnectionInfo = conn;
  if(isConnected()) {
    emit coreConnectionError(tr("Already connected to Core!"));
    return;
  }
  
  if(socket != 0)
    socket->deleteLater();
  
  if(conn["Host"].toString().isEmpty()) {
    clientMode = LocalCore;
    socket = new QBuffer(this);
    connect(socket, SIGNAL(readyRead()), this, SLOT(coreHasData()));
    socket->open(QIODevice::ReadWrite);
    //QVariant state = connectToLocalCore(coreConnectionInfo["User"].toString(), coreConnectionInfo["Password"].toString());
    //syncToCore(state);
    coreSocketConnected();
  } else {
    clientMode = RemoteCore;
    emit coreConnectionMsg(tr("Connecting..."));
    Q_ASSERT(!socket);
    QTcpSocket *sock = new QTcpSocket(this);
    socket = sock;
    connect(sock, SIGNAL(readyRead()), this, SLOT(coreHasData()));
    connect(sock, SIGNAL(connected()), this, SLOT(coreSocketConnected()));
    connect(signalProxy(), SIGNAL(disconnected()), this, SLOT(coreSocketDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));
    sock->connectToHost(conn["Host"].toString(), conn["Port"].toUInt());
  }
}

void Client::disconnectFromCore() {
  socket->close();
}

void Client::setCoreConfiguration(const QVariantMap &settings) {
  SignalProxy::writeDataToDevice(socket, settings);
}

void Client::coreSocketConnected() {
  connect(this, SIGNAL(recvPartialItem(uint, uint)), this, SIGNAL(coreConnectionProgress(uint, uint)));
  emit coreConnectionMsg(tr("Synchronizing to core..."));
  QVariantMap clientInit;
  clientInit["GuiProtocol"] = GUI_PROTOCOL;
  clientInit["User"] = coreConnectionInfo["User"].toString();
  clientInit["Password"] = coreConnectionInfo["Password"].toString();
  SignalProxy::writeDataToDevice(socket, clientInit);
}

void Client::coreSocketDisconnected() {
  instance()->connectedToCore = false;
  emit disconnected();
  emit coreConnectionStateChanged(false);
  socket->deleteLater();
  blockSize = 0;

  /* Clear internal data. Hopefully nothing relies on it at this point. */
  _networkModel->clear();

  QHash<BufferId, Buffer *>::iterator bufferIter =  _buffers.begin();
  while(bufferIter != _buffers.end()) {
    Buffer *buffer = bufferIter.value();
    disconnect(buffer, SIGNAL(destroyed()), this, 0);
    bufferIter = _buffers.erase(bufferIter);
    buffer->deleteLater();
  }
  Q_ASSERT(_buffers.isEmpty());


  QHash<NetworkId, Network*>::iterator netIter = _network.begin();
  while(netIter != _network.end()) {
    Network *net = netIter.value();
    disconnect(net, SIGNAL(destroyed()), this, 0);
    netIter = _network.erase(netIter);
    net->deleteLater();
  }
  Q_ASSERT(_network.isEmpty());

  QHash<IdentityId, Identity*>::iterator idIter = _identities.begin();
  while(idIter != _identities.end()) {
    Identity *id = idIter.value();
    emit identityRemoved(id->id());
    idIter = _identities.erase(idIter);
    id->deleteLater();
  }
  Q_ASSERT(_identities.isEmpty());

  coreConnectionInfo.clear();
  sessionData.clear();
  layoutQueue.clear();
  layoutTimer->stop();
}

void Client::recvCoreState(const QVariant &state) {
  disconnect(this, SIGNAL(recvPartialItem(uint, uint)), this, SIGNAL(coreConnectionProgress(uint, uint)));
  disconnect(socket, 0, this, 0);  // rest of communication happens through SignalProxy
  signalProxy()->addPeer(socket);
  syncToCore(state);
}

// TODO: auth errors
void Client::syncToCore(const QVariant &coreState) {
  if(!coreState.toMap().contains("SessionState")) {
    emit coreConnectionError(tr("Invalid data received from core!"));
    disconnectFromCore();
    return;
  }

  QVariantMap sessionState = coreState.toMap()["SessionState"].toMap();

  // store sessionData
  QVariantMap sessData = sessionState["SessionData"].toMap();
  foreach(QString key, sessData.keys())
    recvSessionData(key, sessData[key]);

  // create identities
  foreach(QVariant vid, sessionState["Identities"].toList()) {
    coreIdentityCreated(vid.value<Identity>());
  }

  // store Buffer details
  QVariantList coreBuffers = sessionState["Buffers"].toList();
  /* make lookups by id faster */
  foreach(QVariant vid, coreBuffers) {
    buffer(vid.value<BufferInfo>()); // create all buffers, so we see them in the network views
  }

  // create network objects
  QVariantList networkids = sessionState["Networks"].toList();
  foreach(QVariant networkid, networkids) {
    networkConnected(networkid.toUInt());
  }

  instance()->connectedToCore = true;
  updateCoreConnectionProgress();

}

void Client::updateCoreConnectionProgress() {
  // we'll do this in three steps:
  // 1.) networks
  // 2.) channels
  // 3.) ircusers

  int numNets = networks().count();
  int numNetsWaiting = 0;

  int numIrcUsers = 0;
  int numIrcUsersWaiting = 0;

  int numChannels = 0;
  int numChannelsWaiting = 0;

  foreach(Network *net, networks()) {
    if(! net->initialized())
      numNetsWaiting++;

    numIrcUsers += net->ircUsers().count();
    foreach(IrcUser *user, net->ircUsers()) {
      if(! user->initialized())
	numIrcUsersWaiting++;
    }

    numChannels += net->ircChannels().count();
    foreach(IrcChannel *channel, net->ircChannels()) {
      if(! channel->initialized())
	numChannelsWaiting++;
    }
  }

  if(numNetsWaiting > 0) {
    emit coreConnectionMsg(tr("Requesting network states..."));
    emit coreConnectionProgress(numNets - numNetsWaiting, numNets);
    return;
  }

  if(numIrcUsersWaiting > 0) {
    emit coreConnectionMsg(tr("Requesting User states..."));
    emit coreConnectionProgress(numIrcUsers - numIrcUsersWaiting, numIrcUsers);
    return;
  }

  if(numChannelsWaiting > 0) {
    emit coreConnectionMsg(tr("Requesting Channel states..."));
    emit coreConnectionProgress(numChannels - numChannelsWaiting, numChannels);
    return;
  }

  emit coreConnectionProgress(1,1);
  emit connected();
  emit coreConnectionStateChanged(true);
  foreach(Network *net, networks()) {
    disconnect(net, 0, this, SLOT(updateCoreConnectionProgress()));
  }

  // signalProxy()->dumpProxyStats();
}

void Client::recvSessionData(const QString &key, const QVariant &data) {
  sessionData[key] = data;
  emit sessionDataChanged(key, data);
  emit sessionDataChanged(key);
}

void Client::storeSessionData(const QString &key, const QVariant &data) {
  // Not sure if this is a good idea, but we'll try it anyway:
  // Calling this function only sends a signal to core. Data is stored upon reception of the update signal,
  // rather than immediately.
  emit instance()->sendSessionData(key, data);
}

QVariant Client::retrieveSessionData(const QString &key, const QVariant &def) {
  if(instance()->sessionData.contains(key)) return instance()->sessionData[key];
  else return def;
}

QStringList Client::sessionDataKeys() {
  return instance()->sessionData.keys();
}

void Client::coreSocketError(QAbstractSocket::SocketError) {
  emit coreConnectionError(socket->errorString());
  socket->deleteLater();
}

void Client::coreHasData() {
  QVariant item;
  if(SignalProxy::readDataFromDevice(socket, blockSize, item)) {
    emit recvPartialItem(1,1);
    QVariantMap msg = item.toMap();
    if (!msg["StartWizard"].toBool()) {
      recvCoreState(msg["Reply"]);
    } else {
      qWarning("Core not configured!");
      qDebug() << "Available storage providers: " << msg["StorageProviders"].toStringList();
      emit showConfigWizard(msg);
    }
    blockSize = 0;
    return;
  }
  if(blockSize > 0) {
    emit recvPartialItem(socket->bytesAvailable(), blockSize);
  }
}

void Client::networkConnected(uint netid) {
  // TODO: create statusBuffer / switch to networkids
  //BufferInfo id = statusBufferInfo(net);
  //Buffer *b = buffer(id);
  //b->setActive(true);

  Network *netinfo = new Network(netid, this);
  netinfo->setProxy(signalProxy());
  networkModel()->attachNetwork(netinfo);
  
  if(!isConnected()) {
    connect(netinfo, SIGNAL(initDone()), this, SLOT(updateCoreConnectionProgress()));
    connect(netinfo, SIGNAL(ircUserInitDone()), this, SLOT(updateCoreConnectionProgress()));
    connect(netinfo, SIGNAL(ircChannelInitDone()), this, SLOT(updateCoreConnectionProgress()));
  }
  connect(netinfo, SIGNAL(destroyed()), this, SLOT(networkDestroyed()));
  _network[netid] = netinfo;
}

void Client::networkDisconnected(uint networkid) {
  if(!_network.contains(networkid)) {
    qWarning() << "Client::networkDisconnected(uint): unknown Network" << networkid;
    return;
  }

  Network *net = _network.take(networkid);
  if(!net->initialized()) {
    qDebug() << "Network" << networkid << "disconnected while not yet initialized!";
    updateCoreConnectionProgress();
  }
  net->deleteLater();
}

void Client::updateBufferInfo(BufferInfo id) {
  emit bufferUpdated(id);
}

void Client::bufferDestroyed() {
  Buffer *buffer = static_cast<Buffer *>(sender());
  QHash<BufferId, Buffer *>::iterator iter = _buffers.begin();
  while(iter != _buffers.end()) {
    if(iter.value() == buffer) {
      iter = _buffers.erase(iter);
      break;
    }
    iter++;
  }
}

void Client::networkDestroyed() {
  Network *netinfo = static_cast<Network *>(sender());
  uint networkId = netinfo->networkId();
  if(_network.contains(networkId))
    _network.remove(networkId);
}

void Client::recvMessage(const Message &msg) {
  Buffer *b = buffer(msg.buffer());

//   Buffer::ActivityLevel level = Buffer::OtherActivity;
//   if(msg.type() == Message::Plain || msg.type() == Message::Notice){
//     level |= Buffer::NewMessage;
//   }
//   if(msg.flags() & Message::Highlight){
//     level |= Buffer::Highlight;
//   }
//   emit bufferActivity(level, b);

  b->appendMsg(msg);
}

void Client::recvStatusMsg(QString /*net*/, QString /*msg*/) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));
}

void Client::recvBacklogData(BufferInfo id, QVariantList msgs, bool /*done*/) {
  Buffer *b = buffer(id);
  foreach(QVariant v, msgs) {
    Message msg = v.value<Message>();
    b->prependMsg(msg);
    if(!layoutQueue.contains(b)) layoutQueue.append(b);
  }
  if(layoutQueue.count() && !layoutTimer->isActive()) layoutTimer->start();
}

void Client::layoutMsg() {
  if(layoutQueue.count()) {
    Buffer *b = layoutQueue.takeFirst();  // TODO make this the current buffer
    if(b->layoutMsg())
      layoutQueue.append(b);  // Buffer has more messages in its queue --> Round Robin
  }
  
  if(!layoutQueue.count())
    layoutTimer->stop();
}

AbstractUiMsg *Client::layoutMsg(const Message &msg) {
  return instance()->mainUi->layoutMsg(msg);
}

void Client::userInput(BufferInfo id, QString msg) {
  emit sendInput(id, msg);
}


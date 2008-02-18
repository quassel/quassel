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
#include "buffersyncer.h"
#include "global.h"
#include "identity.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "message.h"
#include "network.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "quasselui.h"
#include "signalproxy.h"
#include "util.h"

QPointer<Client> Client::instanceptr = 0;
AccountId Client::_currentCoreAccount = 0;

/*** Initialization/destruction ***/

Client *Client::instance() {
  if(!instanceptr)
    instanceptr = new Client();
  return instanceptr;
}

void Client::destroy() {
  //delete instanceptr;
  instanceptr->deleteLater();
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
    _bufferSyncer(0),
    _connectedToCore(false),
    _syncedToCore(false)
{
  _monitorBuffer = new Buffer(BufferInfo(), this);
}

Client::~Client() {
  disconnectFromCore();
}

void Client::init() {
  _currentCoreAccount = 0;
  _networkModel = new NetworkModel(this);
  connect(this, SIGNAL(bufferUpdated(BufferInfo)),
          _networkModel, SLOT(bufferUpdated(BufferInfo)));
  connect(this, SIGNAL(networkRemoved(NetworkId)),
	  _networkModel, SLOT(networkRemoved(NetworkId)));

  _bufferModel = new BufferModel(_networkModel);

  SignalProxy *p = signalProxy();

  p->attachSlot(SIGNAL(displayMsg(const Message &)), this, SLOT(recvMessage(const Message &)));
  p->attachSlot(SIGNAL(displayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));

  p->attachSlot(SIGNAL(backlogData(BufferInfo, const QVariantList &, bool)), this, SLOT(recvBacklogData(BufferInfo, const QVariantList &, bool)));
  p->attachSlot(SIGNAL(bufferInfoUpdated(BufferInfo)), this, SLOT(updateBufferInfo(BufferInfo)));
  p->attachSignal(this, SIGNAL(sendInput(BufferInfo, QString)));
  p->attachSignal(this, SIGNAL(requestNetworkStates()));

  p->attachSignal(this, SIGNAL(requestCreateIdentity(const Identity &)), SIGNAL(createIdentity(const Identity &)));
  p->attachSignal(this, SIGNAL(requestUpdateIdentity(const Identity &)), SIGNAL(updateIdentity(const Identity &)));
  p->attachSignal(this, SIGNAL(requestRemoveIdentity(IdentityId)), SIGNAL(removeIdentity(IdentityId)));
  p->attachSlot(SIGNAL(identityCreated(const Identity &)), this, SLOT(coreIdentityCreated(const Identity &)));
  p->attachSlot(SIGNAL(identityRemoved(IdentityId)), this, SLOT(coreIdentityRemoved(IdentityId)));

  p->attachSignal(this, SIGNAL(requestCreateNetwork(const NetworkInfo &)), SIGNAL(createNetwork(const NetworkInfo &)));
  p->attachSignal(this, SIGNAL(requestUpdateNetwork(const NetworkInfo &)), SIGNAL(updateNetwork(const NetworkInfo &)));
  p->attachSignal(this, SIGNAL(requestRemoveNetwork(NetworkId)), SIGNAL(removeNetwork(NetworkId)));
  p->attachSlot(SIGNAL(networkCreated(NetworkId)), this, SLOT(coreNetworkCreated(NetworkId)));
  p->attachSlot(SIGNAL(networkRemoved(NetworkId)), this, SLOT(coreNetworkRemoved(NetworkId)));

  connect(p, SIGNAL(disconnected()), this, SLOT(disconnectFromCore()));

  //connect(mainUi, SIGNAL(connectToCore(const QVariantMap &)), this, SLOT(connectToCore(const QVariantMap &)));
  connect(mainUi, SIGNAL(disconnectFromCore()), this, SLOT(disconnectFromCore()));
  connect(this, SIGNAL(connected()), mainUi, SLOT(connectedToCore()));
  connect(this, SIGNAL(disconnected()), mainUi, SLOT(disconnectedFromCore()));

  layoutTimer = new QTimer(this);
  layoutTimer->setInterval(0);
  layoutTimer->setSingleShot(false);
  connect(layoutTimer, SIGNAL(timeout()), this, SLOT(layoutMsg()));

}

/*** public static methods ***/

AccountId Client::currentCoreAccount() {
  return _currentCoreAccount;
}

void Client::setCurrentCoreAccount(AccountId id) {
  _currentCoreAccount = id;
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


// FIXME remove
Buffer *Client::buffer(BufferId bufferId) {
  if(instance()->_buffers.contains(bufferId))
    return instance()->_buffers[bufferId];
  else
    return 0;
}

// FIXME remove
Buffer *Client::buffer(BufferInfo bufferInfo) {
  Buffer *buff = buffer(bufferInfo.bufferId());

  if(!buff) {
    Client *client = Client::instance();
    buff = new Buffer(bufferInfo, client);
    connect(buff, SIGNAL(destroyed()),
	    client, SLOT(bufferDestroyed()));
    client->_buffers[bufferInfo.bufferId()] = buff;
    emit client->bufferUpdated(bufferInfo);
  }
  Q_ASSERT(buff);
  return buff;
}

bool Client::isConnected() {
  return instance()->_connectedToCore;
}

bool Client::isSynced() {
  return instance()->_syncedToCore;
}

/*** Network handling ***/

QList<NetworkId> Client::networkIds() {
  return instance()->_networks.keys();
}

const Network * Client::network(NetworkId networkid) {
  if(instance()->_networks.contains(networkid)) return instance()->_networks[networkid];
  else return 0;
}

void Client::createNetwork(const NetworkInfo &info) {
  emit instance()->requestCreateNetwork(info);
}

void Client::updateNetwork(const NetworkInfo &info) {
  emit instance()->requestUpdateNetwork(info);
}

void Client::removeNetwork(NetworkId id) {
  emit instance()->requestRemoveNetwork(id);
}

void Client::addNetwork(Network *net) {
  net->setProxy(signalProxy());
  signalProxy()->synchronize(net);
  networkModel()->attachNetwork(net);
  connect(net, SIGNAL(destroyed()), instance(), SLOT(networkDestroyed()));
  instance()->_networks[net->networkId()] = net;
  emit instance()->networkCreated(net->networkId());
}

void Client::coreNetworkCreated(NetworkId id) {
  if(_networks.contains(id)) {
    qWarning() << "Creation of already existing network requested!";
    return;
  }
  Network *net = new Network(id, this);
  addNetwork(net);
}

void Client::coreNetworkRemoved(NetworkId id) {
  if(!_networks.contains(id)) return;
  Network *net = _networks.take(id);
  emit networkRemoved(net->networkId());
  net->deleteLater();
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
void Client::userInput(BufferInfo bufferInfo, QString message) {
  emit instance()->sendInput(bufferInfo, message);
}

/*** core connection stuff ***/

void Client::setConnectedToCore(QIODevice *sock, AccountId id) {
  socket = sock;
  signalProxy()->addPeer(socket);
  _connectedToCore = true;
  setCurrentCoreAccount(id);
}

void Client::setSyncedToCore() {
  // create buffersyncer
  Q_ASSERT(!_bufferSyncer);
  _bufferSyncer = new BufferSyncer(this);
  connect(bufferSyncer(), SIGNAL(lastSeenSet(BufferId, const QDateTime &)), this, SLOT(updateLastSeen(BufferId, const QDateTime &)));
  connect(bufferSyncer(), SIGNAL(bufferRemoved(BufferId)), this, SLOT(bufferRemoved(BufferId)));
  signalProxy()->synchronize(bufferSyncer());

  _syncedToCore = true;
  emit connected();
  emit coreConnectionStateChanged(true);
}

void Client::disconnectFromCore() {
  if(!isConnected())
    return;
  
  if(socket) {
    socket->close();
    socket->deleteLater();
  }
  _connectedToCore = false;
  _syncedToCore = false;
  setCurrentCoreAccount(0);
  emit disconnected();
  emit coreConnectionStateChanged(false);

  // Clear internal data. Hopefully nothing relies on it at this point.
  if(_bufferSyncer) {
    _bufferSyncer->deleteLater();
    _bufferSyncer = 0;
  }
  _networkModel->clear();

  QHash<BufferId, Buffer *>::iterator bufferIter =  _buffers.begin();
  while(bufferIter != _buffers.end()) {
    Buffer *buffer = bufferIter.value();
    disconnect(buffer, SIGNAL(destroyed()), this, 0);
    bufferIter = _buffers.erase(bufferIter);
    buffer->deleteLater();
  }
  Q_ASSERT(_buffers.isEmpty());

  QHash<NetworkId, Network*>::iterator netIter = _networks.begin();
  while(netIter != _networks.end()) {
    Network *net = netIter.value();
    emit networkRemoved(net->networkId());
    disconnect(net, SIGNAL(destroyed()), this, 0);
    netIter = _networks.erase(netIter);
    net->deleteLater();
  }
  Q_ASSERT(_networks.isEmpty());

  QHash<IdentityId, Identity*>::iterator idIter = _identities.begin();
  while(idIter != _identities.end()) {
    Identity *id = idIter.value();
    emit identityRemoved(id->id());
    idIter = _identities.erase(idIter);
    id->deleteLater();
  }
  Q_ASSERT(_identities.isEmpty());

  layoutQueue.clear();
  layoutTimer->stop();
}

void Client::setCoreConfiguration(const QVariantMap &settings) {
  SignalProxy::writeDataToDevice(socket, settings);
}

/*** ***/

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
  Network *net = static_cast<Network *>(sender());
  QHash<NetworkId, Network *>::iterator netIter = _networks.begin();
  while(netIter != _networks.end()) {
    if(*netIter == net) {
      netIter = _networks.erase(netIter);
      break;
    } else {
      netIter++;
    }
  }
}

void Client::recvMessage(const Message &message) {
  Message msg = message;
  Buffer *b;
  
  if(msg.type() == Message::Error) {
    b = buffer(msg.bufferInfo().bufferId());
    if(!b) {
      // FIXME: if buffer doesn't exist, forward the message to the status or current buffer
      b = buffer(msg.bufferInfo());
    }
  } else {
    b = buffer(msg.bufferInfo());
  }

  checkForHighlight(msg);
  b->appendMsg(msg);
  //bufferModel()->updateBufferActivity(msg);

  if(msg.type() == Message::Plain || msg.type() == Message::Notice || msg.type() == Message::Action) {
    const Network *net = network(msg.bufferInfo().networkId());
    QString networkName = net != 0
      ? net->networkName() + ":"
      : QString();
    QString sender = networkName + msg.bufferInfo().bufferName() + ":" + msg.sender();
    Message mmsg = Message(msg.timestamp(), msg.bufferInfo(), msg.type(), msg.text(), sender, msg.flags());
    monitorBuffer()->appendMsg(mmsg);
  }
}

void Client::recvStatusMsg(QString /*net*/, QString /*msg*/) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));
}

void Client::recvBacklogData(BufferInfo id, QVariantList msgs, bool /*done*/) {
  Buffer *b = buffer(id);
  foreach(QVariant v, msgs) {
    Message msg = v.value<Message>();
    checkForHighlight(msg);
    b->prependMsg(msg);
    //networkModel()->updateBufferActivity(msg);
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

void Client::checkForHighlight(Message &msg) {
  const Network *net = network(msg.bufferInfo().networkId());
  if(net && !net->myNick().isEmpty()) {
    QRegExp nickRegExp("^(.*\\W)?" + QRegExp::escape(net->myNick()) + "(\\W.*)?$");
    if((msg.type() & (Message::Plain | Message::Notice | Message::Action)) && nickRegExp.exactMatch(msg.text()))
      msg.setFlags(msg.flags() | Message::Highlight);
  }
}

void Client::updateLastSeen(BufferId id, const QDateTime &lastSeen) {
  Buffer *b = buffer(id);
  if(!b) {
    qWarning() << "Client::updateLastSeen(): Unknown buffer" << id;
    return;
  }
  b->setLastSeen(lastSeen);
}

void Client::setBufferLastSeen(BufferId id, const QDateTime &lastSeen) {
  if(!bufferSyncer()) return;
  bufferSyncer()->requestSetLastSeen(id, lastSeen);
}

void Client::bufferRemoved(BufferId bufferId) {
  QModelIndex current = bufferModel()->currentIndex();
  if(current.data(NetworkModel::BufferIdRole).value<BufferId>() == bufferId) {
    // select the status buffer if the currently displayed buffer is about to be removed
    QModelIndex newCurrent = current.sibling(0,0);
    bufferModel()->standardSelectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    bufferModel()->standardSelectionModel()->select(newCurrent, QItemSelectionModel::ClearAndSelect);
  }
    
  networkModel()->removeBuffer(bufferId);
  if(_buffers.contains(bufferId)) {
    Buffer *buff = _buffers.take(bufferId);
    disconnect(buff, 0, this, 0);
    buff->deleteLater();
  }
}

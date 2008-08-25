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

#include "abstractmessageprocessor.h"
#include "bufferinfo.h"
#include "buffermodel.h"
#include "buffersettings.h"
#include "buffersyncer.h"
#include "bufferviewmanager.h"
#include "clientbacklogmanager.h"
#include "clientirclisthelper.h"
#include "global.h"
#include "identity.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "message.h"
#include "messagemodel.h"
#include "network.h"
#include "networkmodel.h"
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
    _backlogManager(new ClientBacklogManager(this)),
    _bufferViewManager(0),
    _ircListHelper(new ClientIrcListHelper(this)),
    _messageModel(0),
    _messageProcessor(0),
    _connectedToCore(false),
    _syncedToCore(false)
{
  _signalProxy->synchronize(_ircListHelper);
}

Client::~Client() {
  disconnectFromCore();
}

void Client::init() {
  _currentCoreAccount = 0;
  _networkModel = new NetworkModel(this);

  connect(this, SIGNAL(networkRemoved(NetworkId)),
          _networkModel, SLOT(networkRemoved(NetworkId)));

  _bufferModel = new BufferModel(_networkModel);
  _messageModel = mainUi->createMessageModel(this);
  _messageProcessor = mainUi->createMessageProcessor(this);

  SignalProxy *p = signalProxy();

  p->attachSlot(SIGNAL(displayMsg(const Message &)), this, SLOT(recvMessage(const Message &)));
  p->attachSlot(SIGNAL(displayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));

  p->attachSlot(SIGNAL(bufferInfoUpdated(BufferInfo)), _networkModel, SLOT(bufferUpdated(BufferInfo)));
  p->attachSignal(this, SIGNAL(sendInput(BufferInfo, QString)));
  p->attachSignal(this, SIGNAL(requestNetworkStates()));

  p->attachSignal(this, SIGNAL(requestCreateIdentity(const Identity &)), SIGNAL(createIdentity(const Identity &)));
  p->attachSignal(this, SIGNAL(requestRemoveIdentity(IdentityId)), SIGNAL(removeIdentity(IdentityId)));
  p->attachSlot(SIGNAL(identityCreated(const Identity &)), this, SLOT(coreIdentityCreated(const Identity &)));
  p->attachSlot(SIGNAL(identityRemoved(IdentityId)), this, SLOT(coreIdentityRemoved(IdentityId)));

  p->attachSignal(this, SIGNAL(requestCreateNetwork(const NetworkInfo &)), SIGNAL(createNetwork(const NetworkInfo &)));
  p->attachSignal(this, SIGNAL(requestRemoveNetwork(NetworkId)), SIGNAL(removeNetwork(NetworkId)));
  p->attachSlot(SIGNAL(networkCreated(NetworkId)), this, SLOT(coreNetworkCreated(NetworkId)));
  p->attachSlot(SIGNAL(networkRemoved(NetworkId)), this, SLOT(coreNetworkRemoved(NetworkId)));

  connect(p, SIGNAL(disconnected()), this, SLOT(disconnectFromCore()));

  //connect(mainUi, SIGNAL(connectToCore(const QVariantMap &)), this, SLOT(connectToCore(const QVariantMap &)));
  connect(mainUi, SIGNAL(disconnectFromCore()), this, SLOT(disconnectFromCore()));
  connect(this, SIGNAL(connected()), mainUi, SLOT(connectedToCore()));
  connect(this, SIGNAL(disconnected()), mainUi, SLOT(disconnectedFromCore()));

}

/*** public static methods ***/

AccountId Client::currentCoreAccount() {
  return _currentCoreAccount;
}

void Client::setCurrentCoreAccount(AccountId id) {
  _currentCoreAccount = id;
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

void Client::removeNetwork(NetworkId id) {
  emit instance()->requestRemoveNetwork(id);
}

void Client::updateNetwork(const NetworkInfo &info) {
  Network *netptr = instance()->_networks.value(info.networkId, 0);
  if(!netptr) {
    qWarning() << "Update for unknown network requested:" << info;
    return;
  }
  netptr->requestSetNetworkInfo(info);
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
  if(!_networks.contains(id))
    return;
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

void Client::updateIdentity(IdentityId id, const QVariantMap &ser) {
  Identity *idptr = instance()->_identities.value(id, 0);
  if(!idptr) {
    qWarning() << "Update for unknown identity requested:" << id;
    return;
  }
  idptr->requestUpdate(ser);
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
  connect(bufferSyncer(), SIGNAL(lastSeenMsgSet(BufferId, MsgId)), _networkModel, SLOT(setLastSeenMsgId(BufferId, MsgId)));
  connect(bufferSyncer(), SIGNAL(bufferRemoved(BufferId)), this, SLOT(bufferRemoved(BufferId)));
  connect(bufferSyncer(), SIGNAL(bufferRenamed(BufferId, QString)), this, SLOT(bufferRenamed(BufferId, QString)));
  signalProxy()->synchronize(bufferSyncer());

  // attach backlog manager
  signalProxy()->synchronize(backlogManager());

  // create a new BufferViewManager
  _bufferViewManager = new BufferViewManager(signalProxy(), this);

  _syncedToCore = true;
  emit connected();
  emit coreConnectionStateChanged(true);
}

void Client::setSecuredConnection() {
  emit securedConnection();
}

void Client::disconnectFromCore() {
  if(!isConnected())
    return;
  _connectedToCore = false;

  if(socket) {
    socket->close();
    socket->deleteLater();
  }
  _syncedToCore = false;
  emit disconnected();
  emit coreConnectionStateChanged(false);

  messageProcessor()->reset();

  // Clear internal data. Hopefully nothing relies on it at this point.
  setCurrentCoreAccount(0);

  if(_bufferSyncer) {
    _bufferSyncer->deleteLater();
    _bufferSyncer = 0;
  }

  if(_bufferViewManager) {
    _bufferViewManager->deleteLater();
    _bufferViewManager = 0;
  }

  _messageModel->clear();
  _networkModel->clear();

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

}

void Client::setCoreConfiguration(const QVariantMap &settings) {
  SignalProxy::writeDataToDevice(socket, settings);
}

/*** ***/

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

// Hmm... we never used this...
void Client::recvStatusMsg(QString /*net*/, QString /*msg*/) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));
}

void Client::recvMessage(const Message &msg_) {
  Message msg = msg_;
  messageProcessor()->process(msg);
}

void Client::setBufferLastSeenMsg(BufferId id, const MsgId &msgId) {
  if(!bufferSyncer())
    return;
  bufferSyncer()->requestSetLastSeenMsg(id, msgId);
}

void Client::removeBuffer(BufferId id) {
  if(!bufferSyncer()) return;
  bufferSyncer()->requestRemoveBuffer(id);
}

void Client::bufferRemoved(BufferId bufferId) {
  // select a sane buffer (status buffer)
  /* we have to manually select a buffer because otherwise inconsitent changes
   * to the model might occur:
   * the result of a buffer removal triggers a change in the selection model.
   * the newly selected buffer might be a channel that hasn't been selected yet
   * and a new nickview would be created (which never heard of the "rowsAboutToBeRemoved").
   * this new view (and/or) its sort filter will then only receive a "rowsRemoved" signal.
   */
  QModelIndex current = bufferModel()->currentIndex();
  if(current.data(NetworkModel::BufferIdRole).value<BufferId>() == bufferId) {
    bufferModel()->setCurrentIndex(current.sibling(0,0));
  }

  // and remove it from the model
  networkModel()->removeBuffer(bufferId);
}

void Client::bufferRenamed(BufferId bufferId, const QString &newName) {
  QModelIndex bufferIndex = networkModel()->bufferIndex(bufferId);
  if(bufferIndex.isValid()) {
    networkModel()->setData(bufferIndex, newName, Qt::DisplayRole);
  }
}

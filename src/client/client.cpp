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
    _nickModel(0),
    _connectedToCore(false),
    _syncedToCore(false)
{
}

Client::~Client() {
  disconnectFromCore();
}

void Client::init() {

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
  //p->attachSlot(SIGNAL(networkConnected(uint)),
  //FIXME              this, SLOT(networkConnected(uint)));
  //p->attachSlot(SIGNAL(networkDisconnected(uint)),
  //FIXME              this, SLOT(networkDisconnected(uint)));
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
/*
  p->attachSignal(this, SIGNAL(requestCreateNetwork(const NetworkInfo &)), SIGNAL(createNetwork(const NetworkInfo &)));
  p->attachSignal(this, SIGNAL(requestUpdateNetwork(const NetworkInfo &)), SIGNAL(updateNetwork(const NetworkInfo &)));
  p->attachSignal(this, SIGNAL(requestRemoveNetwork(NetworkId)), SIGNAL(removeNetwork(NetworkId)));
  p->attachSlot(SIGNAL(networkCreated(const NetworkInfo &)), this, SLOT(coreNetworkCreated(const NetworkInfo &)));
  p->attachSlot(SIGNAL(networkRemoved(NetworkId)), this, SLOT(coreNetworkRemoved(NetworkId)));
*/
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
Buffer *Client::buffer(BufferId bufferUid) {
  if(instance()->_buffers.contains(bufferUid))
    return instance()->_buffers[bufferUid];
  else
    return 0;
}

// FIXME remove
Buffer *Client::buffer(BufferInfo id) {
  Buffer *buff = buffer(id.uid());

  if(!buff) {
    Client *client = Client::instance();
    buff = new Buffer(id, client);
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

void Client::setConnectedToCore(QIODevice *sock) {
  socket = sock;
  signalProxy()->addPeer(socket);
  _connectedToCore = true;
}

void Client::setSyncedToCore() {
  _syncedToCore = true;
  emit connected();
  emit coreConnectionStateChanged(true);
}

void Client::disconnectFromCore() {
  if(socket) {
    socket->close();
    socket->deleteLater();
  }
  _connectedToCore = false;
  _syncedToCore = false;
  emit disconnected();
  emit coreConnectionStateChanged(false);

  // Clear internal data. Hopefully nothing relies on it at this point.
  _networkModel->clear();

  QHash<NetworkId, Network*>::iterator netIter = _networks.begin();
  while(netIter != _networks.end()) {
    Network *net = netIter.value();
    disconnect(net, SIGNAL(destroyed()), this, 0);
    netIter = _networks.erase(netIter);
    net->deleteLater();
  }
  Q_ASSERT(_networks.isEmpty());

  QHash<BufferId, Buffer *>::iterator bufferIter =  _buffers.begin();
  while(bufferIter != _buffers.end()) {
    Buffer *buffer = bufferIter.value();
    disconnect(buffer, SIGNAL(destroyed()), this, 0);
    bufferIter = _buffers.erase(bufferIter);
    buffer->deleteLater();
  }
  Q_ASSERT(_buffers.isEmpty());

  QHash<IdentityId, Identity*>::iterator idIter = _identities.begin();
  while(idIter != _identities.end()) {
    Identity *id = idIter.value();
    emit identityRemoved(id->id());
    idIter = _identities.erase(idIter);
    id->deleteLater();
  }
  Q_ASSERT(_identities.isEmpty());

  sessionData.clear();
  layoutQueue.clear();
  layoutTimer->stop();
}

void Client::setCoreConfiguration(const QVariantMap &settings) {
  SignalProxy::writeDataToDevice(socket, settings);
}

/*** Session data ***/

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

/*** ***/

// FIXME
void Client::disconnectFromNetwork(NetworkId id) {
  if(!instance()->_networks.contains(id)) return;
  Network *net = instance()->_networks[id];
  net->requestDisconnect();
}

/*
void Client::networkConnected(uint netid) {
  // TODO: create statusBuffer / switch to networkids
  //BufferInfo id = statusBufferInfo(net);
  //Buffer *b = buffer(id);
  //b->setActive(true);

  Network *netinfo = new Network(netid, this);
  netinfo->setProxy(signalProxy());
  networkModel()->attachNetwork(netinfo);
  connect(netinfo, SIGNAL(destroyed()), this, SLOT(networkDestroyed()));
  _networks[netid] = netinfo;
}

void Client::networkDisconnected(NetworkId networkid) {
  if(!_networks.contains(networkid)) {
    qWarning() << "Client::networkDisconnected(uint): unknown Network" << networkid;
    return;
  }

  Network *net = _networks.take(networkid);
  if(!net->isInitialized()) {
    qDebug() << "Network" << networkid << "disconnected while not yet initialized!";
    updateCoreConnectionProgress();
  }
  net->deleteLater();
}
*/

void Client::addNetwork(Network *net) {
  net->setProxy(signalProxy());
  signalProxy()->synchronize(net);
  networkModel()->attachNetwork(net);
  connect(net, SIGNAL(destroyed()), instance(), SLOT(networkDestroyed()));
  instance()->_networks[net->networkId()] = net;
  emit instance()->networkCreated(net->networkId());
}

void Client::createNetwork(const NetworkInfo &info) {


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
  // FIXME this is not gonna work, net is a QObject here already!
  Network *net = static_cast<Network *>(sender());
  NetworkId networkId = net->networkId();
  if(_networks.contains(networkId))
    _networks.remove(networkId);
}

void Client::recvMessage(const Message &msg) {
  Buffer *b = buffer(msg.buffer());
  b->appendMsg(msg);
  networkModel()->updateBufferActivity(msg);
}

void Client::recvStatusMsg(QString /*net*/, QString /*msg*/) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));
}

void Client::recvBacklogData(BufferInfo id, QVariantList msgs, bool /*done*/) {
  Buffer *b = buffer(id);
  foreach(QVariant v, msgs) {
    Message msg = v.value<Message>();
    b->prependMsg(msg);
    // networkModel()->updateBufferActivity(msg);
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


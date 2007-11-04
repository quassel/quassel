/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "networkinfo.h"
#include "ircuser.h"
#include "ircchannel.h"

#include "message.h"

#include "bufferinfo.h"
#include "buffertreemodel.h"
#include "quasselui.h"
#include "signalproxy.h"
#include "synchronizer.h"
#include "util.h"

QPointer<Client> Client::instanceptr = 0;

// ==============================
//  public Static Methods
// ==============================
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

QList<NetworkInfo *> Client::networkInfos() {
  return instance()->_networkInfo.values();
}

NetworkInfo *Client::networkInfo(uint networkid) {
  if(instance()->_networkInfo.contains(networkid))
    return instance()->_networkInfo[networkid];
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
    connect(buff, SIGNAL(bufferUpdated(Buffer *)),
	    client, SIGNAL(bufferUpdated(Buffer *)));
    connect(buff, SIGNAL(destroyed()),
	    client, SLOT(bufferDestroyed()));
    client->_buffers[id.uid()] = buff;
    emit client->bufferUpdated(buff);
  }
  Q_ASSERT(buff);
  return buff;
}

// FIXME switch to netids!
// WHEN IS THIS NEEDED ANYHOW!?
BufferInfo Client::bufferInfo(QString net, QString buf) {
  foreach(Buffer *buffer_, buffers()) {
    BufferInfo bufferInfo = buffer_->bufferInfo();
    if(bufferInfo.network() == net && bufferInfo.buffer() == buf)
      return bufferInfo;
  }
  Q_ASSERT(false);  // should never happen!
  return BufferInfo();
}

BufferInfo Client::statusBufferInfo(QString net) {
  return bufferInfo(net, "");
}

BufferTreeModel *Client::bufferModel() {
  return instance()->_bufferModel;
}

SignalProxy *Client::signalProxy() {
  return instance()->_signalProxy;
}

// ==============================
//  Constructor / Decon
// ==============================
Client::Client(QObject *parent)
  : QObject(parent),
    socket(0),
    _signalProxy(new SignalProxy(SignalProxy::Client, this)),
    mainUi(0),
    _bufferModel(0),
    connectedToCore(false)
{
}

Client::~Client() {
}

void Client::init() {
  blockSize = 0;

  _bufferModel = new BufferTreeModel(this);

  connect(this, SIGNAL(bufferSelected(Buffer *)),
	  _bufferModel, SLOT(selectBuffer(Buffer *)));
  connect(this, SIGNAL(bufferUpdated(Buffer *)),
	  _bufferModel, SLOT(bufferUpdated(Buffer *)));
  connect(this, SIGNAL(bufferActivity(Buffer::ActivityLevel, Buffer *)),
	  _bufferModel, SLOT(bufferActivity(Buffer::ActivityLevel, Buffer *)));

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

  connect(mainUi, SIGNAL(connectToCore(const QVariantMap &)), this, SLOT(connectToCore(const QVariantMap &)));
  connect(mainUi, SIGNAL(disconnectFromCore()), this, SLOT(disconnectFromCore()));
  connect(this, SIGNAL(connected()), mainUi, SLOT(connectedToCore()));
  connect(this, SIGNAL(disconnected()), mainUi, SLOT(disconnectedFromCore()));

  layoutTimer = new QTimer(this);
  layoutTimer->setInterval(0);
  layoutTimer->setSingleShot(false);
  connect(layoutTimer, SIGNAL(timeout()), this, SLOT(layoutMsg()));

}

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
    connect(sock, SIGNAL(disconnected()), this, SLOT(coreSocketDisconnected()));
    connect(signalProxy(), SIGNAL(disconnected()), this, SLOT(coreSocketDisconnected()));
    //connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(coreSocketStateChanged(QAbstractSocket::SocketState)));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));
    sock->connectToHost(conn["Host"].toString(), conn["Port"].toUInt());
  }
}

void Client::disconnectFromCore() {
  socket->close();
  if(clientMode == LocalCore) {
    coreSocketDisconnected();
  }
}

void Client::coreSocketConnected() {
  connect(this, SIGNAL(recvPartialItem(uint, uint)), this, SIGNAL(coreConnectionProgress(uint, uint)));
  emit coreConnectionMsg(tr("Synchronizing to core..."));
  QVariantMap clientInit;
  clientInit["GuiProtocol"] = GUI_PROTOCOL;
  clientInit["User"] = coreConnectionInfo["User"].toString();
  clientInit["Password"] = coreConnectionInfo["Password"].toString();
  writeDataToDevice(socket, clientInit);
}

void Client::coreSocketDisconnected() {
  instance()->connectedToCore = false;
  emit disconnected();
  socket->deleteLater();
  blockSize = 0;

  /* Clear internal data. Hopefully nothing relies on it at this point. */
  _bufferModel->clear();
  foreach(Buffer *buffer, _buffers.values()) {
    buffer->deleteLater();
  }
  _buffers.clear();

  foreach(NetworkInfo *networkinfo, _networkInfo.values()) {
    networkinfo->deleteLater();
  }
  _networkInfo.clear();

  coreConnectionInfo.clear();
  sessionData.clear();
  layoutQueue.clear();
  layoutTimer->stop();
}

void Client::coreSocketStateChanged(QAbstractSocket::SocketState state) {
  if(state == QAbstractSocket::UnconnectedState) coreSocketDisconnected();
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

  // store Buffer details
  QVariantList coreBuffers = sessionState["Buffers"].toList();
  /* make lookups by id faster */
  foreach(QVariant vid, coreBuffers) {
    buffer(vid.value<BufferInfo>()); // create all buffers, so we see them in the network views
  }

  // create networkInfo objects
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

  int numNets = networkInfos().count();
  int numNetsWaiting = 0;

  int numIrcUsers = 0;
  int numIrcUsersWaiting = 0;

  int numChannels = 0;
  int numChannelsWaiting = 0;

  foreach(NetworkInfo *net, networkInfos()) {
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

  foreach(NetworkInfo *net, networkInfos()) {
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
  if(readDataFromDevice(socket, blockSize, item)) {
    emit recvPartialItem(1,1);
    recvCoreState(item);
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

  NetworkInfo *netinfo = new NetworkInfo(netid, signalProxy(), this);
  connect(netinfo, SIGNAL(initDone()), this, SLOT(updateCoreConnectionProgress()));
  connect(netinfo, SIGNAL(ircUserInitDone()), this, SLOT(updateCoreConnectionProgress()));
  connect(netinfo, SIGNAL(ircChannelInitDone()), this, SLOT(updateCoreConnectionProgress()));
  connect(netinfo, SIGNAL(destroyed()), this, SLOT(networkInfoDestroyed()));
  _networkInfo[netid] = netinfo;
}

void Client::networkDisconnected(uint networkid) {
  foreach(Buffer *buffer, buffers()) {
    if(buffer->bufferInfo().networkId() != networkid)
      continue;

    //buffer->displayMsg(Message(bufferid, Message::Server, tr("Server disconnected."))); FIXME
    buffer->setActive(false);
  }
  
  Q_ASSERT(networkInfo(networkid));
  if(!networkInfo(networkid)->initialized()) {
    qDebug() << "Network" << networkid << "disconnected while not yet initialized!";
    updateCoreConnectionProgress();
  }
}

void Client::updateBufferInfo(BufferInfo id) {
  buffer(id)->updateBufferInfo(id);
}

void Client::bufferDestroyed() {
  Buffer *buffer = static_cast<Buffer *>(sender());
  uint bufferUid = buffer->uid();
  if(_buffers.contains(bufferUid))
    _buffers.remove(bufferUid);
}

void Client::networkInfoDestroyed() {
  NetworkInfo *netinfo = static_cast<NetworkInfo *>(sender());
  uint networkId = netinfo->networkId();
  if(_networkInfo.contains(networkId))
    _networkInfo.remove(networkId);
}

void Client::recvMessage(const Message &msg) {
  Buffer *b = buffer(msg.buffer());

  Buffer::ActivityLevel level = Buffer::OtherActivity;
  if(msg.type() == Message::Plain || msg.type() == Message::Notice){
    level |= Buffer::NewMessage;
  }
  if(msg.flags() & Message::Highlight){
    level |= Buffer::Highlight;
  }
  emit bufferActivity(level, b);

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


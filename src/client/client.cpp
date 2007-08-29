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

#include "buffer.h"
#include "buffertreemodel.h"
//#include "clientproxy.h"
#include "quasselui.h"
#include "signalproxy.h"
#include "util.h"

Client * Client::instanceptr = 0;

bool Client::connectedToCore = false;
Client::ClientMode Client::clientMode;
QVariantMap Client::coreConnectionInfo;
QHash<BufferId, Buffer *> Client::buffers;
QHash<uint, BufferId> Client::bufferIds;
QHash<QString, QHash<QString, QVariantMap> > Client::nicks;
QHash<QString, bool> Client::netConnected;
QStringList Client::netsAwaitingInit;
QHash<QString, QString> Client::ownNick;

Client *Client::instance() {
  if(instanceptr) return instanceptr;
  instanceptr = new Client();
  return instanceptr;
}

void Client::destroy() {
  delete instanceptr;
  instanceptr = 0;
}

Client::Client() {
  //clientProxy = ClientProxy::instance();
  _signalProxy = new SignalProxy(SignalProxy::Client, 0, this);

    // TODO: make this configurable (allow monolithic client to connect to remote cores)
  //if(Global::runMode == Global::Monolithic) clientMode = LocalCore;
  //else clientMode = RemoteCore;
  connectedToCore = false;
  socket = 0;
}

void Client::init(AbstractUi *ui) {
  instance()->mainUi = ui;
  instance()->init();
}

void Client::init() {
  blockSize = 0;

  _bufferModel = new BufferTreeModel(this);

  connect(this, SIGNAL(bufferSelected(Buffer *)), _bufferModel, SLOT(selectBuffer(Buffer *)));
  connect(this, SIGNAL(bufferUpdated(Buffer *)), _bufferModel, SLOT(bufferUpdated(Buffer *)));
  connect(this, SIGNAL(bufferActivity(Buffer::ActivityLevel, Buffer *)), _bufferModel, SLOT(bufferActivity(Buffer::ActivityLevel, Buffer *)));

  //connect(&socket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
  //connect(&socket, SIGNAL(connected()), this, SLOT(coreSocketConnected()));
  //connect(&socket, SIGNAL(disconnected()), this, SLOT(coreSocketDisconnected()));
  //connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(serverError(QAbstractSocket::SocketError)));

  //connect(this, SIGNAL(sendSessionData(const QString &, const QVariant &)), clientProxy, SLOT(gsSessionDataChanged(const QString &, const QVariant &)));
  //connect(clientProxy, SIGNAL(csSessionDataChanged(const QString &, const QVariant &)), this, SLOT(recvSessionData(const QString &, const QVariant &)));

  //connect(clientProxy, SIGNAL(send(ClientSignal, QVariant, QVariant, QVariant)), this, SLOT(recvProxySignal(ClientSignal, QVariant, QVariant, QVariant)));
  //connect(clientProxy, SIGNAL(csCoreState(QVariant)), this, SLOT(recvCoreState(const QVariant &)));
  //connect(clientProxy, SIGNAL(csServerState(QString, QVariant)), this, SLOT(recvNetworkState(QString, QVariant)));
  //connect(clientProxy, SIGNAL(csServerConnected(QString)), this, SLOT(networkConnected(QString)));
  //connect(clientProxy, SIGNAL(csServerDisconnected(QString)), this, SLOT(networkDisconnected(QString)));
  //connect(clientProxy, SIGNAL(csDisplayMsg(Message)), this, SLOT(recvMessage(const Message &)));
  //connect(clientProxy, SIGNAL(csDisplayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  //connect(clientProxy, SIGNAL(csTopicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  //connect(clientProxy, SIGNAL(csNickAdded(QString, QString, QVariantMap)), this, SLOT(addNick(QString, QString, QVariantMap)));
  //connect(clientProxy, SIGNAL(csNickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  //connect(clientProxy, SIGNAL(csNickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  //connect(clientProxy, SIGNAL(csNickUpdated(QString, QString, QVariantMap)), this, SLOT(updateNick(QString, QString, QVariantMap)));
  //connect(clientProxy, SIGNAL(csOwnNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  //connect(clientProxy, SIGNAL(csBacklogData(BufferId, const QList<QVariant> &, bool)), this, SLOT(recvBacklogData(BufferId, QList<QVariant>, bool)));
  //connect(clientProxy, SIGNAL(csUpdateBufferId(BufferId)), this, SLOT(updateBufferId(BufferId)));
  //connect(this, SIGNAL(sendInput(BufferId, QString)), clientProxy, SLOT(gsUserInput(BufferId, QString)));
  //connect(this, SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), clientProxy, SLOT(gsRequestBacklog(BufferId, QVariant, QVariant)));
  //connect(this, SIGNAL(requestNetworkStates()), clientProxy, SLOT(gsRequestNetworkStates()));

  SignalProxy *p = signalProxy();
  p->attachSignal(this, SIGNAL(sendSessionData(const QString &, const QVariant &)), SIGNAL(clientSessionDataChanged(const QString &, const QVariant &)));
  p->attachSlot(SIGNAL(coreSessionDataChanged(const QString &, const QVariant &)), this, SLOT(recvSessionData(const QString &, const QVariant &)));
  p->attachSlot(SIGNAL(coreState(const QVariant &)), this, SLOT(recvCoreState(const QVariant &)));
  p->attachSlot(SIGNAL(networkState(QString, QVariant)), this, SLOT(recvNetworkState(QString, QVariant)));
  p->attachSlot(SIGNAL(networkConnected(QString)), this, SLOT(networkConnected(QString)));
  p->attachSlot(SIGNAL(networkDisconnected(QString)), this, SLOT(networkDisconnected(QString)));
  p->attachSlot(SIGNAL(displayMsg(const Message &)), this, SLOT(recvMessage(const Message &)));
  p->attachSlot(SIGNAL(displayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  p->attachSlot(SIGNAL(topicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  p->attachSlot(SIGNAL(nickAdded(QString, QString, QVariantMap)), this, SLOT(addNick(QString, QString, QVariantMap)));
  p->attachSlot(SIGNAL(nickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  p->attachSlot(SIGNAL(nickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  p->attachSlot(SIGNAL(nickUpdated(QString, QString, QVariantMap)), this, SLOT(updateNick(QString, QString, QVariantMap)));
  p->attachSlot(SIGNAL(ownNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  p->attachSlot(SIGNAL(backlogData(BufferId, const QVariantList &, bool)), this, SLOT(recvBacklogData(BufferId, const QVariantList &, bool)));
  p->attachSlot(SIGNAL(bufferIdUpdated(BufferId)), this, SLOT(updateBufferId(BufferId)));
  p->attachSignal(this, SIGNAL(sendInput(BufferId, QString)));
  //p->attachSignal(this, SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), "requestBacklog");
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

Client::~Client() {
  //delete mainUi;
  //delete _bufferModel;
  foreach(Buffer *buf, buffers.values()) delete buf; // this is done by disconnectFromCore()! FIXME?
  //ClientProxy::destroy();
  Q_ASSERT(!buffers.count());
}

BufferTreeModel *Client::bufferModel() {
  return instance()->_bufferModel;
}

SignalProxy *Client::signalProxy() {
  return instance()->_signalProxy;
}

bool Client::isConnected() {
  return connectedToCore;
}

void Client::connectToCore(const QVariantMap &conn) {
  // TODO implement SSL
  coreConnectionInfo = conn;
  if(isConnected() || socket != 0) {
    emit coreConnectionError(tr("Already connected to Core!"));
    return;
  }
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
    connect(signalProxy(), SIGNAL(peerDisconnected()), this, SLOT(coreSocketDisconnected()));
    //connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(coreSocketStateChanged(QAbstractSocket::SocketState)));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));
    sock->connectToHost(conn["Host"].toString(), conn["Port"].toUInt());
  }
}

void Client::disconnectFromCore() {
  if(clientMode == RemoteCore) {
    socket->close();
    //QAbstractSocket *sock = qobject_cast<QAbstractSocket*>(socket);
    //Q_ASSERT(sock);
    //sock->disconnectFromHost();
  } else {
    socket->close();
    //disconnectFromLocalCore();
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
  connectedToCore = false;
  emit disconnected();
  socket->deleteLater();

  /* Clear internal data. Hopefully nothing relies on it at this point. */
  _bufferModel->clear();
  // Buffers, if deleted, send a signal that causes their removal from buffers and bufferIds.
  // So we cannot simply go through the array in a loop (or use qDeleteAll) for deletion...
  while(buffers.count()) { delete buffers.take(buffers.keys()[0]); }
  Q_ASSERT(!buffers.count());   // should be empty now!
  Q_ASSERT(!bufferIds.count());
  coreConnectionInfo.clear();
  sessionData.clear();
  nicks.clear();
  netConnected.clear();
  netsAwaitingInit.clear();
  ownNick.clear();
  layoutQueue.clear();
  layoutTimer->stop();
}

void Client::coreSocketStateChanged(QAbstractSocket::SocketState state) { qDebug() << state;
  if(state == QAbstractSocket::UnconnectedState) coreSocketDisconnected();
}

void Client::recvCoreState(const QVariant &state) {
  disconnect(this, SIGNAL(recvPartialItem(uint, uint)), this, SIGNAL(coreConnectionProgress(uint, uint)));
  disconnect(socket, 0, this, 0);  // rest of communication happens through SignalProxy
  signalProxy()->addPeer(socket);
  syncToCore(state);
}

void Client::syncToCore(const QVariant &coreState) {
  if(!coreState.toMap().contains("SessionState")) {
    emit coreConnectionError(tr("Invalid data received from core!"));
    disconnectFromCore();
    return;
  }
  QVariantMap sessionState = coreState.toMap()["SessionState"].toMap();
  QVariantMap sessData = sessionState["SessionData"].toMap();

  foreach(QString key, sessData.keys()) {
    recvSessionData(key, sessData[key]);
  }
  QList<QVariant> coreBuffers = sessionState["Buffers"].toList();
  /* make lookups by id faster */
  foreach(QVariant vid, coreBuffers) {
    BufferId id = vid.value<BufferId>();
    bufferIds[id.uid()] = id;  // make lookups by id faster
    buffer(id);                // create all buffers, so we see them in the network views
  }
  netsAwaitingInit = sessionState["Networks"].toStringList();
  connectedToCore = true;
  if(netsAwaitingInit.count()) {
    emit coreConnectionMsg(tr("Requesting network states..."));
    emit coreConnectionProgress(0, netsAwaitingInit.count());
    emit requestNetworkStates();
  }
  else {
    emit coreConnectionProgress(1, 1);
    emit connected();
  }
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

void Client::recvProxySignal(ClientSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  if(clientMode == LocalCore) return;
  QList<QVariant> sigdata;
  sigdata.append(sig); sigdata.append(arg1); sigdata.append(arg2); sigdata.append(arg3);
  //qDebug() << "Sending signal: " << sigdata;
  writeDataToDevice(socket, QVariant(sigdata));
}

void Client::coreSocketError(QAbstractSocket::SocketError) {
  emit coreConnectionError(socket->errorString());
}

void Client::coreHasData() {
  QVariant item;
  if(readDataFromDevice(socket, blockSize, item)) {
    emit recvPartialItem(1,1);
    //QList<QVariant> sigdata = item.toList();
    //Q_ASSERT(sigdata.size() == 4);
    //ClientProxy::instance()->recv((CoreSignal)sigdata[0].toInt(), sigdata[1], sigdata[2], sigdata[3]);
    recvCoreState(item);
    blockSize = 0;
    return;
  }
  if(blockSize > 0) {
    emit recvPartialItem(socket->bytesAvailable(), blockSize);
  }
}

void Client::networkConnected(QString net) {
  Q_ASSERT(!netsAwaitingInit.contains(net));
  netConnected[net] = true;
  BufferId id = statusBufferId(net);
  Buffer *b = buffer(id);
  b->setActive(true);
  //b->displayMsg(Message(id, Message::Server, tr("Connected.")));
  // TODO buffersUpdated();
}

void Client::networkDisconnected(QString net) {
  foreach(BufferId id, buffers.keys()) {
    if(id.network() != net) continue;
    Buffer *b = buffer(id);
    //b->displayMsg(Message(id, Message::Server, tr("Server disconnected."))); FIXME
    b->setActive(false);
  }
  netConnected[net] = false;
  if(netsAwaitingInit.contains(net)) {
    qDebug() << "Network" << net << "disconnected while not yet initialized!";
    netsAwaitingInit.removeAll(net);
    emit coreConnectionProgress(netConnected.count(), netConnected.count() + netsAwaitingInit.count());
    if(!netsAwaitingInit.count()) emit connected();
  }
}

void Client::updateBufferId(BufferId id) {
  bufferIds[id.uid()] = id;  // make lookups by id faster
  buffer(id);
}

BufferId Client::bufferId(QString net, QString buf) {
  foreach(BufferId id, buffers.keys()) {
    if(id.network() == net && id.buffer() == buf) return id;
  }
  Q_ASSERT(false);  // should never happen!
  return BufferId();
}

BufferId Client::statusBufferId(QString net) {
  return bufferId(net, "");
}


Buffer * Client::buffer(BufferId id) {
  Client *client = Client::instance();
  if(!buffers.contains(id)) {
    Buffer *b = new Buffer(id);
    b->setOwnNick(ownNick[id.network()]);
    connect(b, SIGNAL(userInput(BufferId, QString)), client, SLOT(userInput(BufferId, QString)));
    connect(b, SIGNAL(bufferUpdated(Buffer *)), client, SIGNAL(bufferUpdated(Buffer *)));
    connect(b, SIGNAL(bufferDestroyed(Buffer *)), client, SIGNAL(bufferDestroyed(Buffer *)));
    connect(b, SIGNAL(bufferDestroyed(Buffer *)), client, SLOT(removeBuffer(Buffer *)));
    buffers[id] = b;
    emit client->bufferUpdated(b);
  }
  return buffers[id];
}

QList<BufferId> Client::allBufferIds() {
  return buffers.keys();
}

void Client::removeBuffer(Buffer *b) {
  buffers.remove(b->bufferId());
  bufferIds.remove(b->bufferId().uid());
}

void Client::recvNetworkState(QString net, QVariant state) {
  netsAwaitingInit.removeAll(net);
  netConnected[net] = true;
  setOwnNick(net, state.toMap()["OwnNick"].toString());
  buffer(statusBufferId(net))->setActive(true);
  QVariantMap t = state.toMap()["Topics"].toMap();
  QVariantMap n = state.toMap()["Nicks"].toMap();
  foreach(QVariant v, t.keys()) {
    QString buf = v.toString();
    BufferId id = bufferId(net, buf);
    buffer(id)->setActive(true);
    setTopic(net, buf, t[buf].toString());
  }
  foreach(QString nick, n.keys()) {
    addNick(net, nick, n[nick].toMap());
  }
  emit coreConnectionProgress(netConnected.count(), netConnected.count() + netsAwaitingInit.count());
  if(!netsAwaitingInit.count()) emit connected();
}

void Client::recvMessage(const Message &msg) {
  Buffer *b = buffer(msg.buffer);

  Buffer::ActivityLevel level = Buffer::OtherActivity;
  if(msg.type == Message::Plain || msg.type == Message::Notice){
    level |= Buffer::NewMessage;
  }
  if(msg.flags & Message::Highlight){
    level |= Buffer::Highlight;
  }
  emit bufferActivity(level, b);

  b->appendMsg(msg);
}

void Client::recvStatusMsg(QString /*net*/, QString /*msg*/) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));

}

void Client::recvBacklogData(BufferId id, QVariantList msgs, bool /*done*/) {
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
    if(b->layoutMsg()) layoutQueue.append(b);  // Buffer has more messages in its queue --> Round Robin
  }
  if(!layoutQueue.count()) layoutTimer->stop();
}

AbstractUiMsg *Client::layoutMsg(const Message &msg) {
  return instance()->mainUi->layoutMsg(msg);
}

void Client::userInput(BufferId id, QString msg) {
  emit sendInput(id, msg);
}

void Client::setTopic(QString net, QString buf, QString topic) {
  BufferId id = bufferId(net, buf);
  if(!netConnected[id.network()]) return;
  Buffer *b = buffer(id);
  b->setTopic(topic);
  //if(!b->isActive()) {
  //  b->setActive(true);
  //  buffersUpdated();
  //}
}

void Client::addNick(QString net, QString nick, QVariantMap props) {
  if(!netConnected[net]) return;
  nicks[net][nick] = props;
  QVariantMap chans = props["Channels"].toMap();
  QStringList c = chans.keys();
  foreach(QString bufname, c) {
    buffer(bufferId(net, bufname))->addNick(nick, props);
  }
}

void Client::renameNick(QString net, QString oldnick, QString newnick) {
  if(!netConnected[net]) return;
  QStringList chans = nicks[net][oldnick]["Channels"].toMap().keys();
  foreach(QString c, chans) {
    buffer(bufferId(net, c))->renameNick(oldnick, newnick);
  }
  nicks[net][newnick] = nicks[net].take(oldnick);
}

void Client::updateNick(QString net, QString nick, QVariantMap props) {
  if(!netConnected[net]) return;
  QStringList oldchans = nicks[net][nick]["Channels"].toMap().keys();
  QStringList newchans = props["Channels"].toMap().keys();
  foreach(QString c, newchans) {
    if(oldchans.contains(c)) buffer(bufferId(net, c))->updateNick(nick, props);
    else buffer(bufferId(net, c))->addNick(nick, props);
  }
  foreach(QString c, oldchans) {
    if(!newchans.contains(c)) buffer(bufferId(net, c))->removeNick(nick);
  }
  nicks[net][nick] = props;
}

void Client::removeNick(QString net, QString nick) {
  if(!netConnected[net]) return;
  QVariantMap chans = nicks[net][nick]["Channels"].toMap();
  foreach(QString bufname, chans.keys()) {
    buffer(bufferId(net, bufname))->removeNick(nick);
  }
  nicks[net].remove(nick);
}

void Client::setOwnNick(QString net, QString nick) {
  if(!netConnected[net]) return;
  ownNick[net] = nick;
  foreach(BufferId id, buffers.keys()) {
    if(id.network() == net) {
      buffers[id]->setOwnNick(nick);
    }
  }
}


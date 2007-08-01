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
#include "clientproxy.h"
#include "quasselui.h"
#include "util.h"

Client * Client::instanceptr = 0;

bool Client::connectedToCore = false;
Client::ClientMode Client::clientMode;
VarMap Client::coreConnectionInfo;
QHash<BufferId, Buffer *> Client::buffers;
QHash<uint, BufferId> Client::bufferIds;
QHash<QString, QHash<QString, VarMap> > Client::nicks;
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
  clientProxy = ClientProxy::instance();

  _bufferModel = new BufferTreeModel(this);

  connect(this, SIGNAL(bufferSelected(Buffer *)), _bufferModel, SLOT(selectBuffer(Buffer *)));
  connect(this, SIGNAL(bufferUpdated(Buffer *)), _bufferModel, SLOT(bufferUpdated(Buffer *)));
  connect(this, SIGNAL(bufferActivity(Buffer::ActivityLevel, Buffer *)), _bufferModel, SLOT(bufferActivity(Buffer::ActivityLevel, Buffer *)));

    // TODO: make this configurable (allow monolithic client to connect to remote cores)
  if(Global::runMode == Global::Monolithic) clientMode = LocalCore;
  else clientMode = RemoteCore;
  connectedToCore = false;
}

void Client::init(AbstractUi *ui) {
  instance()->mainUi = ui;
  instance()->init();
}

void Client::init() {
  blockSize = 0;

  connect(&socket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
  connect(&socket, SIGNAL(connected()), this, SLOT(coreConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(coreDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(serverError(QAbstractSocket::SocketError)));

  connect(Global::instance(), SIGNAL(dataPutLocally(UserId, QString)), this, SLOT(updateCoreData(UserId, QString)));
  connect(clientProxy, SIGNAL(csUpdateGlobalData(QString, QVariant)), this, SLOT(updateLocalData(QString, QVariant)));
  connect(this, SIGNAL(sendSessionData(const QString &, const QVariant &)), clientProxy, SLOT(gsSessionDataChanged(const QString &, const QVariant &)));
  connect(clientProxy, SIGNAL(csSessionDataChanged(const QString &, const QVariant &)), this, SLOT(recvSessionData(const QString &, const QVariant &)));

  connect(clientProxy, SIGNAL(send(ClientSignal, QVariant, QVariant, QVariant)), this, SLOT(recvProxySignal(ClientSignal, QVariant, QVariant, QVariant)));
  connect(clientProxy, SIGNAL(csServerState(QString, QVariant)), this, SLOT(recvNetworkState(QString, QVariant)));
  connect(clientProxy, SIGNAL(csServerConnected(QString)), this, SLOT(networkConnected(QString)));
  connect(clientProxy, SIGNAL(csServerDisconnected(QString)), this, SLOT(networkDisconnected(QString)));
  connect(clientProxy, SIGNAL(csDisplayMsg(Message)), this, SLOT(recvMessage(const Message &)));
  connect(clientProxy, SIGNAL(csDisplayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  connect(clientProxy, SIGNAL(csTopicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  connect(clientProxy, SIGNAL(csNickAdded(QString, QString, VarMap)), this, SLOT(addNick(QString, QString, VarMap)));
  connect(clientProxy, SIGNAL(csNickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  connect(clientProxy, SIGNAL(csNickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  connect(clientProxy, SIGNAL(csNickUpdated(QString, QString, VarMap)), this, SLOT(updateNick(QString, QString, VarMap)));
  connect(clientProxy, SIGNAL(csOwnNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  connect(clientProxy, SIGNAL(csBacklogData(BufferId, const QList<QVariant> &, bool)), this, SLOT(recvBacklogData(BufferId, QList<QVariant>, bool)));
  connect(clientProxy, SIGNAL(csUpdateBufferId(BufferId)), this, SLOT(updateBufferId(BufferId)));
  connect(this, SIGNAL(sendInput(BufferId, QString)), clientProxy, SLOT(gsUserInput(BufferId, QString)));
  connect(this, SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), clientProxy, SLOT(gsRequestBacklog(BufferId, QVariant, QVariant)));
  connect(this, SIGNAL(requestNetworkStates()), clientProxy, SLOT(gsRequestNetworkStates()));

  connect(mainUi, SIGNAL(connectToCore(const VarMap &)), this, SLOT(connectToCore(const VarMap &)));
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
  foreach(Buffer *buf, buffers.values()) delete buf;
  ClientProxy::destroy();

}

BufferTreeModel *Client::bufferModel() {
  return instance()->_bufferModel;
}

bool Client::isConnected() { return connectedToCore; }

void Client::connectToCore(const VarMap &conn) {
  // TODO implement SSL
  coreConnectionInfo = conn;
  if(isConnected()) {
    emit coreConnectionError(tr("Already connected to Core!"));
    return;
  }
  if(conn["Host"].toString().isEmpty()) {
    clientMode = LocalCore;
    syncToCore();
  } else {
    clientMode = RemoteCore;
    emit coreConnectionMsg(tr("Connecting..."));
    socket.connectToHost(conn["Host"].toString(), conn["Port"].toUInt());
  }
}

void Client::disconnectFromCore() {
  if(clientMode == RemoteCore) {
    socket.close();
  } else {
    disconnectFromLocalCore();
    coreDisconnected();
  }
  /* Clear internal data. Hopefully nothing relies on it at this point. */
  coreConnectionInfo.clear();
  sessionData.clear();
  //foreach(Buffer *buf, buffers.values()) delete buf;
  qDebug() << "barfoo";
  _bufferModel->clear();
  //qDeleteAll(buffers);
  qDebug() << "foobar";
}

void Client::coreConnected() {
  syncToCore();

}

void Client::coreDisconnected() {
  connectedToCore = false;
  emit disconnected();
}

void Client::syncToCore() {
  VarMap state;
  if(clientMode == LocalCore) {
    state = connectToLocalCore(coreConnectionInfo["User"].toString(), coreConnectionInfo["Password"].toString()).toMap();
  } else {
    // TODO connect to remote cores
  }

  VarMap data = state["CoreData"].toMap();
  foreach(QString key, data.keys()) {
    Global::updateData(key, data[key]);
  }
  //if(!Global::data("CoreReady").toBool()) {
  //  qFatal("Something is wrong, getting invalid data from core!");
  //}

  VarMap sessionState = state["SessionState"].toMap();
  VarMap sessData = sessionState["SessionData"].toMap();
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

void Client::updateCoreData(UserId, QString key) {
  if(clientMode == LocalCore) return;
  QVariant data = Global::data(key);
  recvProxySignal(GS_UPDATE_GLOBAL_DATA, key, data, QVariant());
}

void Client::updateLocalData(QString key, QVariant data) {
  Global::updateData(key, data);
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
  writeDataToDevice(&socket, QVariant(sigdata));
}

void Client::serverError(QAbstractSocket::SocketError) {
  emit coreConnectionError(socket.errorString());
}

void Client::serverHasData() {
  QVariant item;
  while(readDataFromDevice(&socket, blockSize, item)) {
    emit recvPartialItem(1,1);
    QList<QVariant> sigdata = item.toList();
    Q_ASSERT(sigdata.size() == 4);
    ClientProxy::instance()->recv((CoreSignal)sigdata[0].toInt(), sigdata[1], sigdata[2], sigdata[3]);
    blockSize = 0;
  }
  if(blockSize > 0) {
    emit recvPartialItem(socket.bytesAvailable(), blockSize);
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
    buffers[id] = b;
    emit client->bufferUpdated(b);
  }
  return buffers[id];
}

QList<BufferId> Client::allBufferIds() {
  return buffers.keys();
}

void Client::recvNetworkState(QString net, QVariant state) {
  netsAwaitingInit.removeAll(net);
  netConnected[net] = true;
  setOwnNick(net, state.toMap()["OwnNick"].toString());
  buffer(statusBufferId(net))->setActive(true);
  VarMap t = state.toMap()["Topics"].toMap();
  VarMap n = state.toMap()["Nicks"].toMap();
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

void Client::recvBacklogData(BufferId id, const QList<QVariant> &msgs, bool /*done*/) {
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

void Client::addNick(QString net, QString nick, VarMap props) {
  if(!netConnected[net]) return;
  nicks[net][nick] = props;
  VarMap chans = props["Channels"].toMap();
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

void Client::updateNick(QString net, QString nick, VarMap props) {
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
  VarMap chans = nicks[net][nick]["Channels"].toMap();
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


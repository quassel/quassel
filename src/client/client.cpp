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
#include "clientproxy.h"
#include "mainwin.h"
#include "buffer.h"
#include "util.h"

Client * Client::instanceptr = 0;

Client::ClientMode Client::clientMode;
QHash<BufferId, Buffer *> Client::buffers;
QHash<uint, BufferId> Client::bufferIds;
QHash<QString, QHash<QString, VarMap> > Client::nicks;
QHash<QString, bool> Client::connected;
QHash<QString, QString> Client::ownNick;
QList<BufferId> Client::coreBuffers;


Client *Client::instance() {
  if(instanceptr) return instanceptr;
  instanceptr = new Client();
  instanceptr->init();
  return instanceptr;
}

void Client::destroy() {
  delete instanceptr;
  instanceptr = 0;
}

Client::Client() {
  clientProxy = ClientProxy::instance();

  //mainWin = new MainWin();
  _bufferModel = new BufferTreeModel(0);  // FIXME

  connect(this, SIGNAL(bufferSelected(Buffer *)), _bufferModel, SLOT(selectBuffer(Buffer *)));
  connect(this, SIGNAL(bufferUpdated(Buffer *)), _bufferModel, SLOT(bufferUpdated(Buffer *)));
  connect(this, SIGNAL(bufferActivity(Buffer::ActivityLevel, Buffer *)), _bufferModel, SLOT(bufferActivity(Buffer::ActivityLevel, Buffer *)));

    // TODO: make this configurable (allow monolithic client to connect to remote cores)
  if(Global::runMode == Global::Monolithic) clientMode = LocalCore;
  else clientMode = RemoteCore;
}

void Client::init() {
  blockSize = 0;

  connect(&socket, SIGNAL(readyRead()), this, SLOT(serverHasData()));
  connect(&socket, SIGNAL(connected()), this, SLOT(coreConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(coreDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(serverError(QAbstractSocket::SocketError)));

  connect(Global::instance(), SIGNAL(dataPutLocally(UserId, QString)), this, SLOT(updateCoreData(UserId, QString)));
  connect(clientProxy, SIGNAL(csUpdateGlobalData(QString, QVariant)), this, SLOT(updateLocalData(QString, QVariant)));

  connect(clientProxy, SIGNAL(send(ClientSignal, QVariant, QVariant, QVariant)), this, SLOT(recvProxySignal(ClientSignal, QVariant, QVariant, QVariant)));
  connect(clientProxy, SIGNAL(csServerState(QString, QVariant)), this, SLOT(recvNetworkState(QString, QVariant)));
  connect(clientProxy, SIGNAL(csServerConnected(QString)), this, SLOT(networkConnected(QString)));
  connect(clientProxy, SIGNAL(csServerDisconnected(QString)), this, SLOT(networkDisconnected(QString)));
  connect(clientProxy, SIGNAL(csDisplayMsg(Message)), this, SLOT(recvMessage(Message)));
  connect(clientProxy, SIGNAL(csDisplayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));
  connect(clientProxy, SIGNAL(csTopicSet(QString, QString, QString)), this, SLOT(setTopic(QString, QString, QString)));
  connect(clientProxy, SIGNAL(csNickAdded(QString, QString, VarMap)), this, SLOT(addNick(QString, QString, VarMap)));
  connect(clientProxy, SIGNAL(csNickRemoved(QString, QString)), this, SLOT(removeNick(QString, QString)));
  connect(clientProxy, SIGNAL(csNickRenamed(QString, QString, QString)), this, SLOT(renameNick(QString, QString, QString)));
  connect(clientProxy, SIGNAL(csNickUpdated(QString, QString, VarMap)), this, SLOT(updateNick(QString, QString, VarMap)));
  connect(clientProxy, SIGNAL(csOwnNickSet(QString, QString)), this, SLOT(setOwnNick(QString, QString)));
  connect(clientProxy, SIGNAL(csBacklogData(BufferId, QList<QVariant>, bool)), this, SLOT(recvBacklogData(BufferId, QList<QVariant>, bool)));
  connect(clientProxy, SIGNAL(csUpdateBufferId(BufferId)), this, SLOT(updateBufferId(BufferId)));
  connect(this, SIGNAL(sendInput(BufferId, QString)), clientProxy, SLOT(gsUserInput(BufferId, QString)));
  connect(this, SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), clientProxy, SLOT(gsRequestBacklog(BufferId, QVariant, QVariant)));

  syncToCore();

  layoutTimer = new QTimer(this);
  layoutTimer->setInterval(0);
  layoutTimer->setSingleShot(false);
  connect(layoutTimer, SIGNAL(timeout()), this, SLOT(layoutMsg()));

  /* make lookups by id faster */
  foreach(BufferId id, coreBuffers) {
    bufferIds[id.uid()] = id;  // make lookups by id faster
    buffer(id);                // create all buffers, so we see them in the network views
    emit requestBacklog(id, -1, -1);  // TODO: use custom settings for backlog request
  }

  mainWin = new MainWin();
  mainWin->init();

}

Client::~Client() {
  delete mainWin;
  delete _bufferModel;
  foreach(Buffer *buf, buffers.values()) delete buf;
  ClientProxy::destroy();

}

BufferTreeModel *Client::bufferModel() {
  return instance()->_bufferModel;
}

void Client::coreConnected() {

}

void Client::coreDisconnected() {

}

void Client::updateCoreData(UserId, QString key) {
  if(clientMode == LocalCore) return;
  QVariant data = Global::data(key);
  recvProxySignal(GS_UPDATE_GLOBAL_DATA, key, data, QVariant());
}

void Client::updateLocalData(QString key, QVariant data) {
  Global::updateData(key, data);
}

void Client::recvProxySignal(ClientSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  if(clientMode == LocalCore) return;
  QList<QVariant> sigdata;
  sigdata.append(sig); sigdata.append(arg1); sigdata.append(arg2); sigdata.append(arg3);
  //qDebug() << "Sending signal: " << sigdata;
  writeDataToDevice(&socket, QVariant(sigdata));
}

void Client::connectToCore(QString host, quint16 port) {
  // TODO implement SSL
  socket.connectToHost(host, port);
}

void Client::disconnectFromCore() {
  socket.close();
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

/*******************************************************************************************************************/

void Client::networkConnected(QString net) {
  connected[net] = true;
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
  connected[net] = false;
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

void Client::recvNetworkState(QString net, QVariant state) {
  connected[net] = true;
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
}

void Client::recvMessage(Message msg) {
  Buffer *b = buffer(msg.buffer);

  Buffer::ActivityLevel level = Buffer::OtherActivity;
  if(msg.type == Message::Plain || msg.type == Message::Notice){
    level |= Buffer::NewMessage;
  }
  if(msg.flags & Message::Highlight){
    level |= Buffer::Highlight;
  }
  emit bufferActivity(level, b);

  //b->displayMsg(msg);
  b->appendChatLine(new ChatLine(msg));
}

void Client::recvStatusMsg(QString net, QString msg) {
  //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));

}

void Client::recvBacklogData(BufferId id, QList<QVariant> msgs, bool done) {
  foreach(QVariant v, msgs) {
    layoutQueue.append(v.value<Message>());
  }
  if(!layoutTimer->isActive()) layoutTimer->start();
}


void Client::layoutMsg() {
  if(layoutQueue.count()) {
    ChatLine *line = new ChatLine(layoutQueue.takeFirst());
    buffer(line->bufferId())->prependChatLine(line);
  }
  if(!layoutQueue.count()) layoutTimer->stop();
}

void Client::userInput(BufferId id, QString msg) {
  emit sendInput(id, msg);
}

void Client::setTopic(QString net, QString buf, QString topic) {
  BufferId id = bufferId(net, buf);
  if(!connected[id.network()]) return;
  Buffer *b = buffer(id);
  b->setTopic(topic);
  //if(!b->isActive()) {
  //  b->setActive(true);
  //  buffersUpdated();
  //}
}

void Client::addNick(QString net, QString nick, VarMap props) {
  if(!connected[net]) return;
  nicks[net][nick] = props;
  VarMap chans = props["Channels"].toMap();
  QStringList c = chans.keys();
  foreach(QString bufname, c) {
    buffer(bufferId(net, bufname))->addNick(nick, props);
  }
}

void Client::renameNick(QString net, QString oldnick, QString newnick) {
  if(!connected[net]) return;
  QStringList chans = nicks[net][oldnick]["Channels"].toMap().keys();
  foreach(QString c, chans) {
    buffer(bufferId(net, c))->renameNick(oldnick, newnick);
  }
  nicks[net][newnick] = nicks[net].take(oldnick);
}

void Client::updateNick(QString net, QString nick, VarMap props) {
  if(!connected[net]) return;
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
  if(!connected[net]) return;
  VarMap chans = nicks[net][nick]["Channels"].toMap();
  foreach(QString bufname, chans.keys()) {
    buffer(bufferId(net, bufname))->removeNick(nick);
  }
  nicks[net].remove(nick);
}

void Client::setOwnNick(QString net, QString nick) {
  if(!connected[net]) return;
  ownNick[net] = nick;
  foreach(BufferId id, buffers.keys()) {
    if(id.network() == net) {
      buffers[id]->setOwnNick(nick);
    }
  }
}



/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#include "coresession.h"
#include "server.h"
#include "storage.h"
#include "util.h"

CoreSession::CoreSession(UserId uid, Storage *_storage) : user(uid), storage(_storage) {
  coreProxy = new CoreProxy();

  QSettings s;
  s.beginGroup(QString("SessionData/%1").arg(user));
  mutex.lock();
  foreach(QString key, s.allKeys()) {
    sessionData[key] = s.value(key);
  }
  mutex.unlock();

  connect(coreProxy, SIGNAL(send(CoreSignal, QVariant, QVariant, QVariant)), this, SIGNAL(proxySignal(CoreSignal, QVariant, QVariant, QVariant)));
  connect(coreProxy, SIGNAL(requestServerStates()), this, SIGNAL(serverStateRequested()));
  connect(coreProxy, SIGNAL(gsRequestConnect(QStringList)), this, SLOT(connectToIrc(QStringList)));
  connect(coreProxy, SIGNAL(gsUserInput(BufferId, QString)), this, SLOT(msgFromGui(BufferId, QString)));
  connect(coreProxy, SIGNAL(gsImportBacklog()), storage, SLOT(importOldBacklog()));
  connect(coreProxy, SIGNAL(gsRequestBacklog(BufferId, QVariant, QVariant)), this, SLOT(sendBacklog(BufferId, QVariant, QVariant)));
  connect(coreProxy, SIGNAL(gsRequestNetworkStates()), this, SLOT(sendServerStates()));
  connect(this, SIGNAL(displayMsg(Message)), coreProxy, SLOT(csDisplayMsg(Message)));
  connect(this, SIGNAL(displayStatusMsg(QString, QString)), coreProxy, SLOT(csDisplayStatusMsg(QString, QString)));
  connect(this, SIGNAL(backlogData(BufferId, QList<QVariant>, bool)), coreProxy, SLOT(csBacklogData(BufferId, QList<QVariant>, bool)));
  connect(this, SIGNAL(bufferIdUpdated(BufferId)), coreProxy, SLOT(csUpdateBufferId(BufferId)));
  connect(storage, SIGNAL(bufferIdUpdated(BufferId)), coreProxy, SLOT(csUpdateBufferId(BufferId)));
  connect(Global::instance(), SIGNAL(dataUpdatedRemotely(UserId, QString)), this, SLOT(globalDataUpdated(UserId, QString)));
  connect(Global::instance(), SIGNAL(dataPutLocally(UserId, QString)), this, SLOT(globalDataUpdated(UserId, QString)));
  connect(this, SIGNAL(sessionDataChanged(const QString &, const QVariant &)), coreProxy, SLOT(csSessionDataChanged(const QString &, const QVariant &)));
  connect(coreProxy, SIGNAL(gsSessionDataChanged(const QString &, const QVariant &)), this, SLOT(storeSessionData(const QString &, const QVariant &)));
}

CoreSession::~CoreSession() {

}

UserId CoreSession::userId() const {
  return user;
}

void CoreSession::processSignal(ClientSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  coreProxy->recv(sig, arg1, arg2, arg3);
}

void CoreSession::globalDataUpdated(UserId uid, QString key) {
  Q_ASSERT(uid == userId());
  QVariant data = Global::data(userId(), key);
  QSettings s;
  s.setValue(QString("Global/%1/").arg(userId())+key, data);
}

void CoreSession::storeSessionData(const QString &key, const QVariant &data) {
  QSettings s;
  s.beginGroup(QString("SessionData/%1").arg(user));
  mutex.lock();
  sessionData[key] = data;
  s.setValue(key, data);
  mutex.unlock();
  s.endGroup();
  emit sessionDataChanged(key, data);
  emit sessionDataChanged(key);
}

QVariant CoreSession::retrieveSessionData(const QString &key, const QVariant &def) {
  QVariant data;
  mutex.lock();
  if(!sessionData.contains(key)) data = def;
  else data = sessionData[key];
  mutex.unlock();
  return data;
}

void CoreSession::connectToIrc(QStringList networks) {
  foreach(QString net, networks) {
    if(servers.contains(net)) {

    } else {
      Server *server = new Server(userId(), net);
      connect(this, SIGNAL(serverStateRequested()), server, SLOT(sendState()));
      connect(this, SIGNAL(connectToIrc(QString)), server, SLOT(connectToIrc(QString)));
      connect(this, SIGNAL(disconnectFromIrc(QString)), server, SLOT(disconnectFromIrc(QString)));
      connect(this, SIGNAL(msgFromGui(QString, QString, QString)), server, SLOT(userInput(QString, QString, QString)));

      connect(server, SIGNAL(connected(QString)), this, SLOT(serverConnected(QString)));
      connect(server, SIGNAL(disconnected(QString)), this, SLOT(serverDisconnected(QString)));

      connect(server, SIGNAL(serverState(QString, VarMap)), coreProxy, SLOT(csServerState(QString, VarMap)));
      //connect(server, SIGNAL(displayMsg(Message)), this, SLOT(recvMessageFromServer(Message)));
      connect(server, SIGNAL(displayMsg(Message::Type, QString, QString, QString, quint8)), this, SLOT(recvMessageFromServer(Message::Type, QString, QString, QString, quint8)));
      connect(server, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));
      connect(server, SIGNAL(modeSet(QString, QString, QString)), coreProxy, SLOT(csModeSet(QString, QString, QString)));
      connect(server, SIGNAL(topicSet(QString, QString, QString)), coreProxy, SLOT(csTopicSet(QString, QString, QString)));
      connect(server, SIGNAL(nickAdded(QString, QString, VarMap)), coreProxy, SLOT(csNickAdded(QString, QString, VarMap)));
      connect(server, SIGNAL(nickRenamed(QString, QString, QString)), coreProxy, SLOT(csNickRenamed(QString, QString, QString)));
      connect(server, SIGNAL(nickRemoved(QString, QString)), coreProxy, SLOT(csNickRemoved(QString, QString)));
      connect(server, SIGNAL(nickUpdated(QString, QString, VarMap)), coreProxy, SLOT(csNickUpdated(QString, QString, VarMap)));
      connect(server, SIGNAL(ownNickSet(QString, QString)), coreProxy, SLOT(csOwnNickSet(QString, QString)));
      connect(server, SIGNAL(queryRequested(QString, QString)), coreProxy, SLOT(csQueryRequested(QString, QString)));
      // TODO add error handling
      connect(server, SIGNAL(connected(QString)), coreProxy, SLOT(csServerConnected(QString)));
      connect(server, SIGNAL(disconnected(QString)), coreProxy, SLOT(csServerDisconnected(QString)));

      server->start();
      servers[net] = server;
    }
    emit connectToIrc(net);
  }
}

void CoreSession::serverConnected(QString net) {
  storage->getBufferId(userId(), net); // create status buffer
}

void CoreSession::serverDisconnected(QString net) {
  delete servers[net];
  servers.remove(net);
  coreProxy->csServerDisconnected(net);
}

void CoreSession::msgFromGui(BufferId bufid, QString msg) {
  emit msgFromGui(bufid.network(), bufid.buffer(), msg);
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.

void CoreSession::recvMessageFromServer(Message::Type type, QString target, QString text, QString sender, quint8 flags) {
  Server *s = qobject_cast<Server*>(this->sender());
  Q_ASSERT(s);
  BufferId buf;
  if((flags & Message::PrivMsg) && !(flags & Message::Self)) {
    buf = storage->getBufferId(user, s->getNetwork(), nickFromMask(sender));
  } else {
    buf = storage->getBufferId(user, s->getNetwork(), target);
  }
  Message msg(buf, type, text, sender, flags);
  msg.msgId = storage->logMessage(msg); //qDebug() << msg.msgId;
  Q_ASSERT(msg.msgId);
  emit displayMsg(msg);
}

void CoreSession::recvStatusMsgFromServer(QString msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  emit displayStatusMsg(s->getNetwork(), msg);
}


QList<BufferId> CoreSession::buffers() const {
  return storage->requestBuffers(user);
}


QVariant CoreSession::sessionState() {
  VarMap v;
  QList<QVariant> bufs;
  foreach(BufferId id, storage->requestBuffers(user)) { bufs.append(QVariant::fromValue(id)); }
  v["Buffers"] = bufs;
  mutex.lock();
  v["SessionData"] = sessionData;
  mutex.unlock();

  return v;
}

void CoreSession::sendServerStates() {
  emit serverStateRequested();
}

void CoreSession::sendBacklog(BufferId id, QVariant v1, QVariant v2) {
  QList<QVariant> log;
  QList<Message> msglist;
  if(v1.type() == QVariant::DateTime) {


  } else {
    msglist = storage->requestMsgs(id, v1.toInt(), v2.toInt());
  }

  // Send messages out in smaller packages - we don't want to make the signal data too large!
  for(int i = 0; i < msglist.count(); i++) {
    log.append(QVariant::fromValue(msglist[i]));
    if(log.count() >= 5) {
      emit backlogData(id, log, i >= msglist.count() - 1);
      log.clear();
    }
  }
  if(log.count() > 0) emit backlogData(id, log, true);
}

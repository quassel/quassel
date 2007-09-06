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
#include "signalproxy.h"
#include "storage.h"
#include "util.h"

CoreSession::CoreSession(UserId uid, Storage *_storage, QObject *parent) : QObject(parent), user(uid), storage(_storage) {
  _signalProxy = new SignalProxy(SignalProxy::Server, 0, this);

  QSettings s;
  s.beginGroup(QString("SessionData/%1").arg(user));
  mutex.lock();
  foreach(QString key, s.allKeys()) {
    sessionData[key] = s.value(key);
  }
  mutex.unlock();

  SignalProxy *p = signalProxy();

  p->attachSlot(SIGNAL(requestNetworkStates()), this, SIGNAL(serverStateRequested()));
  p->attachSlot(SIGNAL(requestConnect(QString)), this, SLOT(connectToNetwork(QString)));
  p->attachSlot(SIGNAL(sendInput(BufferId, QString)), this, SLOT(msgFromGui(BufferId, QString)));
  p->attachSlot(SIGNAL(importOldBacklog()), storage, SLOT(importOldBacklog()));
  p->attachSlot(SIGNAL(requestBacklog(BufferId, QVariant, QVariant)), this, SLOT(sendBacklog(BufferId, QVariant, QVariant)));
  p->attachSlot(SIGNAL(requestNetworkStates()), this, SLOT(sendServerStates()));
  p->attachSignal(this, SIGNAL(displayMsg(Message)));
  p->attachSignal(this, SIGNAL(displayStatusMsg(QString, QString)));
  p->attachSignal(this, SIGNAL(backlogData(BufferId, QVariantList, bool)));
  p->attachSignal(this, SIGNAL(bufferIdUpdated(BufferId)));
  p->attachSignal(storage, SIGNAL(bufferIdUpdated(BufferId)));
  p->attachSignal(this, SIGNAL(sessionDataChanged(const QString &, const QVariant &)), SIGNAL(coreSessionDataChanged(const QString &, const QVariant &)));
  p->attachSlot(SIGNAL(clientSessionDataChanged(const QString &, const QVariant &)), this, SLOT(storeSessionData(const QString &, const QVariant &)));

  /* Autoconnect. (When) do we actually do this?
  QStringList list;
  QVariantMap networks = retrieveSessionData("Networks").toMap();
  foreach(QString net, networks.keys()) {
    if(networks[net].toMap()["AutoConnect"].toBool()) {
      list << net;
    }
  } qDebug() << list;
  if(list.count()) connectToIrc(list);
  */
}

CoreSession::~CoreSession() {

}

UserId CoreSession::userId() const {
  return user;
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

void CoreSession::connectToNetwork(QString network) {
  QStringList networks; networks << network; // FIXME obsolete crap
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
      connect(server, SIGNAL(displayMsg(Message::Type, QString, QString, QString, quint8)), this, SLOT(recvMessageFromServer(Message::Type, QString, QString, QString, quint8)));
      connect(server, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));

      SignalProxy *p = signalProxy();
      p->attachSignal(server, SIGNAL(serverState(QString, QVariantMap)), SIGNAL(networkState(QString, QVariantMap)));
      p->attachSignal(server, SIGNAL(modeSet(QString, QString, QString)));
      p->attachSignal(server, SIGNAL(nickAdded(QString, QString, QVariantMap)));
      p->attachSignal(server, SIGNAL(nickRenamed(QString, QString, QString)));
      p->attachSignal(server, SIGNAL(nickRemoved(QString, QString)));
      p->attachSignal(server, SIGNAL(nickUpdated(QString, QString, QVariantMap)));
      p->attachSignal(server, SIGNAL(ownNickSet(QString, QString)));
      p->attachSignal(server, SIGNAL(queryRequested(QString, QString)));
      // TODO add error handling
      p->attachSignal(server, SIGNAL(connected(QString)), SIGNAL(networkConnected(QString)));
      p->attachSignal(server, SIGNAL(disconnected(QString)), SIGNAL(networkDisconnected(QString)));

      server->start();
      servers[net] = server;
    }
    emit connectToIrc(net);
  }
}

void CoreSession::addClient(QIODevice *device) {
  signalProxy()->addPeer(device);
}

SignalProxy *CoreSession::signalProxy() const {
  return _signalProxy;
}

void CoreSession::serverConnected(QString net) {
  storage->getBufferId(userId(), net); // create status buffer
}

void CoreSession::serverDisconnected(QString net) {
  delete servers[net];
  servers.remove(net);
  signalProxy()->sendSignal(SIGNAL(networkDisconnected(QString)), net);  // FIXME does this work?
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
  msg.msgId = storage->logMessage(msg);
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
  QVariantMap v;
  QList<QVariant> bufs;
  foreach(BufferId id, storage->requestBuffers(user)) { bufs.append(QVariant::fromValue(id)); }
  v["Buffers"] = bufs;
  mutex.lock();
  v["SessionData"] = sessionData;
  mutex.unlock();
  v["Networks"] = QVariant(servers.keys());
  // v["Payload"] = QByteArray(100000000, 'a');  // for testing purposes
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

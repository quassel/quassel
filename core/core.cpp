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

#include "core.h"
#include "server.h"
#include "global.h"
#include "util.h"
#include "coreproxy.h"
#include "sqlitestorage.h"

#include <QtSql>
#include <QSettings>

Core *Core::instanceptr = 0;

Core * Core::instance() {
  if(instanceptr) return instanceptr;
  instanceptr = new Core();
  instanceptr->init();
  return instanceptr;
}

void Core::destroy() {
  delete instanceptr;
  instanceptr = 0;
}

Core::Core() {

}

void Core::init() {
  if(!SqliteStorage::isAvailable()) {
    qFatal("Sqlite is currently required! Please make sure your Qt library has sqlite support enabled.");
  }
  //SqliteStorage::init();
  storage = new SqliteStorage();
  connect(Global::instance(), SIGNAL(dataPutLocally(UserId, QString)), this, SLOT(updateGlobalData(UserId, QString)));
  connect(&server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  //startListening(); // FIXME
  if(Global::runMode == Global::Monolithic) {  // TODO Make GUI user configurable
    guiUser = storage->validateUser("Default", "password");
    if(!guiUser) guiUser = storage->addUser("Default", "password");
    Q_ASSERT(guiUser);
    Global::setGuiUser(guiUser);
    createSession(guiUser);
  } else guiUser = 0;

  // Read global settings from config file
  QSettings s;
  s.beginGroup("Global");
  foreach(QString unum, s.childGroups()) {
    UserId uid = unum.toUInt();
    s.beginGroup(unum);
    foreach(QString key, s.childKeys()) {
      Global::updateData(uid, key, s.value(key));
    }
    s.endGroup();
  }
  s.endGroup();
}

Core::~Core() {
  foreach(QTcpSocket *sock, validClients.keys()) {
    delete sock;
  }
  qDeleteAll(sessions);
  delete storage;
}

CoreSession *Core::session(UserId uid) {
  Core *core = instance();
  if(core->sessions.contains(uid)) return core->sessions[uid];
  else return 0;
}

CoreSession *Core::guiSession() {
  Core *core = instance();
  if(core->guiUser && core->sessions.contains(core->guiUser)) return core->sessions[core->guiUser];
  else return 0;
}

CoreSession *Core::createSession(UserId uid) {
  Core *core = instance();
  Q_ASSERT(!core->sessions.contains(uid));
  CoreSession *sess = new CoreSession(uid, core->storage);
  core->sessions[uid] = sess;
  connect(sess, SIGNAL(proxySignal(CoreSignal, QVariant, QVariant, QVariant)), core, SLOT(recvProxySignal(CoreSignal, QVariant, QVariant, QVariant)));
  return sess;
}


bool Core::startListening(uint port) {
  if(!server.listen(QHostAddress::Any, port)) {
    qWarning(QString(QString("Could not open GUI client port %1: %2").arg(port).arg(server.errorString())).toAscii());
    return false;
  }
  qDebug() << "Listening for GUI clients on port" << server.serverPort();
  return true;
}

void Core::stopListening() {
  server.close();
  qDebug() << "No longer listening for GUI clients.";
}

void Core::incomingConnection() {
  // TODO implement SSL
  QTcpSocket *socket = server.nextPendingConnection();
  connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
  connect(socket, SIGNAL(readyRead()), this, SLOT(clientHasData()));
  blockSizes.insert(socket, (quint32)0);
  qDebug() << "Client connected from " << socket->peerAddress().toString();
}

void Core::clientHasData() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  Q_ASSERT(socket && blockSizes.contains(socket));
  quint32 bsize = blockSizes.value(socket);
  QVariant item;
  while(readDataFromDevice(socket, bsize, item)) {
    if(validClients.contains(socket)) {
      QList<QVariant> sigdata = item.toList();
      if((GUISignal)sigdata[0].toInt() == GS_UPDATE_GLOBAL_DATA) {
        processClientUpdate(socket, sigdata[1].toString(), sigdata[2]);
      } else {
        sessions[validClients[socket]]->processSignal((GUISignal)sigdata[0].toInt(), sigdata[1], sigdata[2], sigdata[3]);
      }
    } else {
      // we need to auth the client
      try {
        processClientInit(socket, item);
      } catch(Storage::AuthError) {
        qWarning() << "Authentification error!";  // FIXME
        socket->close();
        return;
      } catch(Exception e) {
        qWarning() << "Client init error:" << e.msg();
        socket->close();
        return;
      }
    }
    blockSizes[socket] = bsize = 0;
  }
  blockSizes[socket] = bsize;
}

void Core::clientDisconnected() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  blockSizes.remove(socket);
  validClients.remove(socket);
  qDebug() << "Client disconnected.";
  // TODO remove unneeded sessions - if necessary/possible...
}

void Core::processClientInit(QTcpSocket *socket, const QVariant &v) {
  VarMap msg = v.toMap();
  if(msg["GUIProtocol"].toUInt() != GUI_PROTOCOL) {
    //qWarning() << "Client version mismatch.";
    throw Exception("GUI client version mismatch");
  }
  // Auth
  UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString());  // throws exception if this failed

  // Find or create session for validated user
  CoreSession *sess;
  if(sessions.contains(uid)) sess = sessions[uid];
  else {
    sess = createSession(uid);
    validClients[socket] = uid;
  }
  VarMap reply;
  VarMap coreData;
  // FIXME
  QStringList dataKeys = Global::keys(uid);
  QString key;
  foreach(key, dataKeys) {
    coreData[key] = Global::data(key);
  }
  reply["CoreData"] = coreData;
  reply["SessionState"] = sess->sessionState();
  QList<QVariant> sigdata;
  sigdata.append(CS_CORE_STATE); sigdata.append(QVariant(reply)); sigdata.append(QVariant()); sigdata.append(QVariant());
  writeDataToDevice(socket, QVariant(sigdata));
  sess->sendServerStates();
}

void Core::processClientUpdate(QTcpSocket *socket, QString key, const QVariant &data) {
  UserId uid = validClients[socket];
  Global::updateData(uid, key, data);
  QList<QVariant> sigdata;
  sigdata.append(CS_UPDATE_GLOBAL_DATA); sigdata.append(key); sigdata.append(data); sigdata.append(QVariant());
  foreach(QTcpSocket *s, validClients.keys()) {
    if(validClients[s] == uid && s != socket) writeDataToDevice(s, QVariant(sigdata));
  }
}

void Core::updateGlobalData(UserId uid, QString key) {
  QVariant data = Global::data(uid, key);
  QList<QVariant> sigdata;
  sigdata.append(CS_UPDATE_GLOBAL_DATA); sigdata.append(key); sigdata.append(data); sigdata.append(QVariant());
  foreach(QTcpSocket *socket, validClients.keys()) {
    if(validClients[socket] == uid) writeDataToDevice(socket, QVariant(sigdata));
  }
}

void Core::recvProxySignal(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  CoreSession *sess = qobject_cast<CoreSession*>(sender());
  Q_ASSERT(sess);
  UserId uid = sess->userId();
  QList<QVariant> sigdata;
  sigdata.append(sig); sigdata.append(arg1); sigdata.append(arg2); sigdata.append(arg3);
  //qDebug() << "Sending signal: " << sigdata;
  foreach(QTcpSocket *socket, validClients.keys()) {
    if(validClients[socket] == uid) writeDataToDevice(socket, QVariant(sigdata));
  }
}

/*
  // Read global settings from config file
  QSettings s;
  s.beginGroup("Global");
  QString key;
  foreach(key, s.childKeys()) {
    global->updateData(key, s.value(key));
  }

  global->updateData("CoreReady", true);
  // Now that we are in sync, we can connect signals to automatically store further updates.
  // I don't think we care if global data changed locally or if it was updated by a client. 
  connect(global, SIGNAL(dataUpdatedRemotely(QString)), SLOT(globalDataUpdated(QString)));
  connect(global, SIGNAL(dataPutLocally(QString)), SLOT(globalDataUpdated(QString)));

}
  */

CoreSession::CoreSession(UserId uid, Storage *_storage) : user(uid), storage(_storage) {
  coreProxy = new CoreProxy();
  connect(coreProxy, SIGNAL(send(CoreSignal, QVariant, QVariant, QVariant)), this, SIGNAL(proxySignal(CoreSignal, QVariant, QVariant, QVariant)));

  connect(coreProxy, SIGNAL(requestServerStates()), this, SIGNAL(serverStateRequested()));
  connect(coreProxy, SIGNAL(gsRequestConnect(QStringList)), this, SLOT(connectToIrc(QStringList)));
  connect(coreProxy, SIGNAL(gsUserInput(BufferId, QString)), this, SLOT(msgFromGui(BufferId, QString)));
  connect(coreProxy, SIGNAL(gsImportBacklog()), storage, SLOT(importOldBacklog()));
  connect(coreProxy, SIGNAL(gsRequestBacklog(BufferId, QVariant, QVariant)), this, SLOT(sendBacklog(BufferId, QVariant, QVariant)));
  connect(this, SIGNAL(displayMsg(Message)), coreProxy, SLOT(csDisplayMsg(Message)));
  connect(this, SIGNAL(displayStatusMsg(QString, QString)), coreProxy, SLOT(csDisplayStatusMsg(QString, QString)));
  connect(this, SIGNAL(backlogData(BufferId, QList<QVariant>, bool)), coreProxy, SLOT(csBacklogData(BufferId, QList<QVariant>, bool)));
  connect(this, SIGNAL(bufferIdUpdated(BufferId)), coreProxy, SLOT(csUpdateBufferId(BufferId)));
  connect(storage, SIGNAL(bufferIdUpdated(BufferId)), coreProxy, SLOT(csUpdateBufferId(BufferId)));
  connect(Global::instance(), SIGNAL(dataUpdatedRemotely(UserId, QString)), this, SLOT(globalDataUpdated(UserId, QString)));
  connect(Global::instance(), SIGNAL(dataPutLocally(UserId, QString)), this, SLOT(globalDataUpdated(UserId, QString)));

}

CoreSession::~CoreSession() {

}

UserId CoreSession::userId() {
  return user;
}

void CoreSession::processSignal(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  coreProxy->recv(sig, arg1, arg2, arg3);
}

void CoreSession::globalDataUpdated(UserId uid, QString key) {
  Q_ASSERT(uid == userId());
  QVariant data = Global::data(userId(), key);
  QSettings s;
  s.setValue(QString("Global/%1/").arg(userId())+key, data);
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
      connect(server, SIGNAL(disconnected(QString)), this, SLOT(serverDisconnected(QString)));

      server->start();
      servers[net] = server;
    }
    emit connectToIrc(net);
  }
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


//Core *core = 0;

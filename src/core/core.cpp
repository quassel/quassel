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
#include "coresession.h"
#include "sqlitestorage.h"
#include "util.h"

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
  guiUser = 0;
  /*
  if(Global::runMode == Global::Monolithic) {  // TODO Make GUI user configurable
    try {
      guiUser = storage->validateUser("Default", "password");
    } catch(Storage::AuthError) {
      guiUser = storage->addUser("Default", "password");
    }
    Q_ASSERT(guiUser);
    Global::setGuiUser(guiUser);
    createSession(guiUser);
  } else guiUser = 0;
  */
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

CoreSession *Core::localSession() {
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
      if((ClientSignal)sigdata[0].toInt() == GS_UPDATE_GLOBAL_DATA) {
        processClientUpdate(socket, sigdata[1].toString(), sigdata[2]);
      } else {
        sessions[validClients[socket]]->processSignal((ClientSignal)sigdata[0].toInt(), sigdata[1], sigdata[2], sigdata[3]);
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

QVariant Core::connectLocalClient(QString user, QString passwd) {
  UserId uid = instance()->storage->validateUser(user, passwd);
  QVariant reply = instance()->initSession(uid);
  instance()->guiUser = uid;
  Global::setGuiUser(uid);
  qDebug() << "Local client connected.";
  return reply;
}

void Core::disconnectLocalClient() {
  qDebug() << "Local client disconnected.";
  instance()->guiUser = 0;
  Global::setGuiUser(0);
}

void Core::processClientInit(QTcpSocket *socket, const QVariant &v) {
  VarMap msg = v.toMap();
  if(msg["GUIProtocol"].toUInt() != GUI_PROTOCOL) {
    //qWarning() << "Client version mismatch.";
    throw Exception("GUI client version mismatch");
  }
    // Auth
  UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString());  // throws exception if this failed
  VarMap reply = initSession(uid).toMap();
  validClients[socket] = uid;
  QList<QVariant> sigdata;
  sigdata.append(CS_CORE_STATE); sigdata.append(QVariant(reply)); sigdata.append(QVariant()); sigdata.append(QVariant());
  writeDataToDevice(socket, QVariant(sigdata));
}

QVariant Core::initSession(UserId uid) {
  // Find or create session for validated user
  CoreSession *sess;
  if(sessions.contains(uid)) sess = sessions[uid];
  else {
    sess = createSession(uid);
    //validClients[socket] = uid;
  }
  VarMap reply;
  VarMap coreData;
  QStringList dataKeys = Global::keys(uid);
  foreach(QString key, dataKeys) {
    coreData[key] = Global::data(uid, key);
  }
  reply["CoreData"] = coreData;
  reply["SessionState"] = sess->sessionState();
  return reply;
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

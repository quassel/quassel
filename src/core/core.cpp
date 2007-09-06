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
  connect(&server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  startListening(); // FIXME make configurable
  guiUser = 0;
}

Core::~Core() {
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
  // TODO While
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
  if(readDataFromDevice(socket, bsize, item)) {
    // we need to auth the client
    try {
      processClientInit(socket, item);
    } catch(Storage::AuthError) {
      qWarning() << "Authentification error!";  // FIXME: send auth error to client
      socket->close();
      return;
    } catch(Exception e) {
      qWarning() << "Client init error:" << e.msg();
      socket->close();
      return;
    }
  }
  blockSizes[socket] = bsize = 0;  // FIXME blockSizes aufräum0rn!
}

// FIXME: no longer called, since connection handling is now in SignalProxy
void Core::clientDisconnected() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  blockSizes.remove(socket);
  qDebug() << "Client disconnected.";
  // TODO remove unneeded sessions - if necessary/possible...
}

QVariant Core::connectLocalClient(QString user, QString passwd) {
  UserId uid = instance()->storage->validateUser(user, passwd);
  QVariant reply = instance()->initSession(uid);
  instance()->guiUser = uid;
  qDebug() << "Local client connected.";
  return reply;
}

void Core::disconnectLocalClient() {
  qDebug() << "Local client disconnected.";
  instance()->guiUser = 0;
}

void Core::processClientInit(QTcpSocket *socket, const QVariant &v) {
  QVariantMap msg = v.toMap();
  if(msg["GuiProtocol"].toUInt() != GUI_PROTOCOL) {
    //qWarning() << "Client version mismatch.";
    throw Exception("GUI client version mismatch");
  }
    // Auth
  UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString());  // throws exception if this failed
  QVariant reply = initSession(uid);
  disconnect(socket, 0, this, 0);
  sessions[uid]->addClient(socket);
  qDebug() << "Client initialized successfully.";
  writeDataToDevice(socket, reply);
}

QVariant Core::initSession(UserId uid) {
  // Find or create session for validated user
  CoreSession *sess;
  if(sessions.contains(uid)) sess = sessions[uid];
  else {
    sess = createSession(uid);
  }
  QVariantMap reply;
  reply["SessionState"] = sess->sessionState();
  return reply;
}

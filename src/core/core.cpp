/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include "core.h"
#include "coresession.h"
#include "coresettings.h"
#include "sqlitestorage.h"
#include "util.h"

#include <QMetaObject>
#include <QMetaMethod>

Core *Core::instanceptr = 0;

Core *Core::instance() {
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
  storage = NULL;
}

void Core::init() {
  CoreSettings s;
  configured = false;

  QVariantMap dbSettings = s.databaseSettings().toMap();
  QString hname = dbSettings["Type"].toString().toLower();
  hname[0] = hname[0].toUpper();
  hname = "initStorage" + hname;
  if (!QMetaObject::invokeMethod(this, hname.toAscii(), Q_RETURN_ARG(bool, configured),  Q_ARG(QVariantMap, dbSettings), Q_ARG(bool, false))) {
    qWarning("No database backend configured.");
  }
  
  if (!configured) {
    qWarning("Core is currently not configured!");
  }
    
  connect(&server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  startListening(s.port());
  guiUser = 0;
}

bool Core::initStorageSqlite(QVariantMap dbSettings, bool setup) {
  if (!SqliteStorage::isAvailable()) {
    qFatal("Sqlite is currently required! Please make sure your Qt library has sqlite support enabled.");
  }
  if (storage) {
    qDebug() << "Deleting old storage object.";
    delete storage;
    storage = NULL;
  }
  
  storage = new SqliteStorage();
  if (setup && !storage->setup(dbSettings)) {
    return false;
  }
  
  return storage->init(dbSettings);
}

Core::~Core() {
  qDeleteAll(sessions);
  if (storage) {
    delete storage;
    storage = NULL;
  }
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
  while (server.hasPendingConnections()) {
    QTcpSocket *socket = server.nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(clientHasData()));
    blockSizes.insert(socket, (quint32)0);
    qDebug() << "Client connected from " << socket->peerAddress().toString();
    
    if (!configured) {
      server.close();
      qDebug() << "Closing server for basic setup.";
    }
  }
}

void Core::clientHasData() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  Q_ASSERT(socket && blockSizes.contains(socket));
  quint32 bsize = blockSizes.value(socket);
  QVariant item;
  if(readDataFromDevice(socket, bsize, item)) {
    // we need to auth the client
    try {
      QVariantMap msg = item.toMap();
      if (msg["GuiProtocol"].toUInt() != GUI_PROTOCOL) {
        throw Exception("GUI client version mismatch");
      }
      if (configured) {
        processClientInit(socket, msg);
      } else {
        processCoreSetup(socket, msg);
      }
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
  blockSizes[socket] = bsize = 0;  // FIXME blockSizes aufr�um0rn!
}

// FIXME: no longer called, since connection handling is now in SignalProxy
// No, it is called as long as core is not configured. (kaffeedoktor)
void Core::clientDisconnected() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  blockSizes.remove(socket);
  qDebug() << "Client disconnected.";
  
  // make server listen again if still not configured
  if (!configured) {
    startListening();
  }
  
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

void Core::processClientInit(QTcpSocket *socket, const QVariantMap &msg) {
  // Auth
  QVariantMap reply;
  UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString()); // throws exception if this failed
  reply["StartWizard"] = false;
  reply["Reply"] = initSession(uid);
  disconnect(socket, 0, this, 0);
  sessions[uid]->addClient(socket);
  qDebug() << "Client initialized successfully.";
  writeDataToDevice(socket, reply);
}

void Core::processCoreSetup(QTcpSocket *socket, QVariantMap &msg) {
  if(msg["HasSettings"].toBool()) {
    QVariantMap auth;
    auth["User"] = msg["User"];
    auth["Password"] = msg["Password"];
    msg.remove("User");
    msg.remove("Password");
    qDebug() << "Initializing storage provider" << msg["Type"].toString();
    QString hname = msg["Type"].toString().toLower();
    hname[0] = hname[0].toUpper();
    hname = "initStorage" + hname;
    if (!QMetaObject::invokeMethod(this, hname.toAscii(), Q_RETURN_ARG(bool, configured),  Q_ARG(QVariantMap, msg), Q_ARG(bool, true))) {
      qWarning("No database backend configured.");
    }
    if (!configured) {
      // notify client to start wizard again
      qWarning("Core is currently not configured!");
      QVariantMap reply;
      reply["StartWizard"] = true;
      reply["StorageProviders"] = availableStorageProviders();
      writeDataToDevice(socket, reply);
    } else {
      // write coresettings
      CoreSettings s;
      s.setDatabaseSettings(msg);
      // write admin user to database & make the core listen again to connections
      storage->addUser(auth["User"].toString(), auth["Password"].toString());
      startListening();
      // continue the normal procedure
      processClientInit(socket, auth);
    }
  } else {
    // notify client to start wizard
    QVariantMap reply;
    reply["StartWizard"] = true;
    reply["StorageProviders"] = availableStorageProviders();
    writeDataToDevice(socket, reply);
  }
}

QVariant Core::initSession(UserId uid) {
  // Find or create session for validated user
  CoreSession *sess;
  if(sessions.contains(uid))
    sess = sessions[uid];
  else
    sess = createSession(uid);
  QVariantMap reply;
  reply["SessionState"] = sess->sessionState();
  return reply;
}

QStringList Core::availableStorageProviders() {
  QStringList storageProviders;
  if (SqliteStorage::isAvailable()) {
    storageProviders.append(SqliteStorage::displayName());
  }
  // TODO: temporary
  storageProviders.append("MySQL");
  
  return storageProviders;
}

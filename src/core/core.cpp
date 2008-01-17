/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include <QMetaObject>
#include <QMetaMethod>
#include <QMutexLocker>
#include <QCoreApplication>

#include "core.h"
#include "coresession.h"
#include "coresettings.h"
#include "signalproxy.h"
#include "sqlitestorage.h"

Core *Core::instanceptr = 0;
QMutex Core::mutex;

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

Core::Core()
  : storage(0)
{
  startTime = QDateTime::currentDateTime();  // for uptime :)
}

void Core::init() {
  configured = false;

  CoreSettings cs;
  if(!(configured = initStorage(cs.databaseSettings().toMap()))) {
    qWarning("Core is currently not configured!");
  }

  connect(&server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  startListening(cs.port());
  guiUser = 0;

}

bool Core::initStorage(QVariantMap dbSettings, bool setup) {
  QString engine = dbSettings["Type"].toString().toLower();

  if(storage) {
    qDebug() << "Deleting old storage object.";
    storage->deleteLater();
    storage = 0;
  }

  // FIXME register new storageProviders here
  if(engine == "sqlite" && SqliteStorage::isAvailable()) {
    storage = new SqliteStorage(this);
    connect(storage, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)), this, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)));
  } else {
    qWarning() << "Selected StorageBackend is not available:" << dbSettings["Type"].toString();
    return configured = false;
  }

  if(setup && !storage->setup(dbSettings)) {
    return configured = false;
  }

  return configured = storage->init(dbSettings);
}

Core::~Core() {
  // FIXME properly shutdown the sessions
  qDeleteAll(sessions);
}

void Core::restoreState() {
  return;
  /*
  Q_ASSERT(!instance()->sessions.count());
  CoreSettings s;
  QList<QVariant> users = s.coreState().toList();
  if(users.count() > 0) {
    qDebug() << "Restoring previous core state...";
    foreach(QVariant v, users) {
      QVariantMap m = v.toMap();
      if(m.contains("UserId")) {
        CoreSession *sess = createSession(m["UserId"].toUInt());
        sess->restoreState(m["State"]);  // FIXME multithreading
      }
    }
    qDebug() << "...done.";
  }
  */
}

void Core::saveState() {
  /*
  CoreSettings s;
  QList<QVariant> users;
  foreach(CoreSession *sess, instance()->sessions.values()) {
    QVariantMap m;
    m["UserId"] = sess->user();  // FIXME multithreading
    m["State"] = sess->state();
    users << m;
  }
  s.setCoreState(users);
  */
}

/*** Storage Access ***/

NetworkId Core::networkId(UserId user, const QString &network) {
  QMutexLocker locker(&mutex);
  return instance()->storage->getNetworkId(user, network);
}

BufferInfo Core::bufferInfo(UserId user, const QString &network, const QString &buffer) {
  //QMutexLocker locker(&mutex);
  return instance()->storage->getBufferInfo(user, network, buffer);
}

MsgId Core::storeMessage(const Message &message) {
  QMutexLocker locker(&mutex);
  return instance()->storage->logMessage(message);
}

QList<Message> Core::requestMsgs(BufferInfo buffer, int lastmsgs, int offset) {
  QMutexLocker locker(&mutex);
  return instance()->storage->requestMsgs(buffer, lastmsgs, offset);
}

QList<Message> Core::requestMsgs(BufferInfo buffer, QDateTime since, int offset) {
  QMutexLocker locker(&mutex);
  return instance()->storage->requestMsgs(buffer, since, offset);
}

QList<Message> Core::requestMsgRange(BufferInfo buffer, int first, int last) {
  QMutexLocker locker(&mutex);
  return instance()->storage->requestMsgRange(buffer, first, last);
}

QList<BufferInfo> Core::requestBuffers(UserId user, QDateTime since) {
  QMutexLocker locker(&mutex);
  return instance()->storage->requestBuffers(user, since);
}

/*** Network Management ***/

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
    QVariantMap clientInfo;
    blocksizes.insert(socket, (quint32)0);
    qDebug() << "Client connected from"  << qPrintable(socket->peerAddress().toString());

    if (!configured) {
      server.close();
      qDebug() << "Closing server for basic setup.";
    }
  }
}

void Core::clientHasData() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  Q_ASSERT(socket && blocksizes.contains(socket));
  QVariant item;
  while(SignalProxy::readDataFromDevice(socket, blocksizes[socket], item)) {
    QVariantMap msg = item.toMap();
    if(!msg.contains("MsgType")) {
      // Client is way too old, does not even use the current init format
      qWarning() << qPrintable(tr("Antique client trying to connect... refusing."));
      socket->close();
      return;
    }
    // OK, so we have at least an init message format we can understand
    if(msg["MsgType"] == "ClientInit") {
      QVariantMap reply;
      reply["CoreVersion"] = Global::quasselVersion;
      reply["CoreDate"] = Global::quasselDate;
      reply["CoreBuild"] = Global::quasselBuild;
      // TODO: Make the core info configurable
      int uptime = startTime.secsTo(QDateTime::currentDateTime());
      int updays = uptime / 86400; uptime %= 86400;
      int uphours = uptime / 3600; uptime %= 3600;
      int upmins = uptime / 60;
      reply["CoreInfo"] = tr("<b>Quassel Core Version %1 (Build >= %2)</b><br>"
                             "Up %3d%4h%5m (since %6)").arg(Global::quasselVersion).arg(Global::quasselBuild)
                             .arg(updays).arg(uphours,2,10,QChar('0')).arg(upmins,2,10,QChar('0')).arg(startTime.toString(Qt::TextDate));

      reply["SupportSsl"] = false;
      reply["LoginEnabled"] = true;
      // TODO: check if we are configured, start wizard otherwise

      // Just version information -- check it!
      if(msg["ClientBuild"].toUInt() < Global::clientBuildNeeded) {
        reply["MsgType"] = "ClientInitReject";
        reply["Error"] = tr("<b>Your Quassel Client is too old!</b><br>"
                            "This core needs at least client version %1 (Build >= %2).<br>"
                            "Please consider upgrading your client.").arg(Global::quasselVersion).arg(Global::quasselBuild);
        SignalProxy::writeDataToDevice(socket, reply);
        qWarning() << qPrintable(tr("Client %1 too old, rejecting.").arg(socket->peerAddress().toString()));
        socket->close(); return;
      }
      clientInfo[socket] = msg; // store for future reference
      reply["MsgType"] = "ClientInitAck";
      SignalProxy::writeDataToDevice(socket, reply);
    } else if(msg["MsgType"] == "ClientLogin") {
      QVariantMap reply;
      if(!clientInfo.contains(socket)) {
        reply["MsgType"] = "ClientLoginReject";
        reply["Error"] = tr("<b>Client not initialized!</b><br>You need to send an init message before trying to login.");
        SignalProxy::writeDataToDevice(socket, reply);
        qWarning() << qPrintable(tr("Client %1 did not send an init message before trying to login, rejecting.").arg(socket->peerAddress().toString()));
        socket->close(); return;
      }
      mutex.lock();
      UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString());
      mutex.unlock();
      if(!uid) {
        reply["MsgType"] = "ClientLoginReject";
        reply["Error"] = tr("<b>Invalid username or password!</b><br>The username/password combination you supplied could not be found in the database.");
        SignalProxy::writeDataToDevice(socket, reply);
        continue;
      }
      reply["MsgType"] = "ClientLoginAck";
      SignalProxy::writeDataToDevice(socket, reply);
      qDebug() << qPrintable(tr("Client %1 initialized and authentificated successfully as \"%2\".").arg(socket->peerAddress().toString(), msg["User"].toString()));
      setupClientSession(socket, uid);
    }
    //socket->close(); return;
    /*
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
    } */
  }
}

// Potentially called during the initialization phase (before handing the connection off to the session)
void Core::clientDisconnected() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());
  blocksizes.remove(socket);
  clientInfo.remove(socket);
  qDebug() << qPrintable(tr("Client %1 disconnected.").arg(socket->peerAddress().toString()));
  socket->deleteLater();
  socket = 0;

  // make server listen again if still not configured  FIXME
  if (!configured) {
    startListening();
  }

  // TODO remove unneeded sessions - if necessary/possible...
  // Suggestion: kill sessions if they are not connected to any network and client.
}

  
  //disconnect(socket, 0, this, 0);
  /*
  sessions[uid]->addClient(socket);  // FIXME multithreading
  qDebug() << "Client initialized successfully.";
  SignalProxy::writeDataToDevice(socket, reply);
  */


void Core::processCoreSetup(QTcpSocket *socket, QVariantMap &msg) {
  if(msg["HasSettings"].toBool()) {
    QVariantMap auth;
    auth["User"] = msg["User"];
    auth["Password"] = msg["Password"];
    msg.remove("User");
    msg.remove("Password");
    qDebug() << "Initializing storage provider" << msg["Type"].toString();

    if(!initStorage(msg, true)) {
      // notify client to start wizard again
      qWarning("Core is currently not configured!");
      QVariantMap reply;
      reply["StartWizard"] = true;
      reply["StorageProviders"] = availableStorageProviders();
      SignalProxy::writeDataToDevice(socket, reply);
    } else {
      // write coresettings
      CoreSettings s;
      s.setDatabaseSettings(msg);
      // write admin user to database & make the core listen again to connections
      storage->addUser(auth["User"].toString(), auth["Password"].toString());
      startListening();
      // continue the normal procedure
      //processClientInit(socket, auth);
    }
  } else {
    // notify client to start wizard
    QVariantMap reply;
    reply["StartWizard"] = true;
    reply["StorageProviders"] = availableStorageProviders();
    SignalProxy::writeDataToDevice(socket, reply);
  }
}

void Core::setupClientSession(QTcpSocket *socket, UserId uid) {
  // Find or create session for validated user
  SessionThread *sess;
  if(sessions.contains(uid)) sess = sessions[uid];
  else sess = createSession(uid);
  // Hand over socket, session then sends state itself
  disconnect(socket, 0, this, 0);
  if(!sess) {
    qWarning() << qPrintable(tr("Could not initialize session for client %1!").arg(socket->peerAddress().toString()));
    socket->close();
  }
  sess->addClient(socket);
}

SessionThread *Core::createSession(UserId uid) {
  if(sessions.contains(uid)) {
    qWarning() << "Calling createSession() when a session for the user already exists!";
    return 0;
  }
  SessionThread *sess = new SessionThread(uid, this);
  sessions[uid] = sess;
  sess->start();
  return sess;
}

QStringList Core::availableStorageProviders() {
  QStringList storageProviders;
  if (SqliteStorage::isAvailable()) {
    storageProviders.append(SqliteStorage::displayName());
  }
  // TODO: temporary
  // storageProviders.append("MySQL");
  
  return storageProviders;
}

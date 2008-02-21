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
#include "network.h"

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

Core::Core() : storage(0) {
  startTime = QDateTime::currentDateTime();  // for uptime :)

  // Register storage backends here!
  registerStorageBackend(new SqliteStorage(this));

  if(!_storageBackends.count()) {
    qWarning() << qPrintable(tr("Could not initialize any storage backend! Exiting..."));
    exit(1); // TODO make this less brutal (especially for mono client -> popup)
  }
  connect(&_storageSyncTimer, SIGNAL(timeout()), this, SLOT(syncStorage()));
  _storageSyncTimer.start(10 * 60 * 1000); // in msecs
}

void Core::init() {
  configured = false;

  CoreSettings cs;

  if(!(configured = initStorage(cs.storageSettings().toMap()))) {
    qWarning("Core is currently not configured!");

    // try to migrate old settings
    QVariantMap old = cs.oldDbSettings().toMap();
    if(old.count() && old["Type"].toString().toUpper() == "SQLITE") {
      QVariantMap newSettings;
      newSettings["Backend"] = "SQLite";
      if((configured = initStorage(newSettings))) {
        qWarning("...but thankfully I found some old settings to migrate!");
        cs.setStorageSettings(newSettings);
      }
    }
  }

  connect(&server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  if(!startListening(cs.port())) exit(1); // TODO make this less brutal
}

Core::~Core() {
  foreach(QTcpSocket *socket, blocksizes.keys()) { qDebug() << "disconnecting" << socket << blocksizes.keys();
    socket->disconnectFromHost();  // disconnect local (i.e. non-authed) clients
  }
  qDeleteAll(sessions);
  qDeleteAll(_storageBackends);
}

/*** Session Restore ***/

void Core::saveState() {
  CoreSettings s;
  QVariantMap state;
  QVariantList activeSessions;
  foreach(UserId user, instance()->sessions.keys()) activeSessions << QVariant::fromValue<UserId>(user);
  state["CoreBuild"] = Global::quasselBuild;
  state["ActiveSessions"] = activeSessions;
  s.setCoreState(state);
}

void Core::restoreState() {
  if(!instance()->configured) {
    qWarning() << qPrintable(tr("Cannot restore a state for an unconfigured core!"));
    return;
  }
  if(instance()->sessions.count()) {
    qWarning() << qPrintable(tr("Calling restoreState() even though active sessions exist!"));
    return;
  }
  CoreSettings s;
  uint build = s.coreState().toMap()["CoreBuild"].toUInt();
  if(build < 362) {
    qWarning() << qPrintable(tr("Core state too old, ignoring..."));
    return;
  }
  QVariantList activeSessions = s.coreState().toMap()["ActiveSessions"].toList();
  if(activeSessions.count() > 0) {
    qDebug() << "Restoring previous core state...";
    foreach(QVariant v, activeSessions) {
      UserId user = v.value<UserId>();
      instance()->createSession(user, true);
    }
  }
}

/*** Core Setup ***/

QString Core::setupCore(const QVariant &setupData_) {
  QVariantMap setupData = setupData_.toMap();
  QString user = setupData.take("AdminUser").toString();
  QString password = setupData.take("AdminPasswd").toString();
  if(user.isEmpty() || password.isEmpty()) {
    return tr("Admin user or password not set.");
  }
  if(!initStorage(setupData, true)) {
    return tr("Could not setup storage!");
  }
  CoreSettings s;
  s.setStorageSettings(setupData);
  qDebug() << qPrintable(tr("Creating admin user..."));
  mutex.lock();
  storage->addUser(user, password);
  mutex.unlock();
  startListening();  // TODO check when we need this
  return QString();
}

/*** Storage Handling ***/

bool Core::registerStorageBackend(Storage *backend) {
  if(backend->isAvailable()) {
    _storageBackends[backend->displayName()] = backend;
    return true;
  } else {
    backend->deleteLater();
    return false;
  }
}

void Core::unregisterStorageBackend(Storage *backend) {
  _storageBackends.remove(backend->displayName());
  backend->deleteLater();
}

// old db settings:
// "Type" => "sqlite"
bool Core::initStorage(QVariantMap dbSettings, bool setup) {
  QString backend = dbSettings["Backend"].toString();
  if(backend.isEmpty()) {
    //qWarning() << "No storage backend selected!";
    return configured = false;
  }

  if(_storageBackends.contains(backend)) {
    storage = _storageBackends[backend];
  } else {
    qWarning() << "Selected storage backend is not available:" << backend;
    return configured = false;
  }
  if(!storage->init(dbSettings)) {
    if(!setup || !(storage->setup(dbSettings) && storage->init(dbSettings))) {
      qWarning() << "Could not init storage!";
      storage = 0;
      return configured = false;
    }
  }
  // delete all other backends
  foreach(Storage *s, _storageBackends.values()) {
    if(s != storage) s->deleteLater();
  }
  _storageBackends.clear();

  connect(storage, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)), this, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)));
  return configured = true;
}

void Core::syncStorage() {
  QMutexLocker locker(&mutex);
  if(storage) storage->sync();
}

/*** Storage Access ***/
bool Core::createNetwork(UserId user, NetworkInfo &info) {
  QMutexLocker locker(&mutex);
  NetworkId networkId = instance()->storage->createNetwork(user, info);
  if(!networkId.isValid())
    return false;

  info.networkId = networkId;
  return true;
}

bool Core::updateNetwork(UserId user, const NetworkInfo &info) {
  QMutexLocker locker(&mutex);
  return instance()->storage->updateNetwork(user, info);
}

bool Core::removeNetwork(UserId user, const NetworkId &networkId) {
  QMutexLocker locker(&mutex);
  return instance()->storage->removeNetwork(user, networkId);
}

QList<NetworkInfo> Core::networks(UserId user) {
  QMutexLocker locker(&mutex);
  return instance()->storage->networks(user);
}

NetworkId Core::networkId(UserId user, const QString &network) {
  QMutexLocker locker(&mutex);
  return instance()->storage->getNetworkId(user, network);
}

QList<NetworkId> Core::connectedNetworks(UserId user) {
  QMutexLocker locker(&mutex);
  return instance()->storage->connectedNetworks(user);
}

void Core::setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected) {
  QMutexLocker locker(&mutex);
  return instance()->storage->setNetworkConnected(user, networkId, isConnected);
}

QHash<QString, QString> Core::persistentChannels(UserId user, const NetworkId &networkId) {
  QMutexLocker locker(&mutex);
  return instance()->storage->persistentChannels(user, networkId);
}

void Core::setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined) {
  QMutexLocker locker(&mutex);
  return instance()->storage->setChannelPersistent(user, networkId, channel, isJoined);
}

void Core::setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key) {
  QMutexLocker locker(&mutex);
  return instance()->storage->setPersistentChannelKey(user, networkId, channel, key);
}

BufferInfo Core::bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer) {
  QMutexLocker locker(&mutex);
  return instance()->storage->getBufferInfo(user, networkId, type, buffer);
}

BufferInfo Core::getBufferInfo(UserId user, const BufferId &bufferId) {
  QMutexLocker locker(&mutex);
  return instance()->storage->getBufferInfo(user, bufferId);
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

bool Core::removeBuffer(const UserId &user, const BufferId &bufferId) {
  QMutexLocker locker(&mutex);
  return instance()->storage->removeBuffer(user, bufferId);
}

BufferId Core::renameBuffer(const UserId &user, const NetworkId &networkId, const QString &newName, const QString &oldName) {
  QMutexLocker locker(&mutex);
  return instance()->storage->renameBuffer(user, networkId, newName, oldName);
}

void Core::setBufferLastSeen(UserId user, const BufferId &bufferId, const QDateTime &seenDate) {
  QMutexLocker locker(&mutex);
  return instance()->storage->setBufferLastSeen(user, bufferId, seenDate);
}

QHash<BufferId, QDateTime> Core::bufferLastSeenDates(UserId user) {
  QMutexLocker locker(&mutex);
  return instance()->storage->bufferLastSeenDates(user);
}

/*** Network Management ***/

bool Core::startListening(uint port) {
  if(!server.listen(QHostAddress::Any, port)) {
    qWarning(qPrintable(QString("Could not open GUI client port %1: %2").arg(port).arg(server.errorString())));
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
  while(server.hasPendingConnections()) {
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
    processClientMessage(socket, msg);
    if(!blocksizes.contains(socket)) break;  // this socket is no longer ours to handle!
  }
}

void Core::processClientMessage(QTcpSocket *socket, const QVariantMap &msg) {
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
    // check if we are configured, start wizard otherwise
    if(!configured) {
      reply["Configured"] = false;
      QList<QVariant> backends;
      foreach(Storage *backend, _storageBackends.values()) {
        QVariantMap v;
        v["DisplayName"] = backend->displayName();
        v["Description"] = backend->description();
        backends.append(v);
      }
      reply["StorageBackends"] = backends;
      reply["LoginEnabled"] = false;
    } else {
      reply["Configured"] = true;
    }
    clientInfo[socket] = msg; // store for future reference
    reply["MsgType"] = "ClientInitAck";
    SignalProxy::writeDataToDevice(socket, reply);
  } else {
    // for the rest, we need an initialized connection
    if(!clientInfo.contains(socket)) {
      QVariantMap reply;
      reply["MsgType"] = "ClientLoginReject";
      reply["Error"] = tr("<b>Client not initialized!</b><br>You need to send an init message before trying to login.");
      SignalProxy::writeDataToDevice(socket, reply);
      qWarning() << qPrintable(tr("Client %1 did not send an init message before trying to login, rejecting.").arg(socket->peerAddress().toString()));
      socket->close(); return;
    }
    if(msg["MsgType"] == "CoreSetupData") {
      QVariantMap reply;
      QString result = setupCore(msg["SetupData"]);
      if(!result.isEmpty()) {
        reply["MsgType"] = "CoreSetupReject";
        reply["Error"] = result;
      } else {
        reply["MsgType"] = "CoreSetupAck";
      }
      SignalProxy::writeDataToDevice(socket, reply);
    } else if(msg["MsgType"] == "ClientLogin") {
      QVariantMap reply;
      mutex.lock();
      UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString());
      mutex.unlock();
      if(uid == 0) {
        reply["MsgType"] = "ClientLoginReject";
        reply["Error"] = tr("<b>Invalid username or password!</b><br>The username/password combination you supplied could not be found in the database.");
        SignalProxy::writeDataToDevice(socket, reply);
        return;
      }
      reply["MsgType"] = "ClientLoginAck";
      SignalProxy::writeDataToDevice(socket, reply);
      qDebug() << qPrintable(tr("Client %1 initialized and authentificated successfully as \"%2\".").arg(socket->peerAddress().toString(), msg["User"].toString()));
      setupClientSession(socket, uid);
    }
  }
}

// Potentially called during the initialization phase (before handing the connection off to the session)
void Core::clientDisconnected() {
  QTcpSocket *socket = dynamic_cast<QTcpSocket*>(sender());  // Note: This might be a QObject* already (if called by ~Core())!
  Q_ASSERT(socket);
  blocksizes.remove(socket);
  clientInfo.remove(socket);
  qDebug() << qPrintable(tr("Non-authed client disconnected."));
  socket->deleteLater();
  socket = 0;

  // make server listen again if still not configured
  if (!configured) {
    startListening();
  }

  // TODO remove unneeded sessions - if necessary/possible...
  // Suggestion: kill sessions if they are not connected to any network and client.
}

void Core::setupClientSession(QTcpSocket *socket, UserId uid) {
  // Find or create session for validated user
  SessionThread *sess;
  if(sessions.contains(uid)) sess = sessions[uid];
  else sess = createSession(uid);
  // Hand over socket, session then sends state itself
  disconnect(socket, 0, this, 0);
  blocksizes.remove(socket);
  clientInfo.remove(socket);
  if(!sess) {
    qWarning() << qPrintable(tr("Could not initialize session for client %1!").arg(socket->peerAddress().toString()));
    socket->close();
  }
  sess->addClient(socket);
}

SessionThread *Core::createSession(UserId uid, bool restore) {
  if(sessions.contains(uid)) {
    qWarning() << "Calling createSession() when a session for the user already exists!";
    return 0;
  }
  SessionThread *sess = new SessionThread(uid, restore, this);
  sessions[uid] = sess;
  sess->start();
  return sess;
}

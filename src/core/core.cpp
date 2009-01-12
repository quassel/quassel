/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include <QCoreApplication>

#include "core.h"
#include "coresession.h"
#include "coresettings.h"
#include "quassel.h"
#include "signalproxy.h"
#include "sqlitestorage.h"
#include "network.h"
#include "logger.h"

#include "util.h"

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

Core::Core() : storage(0) {
  _startTime = QDateTime::currentDateTime().toUTC();  // for uptime :)

  loadTranslation(QLocale::system());

  // Register storage backends here!
  registerStorageBackend(new SqliteStorage(this));

  if(!_storageBackends.count()) {
    qWarning() << qPrintable(tr("Could not initialize any storage backend! Exiting..."));
    qWarning() << qPrintable(tr("Currently, Quassel only supports SQLite3. You need to build your\n"
                                "Qt library with the sqlite plugin enabled in order for quasselcore\n"
                                "to work."));
    exit(1); // TODO make this less brutal (especially for mono client -> popup)
  }
  connect(&_storageSyncTimer, SIGNAL(timeout()), this, SLOT(syncStorage()));
  _storageSyncTimer.start(10 * 60 * 1000); // 10 minutes
}

void Core::init() {
  configured = false;

  CoreSettings cs;

  if(!(configured = initStorage(cs.storageSettings().toMap()))) {
    qWarning() << "Core is currently not configured! Please connect with a Quassel Client for basic setup.";

    // try to migrate old settings
    QVariantMap old = cs.oldDbSettings().toMap();
    if(old.count() && old["Type"].toString().toUpper() == "SQLITE") {
      QVariantMap newSettings;
      newSettings["Backend"] = "SQLite";
      if((configured = initStorage(newSettings))) {
        qWarning() << "...but thankfully I found some old settings to migrate!";
        cs.setStorageSettings(newSettings);
      }
    }
  }

  connect(&_server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  connect(&_v6server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
  if(!startListening()) exit(1); // TODO make this less brutal
}

Core::~Core() {
  foreach(QTcpSocket *socket, blocksizes.keys()) {
    socket->disconnectFromHost();  // disconnect non authed clients
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
  state["CoreStateVersion"] = 1;
  state["ActiveSessions"] = activeSessions;
  s.setCoreState(state);
}

void Core::restoreState() {
  if(!instance()->configured) {
    // qWarning() << qPrintable(tr("Cannot restore a state for an unconfigured core!"));
    return;
  }
  if(instance()->sessions.count()) {
    qWarning() << qPrintable(tr("Calling restoreState() even though active sessions exist!"));
    return;
  }
  CoreSettings s;
  /* We don't check, since we are at the first version since switching to Git
  uint statever = s.coreState().toMap()["CoreStateVersion"].toUInt();
  if(statever < 1) {
    qWarning() << qPrintable(tr("Core state too old, ignoring..."));
    return;
  }
  */
  QVariantList activeSessions = s.coreState().toMap()["ActiveSessions"].toList();
  if(activeSessions.count() > 0) {
    quInfo() << "Restoring previous core state...";
    foreach(QVariant v, activeSessions) {
      UserId user = v.value<UserId>();
      instance()->createSession(user, true);
    }
  }
}

/*** Core Setup ***/
QString Core::setupCoreForInternalUsage() {
  Q_ASSERT(!_storageBackends.isEmpty());
  QVariantMap setupData;
  qsrand(QDateTime::currentDateTime().toTime_t());
  int pass = 0;
  for(int i = 0; i < 10; i++) {
    pass *= 10;
    pass += qrand() % 10;
  }
  setupData["AdminUser"] = "AdminUser";
  setupData["AdminPasswd"] = QString::number(pass);
  setupData["Backend"] = _storageBackends[_storageBackends.keys().first()]->displayName();
  return setupCore(setupData);
}

QString Core::setupCore(QVariantMap setupData) {
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
  quInfo() << qPrintable(tr("Creating admin user..."));
  storage->addUser(user, password);
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
    qCritical() << "Selected storage backend is not available:" << backend;
    return configured = false;
  }
  if(!storage->init(dbSettings)) {
    if(!setup || !(storage->setup(dbSettings) && storage->init(dbSettings))) {
      qCritical() << "Could not init storage!";
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
  if(storage) storage->sync();
}

/*** Storage Access ***/
bool Core::createNetwork(UserId user, NetworkInfo &info) {
  NetworkId networkId = instance()->storage->createNetwork(user, info);
  if(!networkId.isValid())
    return false;

  info.networkId = networkId;
  return true;
}

/*** Network Management ***/

bool Core::startListening() {
  // in mono mode we only start a local port if a port is specified in the cli call
  if(Quassel::runMode() == Quassel::Monolithic && !Quassel::isOptionSet("port"))
    return true;

  bool success = false;
  uint port = Quassel::optionValue("port").toUInt();

  if(_server.listen(QHostAddress::Any, port)) {
    quInfo() << "Listening for GUI clients on IPv4 port" << _server.serverPort()
             << "using protocol version" << Quassel::buildInfo().protocolVersion;
    success = true;
  }
  if(_v6server.listen(QHostAddress::AnyIPv6, port)) {
    quInfo() << "Listening for GUI clients on IPv6 port" << _v6server.serverPort()
             << "using protocol version" << Quassel::buildInfo().protocolVersion;
    success = true;
  }

  if(!success) {
    qCritical() << qPrintable(QString("Could not open GUI client port %1: %2").arg(port).arg(_server.errorString()));
  }

  return success;
}

void Core::stopListening(const QString &reason) {
  bool wasListening = false;
  if(_server.isListening()) {
    wasListening = true;
    _server.close();
  }
  if(_v6server.isListening()) {
    wasListening = true;
    _v6server.close();
  }
  if(wasListening) {
    if(reason.isEmpty())
      quInfo() << "No longer listening for GUI clients.";
    else
      quInfo() << qPrintable(reason);
  }
}

void Core::incomingConnection() {
  QTcpServer *server = qobject_cast<QTcpServer *>(sender());
  Q_ASSERT(server);
  while(server->hasPendingConnections()) {
    QTcpSocket *socket = server->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(clientHasData()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));

    QVariantMap clientInfo;
    blocksizes.insert(socket, (quint32)0);
    quInfo() << qPrintable(tr("Client connected from"))  << qPrintable(socket->peerAddress().toString());

    if(!configured) {
      stopListening(tr("Closing server for basic setup."));
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

    // Just version information -- check it!
    uint ver = msg["ProtocolVersion"].toUInt();
    if(ver < Quassel::buildInfo().coreNeedsProtocol) {
      reply["MsgType"] = "ClientInitReject";
      reply["Error"] = tr("<b>Your Quassel Client is too old!</b><br>"
      "This core needs at least client/core protocol version %1.<br>"
      "Please consider upgrading your client.").arg(Quassel::buildInfo().coreNeedsProtocol);
      SignalProxy::writeDataToDevice(socket, reply);
      qWarning() << qPrintable(tr("Client")) << qPrintable(socket->peerAddress().toString()) << qPrintable(tr("too old, rejecting."));
      socket->close(); return;
    }

    reply["CoreVersion"] = Quassel::buildInfo().fancyVersionString;
    reply["CoreDate"] = Quassel::buildInfo().buildDate;
    reply["ProtocolVersion"] = Quassel::buildInfo().protocolVersion;
    // TODO: Make the core info configurable
    int uptime = startTime().secsTo(QDateTime::currentDateTime().toUTC());
    int updays = uptime / 86400; uptime %= 86400;
    int uphours = uptime / 3600; uptime %= 3600;
    int upmins = uptime / 60;
    reply["CoreInfo"] = tr("<b>Quassel Core Version %1</b><br>"
			   "Built: %2<br>"
			   "Up %3d%4h%5m (since %6)").arg(Quassel::buildInfo().fancyVersionString)
                                                     .arg(Quassel::buildInfo().buildDate)
      .arg(updays).arg(uphours,2,10,QChar('0')).arg(upmins,2,10,QChar('0')).arg(startTime().toString(Qt::TextDate));

#ifdef HAVE_SSL
    SslServer *sslServer = qobject_cast<SslServer *>(&_server);
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
    bool supportSsl = (bool)sslServer && (bool)sslSocket && sslServer->certIsValid();
#else
    bool supportSsl = false;
#endif

#ifndef QT_NO_COMPRESS
    bool supportsCompression = true;
#else
    bool supportsCompression = false;
#endif

    reply["SupportSsl"] = supportSsl;
    reply["SupportsCompression"] = supportsCompression;
    // switch to ssl/compression after client has been informed about our capabilities (see below)

    reply["LoginEnabled"] = true;

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

#ifdef HAVE_SSL
    // after we told the client that we are ssl capable we switch to ssl mode
    if(supportSsl && msg["UseSsl"].toBool()) {
      qDebug() << qPrintable(tr("Starting TLS for Client:"))  << qPrintable(socket->peerAddress().toString());
      connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));
      sslSocket->startServerEncryption();
    }
#endif

#ifndef QT_NO_COMPRESS
    if(supportsCompression && msg["UseCompression"].toBool()) {
      socket->setProperty("UseCompression", true);
      qDebug() << "Using compression for Client:" << qPrintable(socket->peerAddress().toString());
    }
#endif

  } else {
    // for the rest, we need an initialized connection
    if(!clientInfo.contains(socket)) {
      QVariantMap reply;
      reply["MsgType"] = "ClientLoginReject";
      reply["Error"] = tr("<b>Client not initialized!</b><br>You need to send an init message before trying to login.");
      SignalProxy::writeDataToDevice(socket, reply);
      qWarning() << qPrintable(tr("Client")) << qPrintable(socket->peerAddress().toString()) << qPrintable(tr("did not send an init message before trying to login, rejecting."));
      socket->close(); return;
    }
    if(msg["MsgType"] == "CoreSetupData") {
      QVariantMap reply;
      QString result = setupCore(msg["SetupData"].toMap());
      if(!result.isEmpty()) {
        reply["MsgType"] = "CoreSetupReject";
        reply["Error"] = result;
      } else {
        reply["MsgType"] = "CoreSetupAck";
      }
      SignalProxy::writeDataToDevice(socket, reply);
    } else if(msg["MsgType"] == "ClientLogin") {
      QVariantMap reply;
      UserId uid = storage->validateUser(msg["User"].toString(), msg["Password"].toString());
      if(uid == 0) {
        reply["MsgType"] = "ClientLoginReject";
        reply["Error"] = tr("<b>Invalid username or password!</b><br>The username/password combination you supplied could not be found in the database.");
        SignalProxy::writeDataToDevice(socket, reply);
        return;
      }
      reply["MsgType"] = "ClientLoginAck";
      SignalProxy::writeDataToDevice(socket, reply);
      quInfo() << qPrintable(tr("Client")) << qPrintable(socket->peerAddress().toString()) << qPrintable(tr("initialized and authenticated successfully as \"%1\" (UserId: %2).").arg(msg["User"].toString()).arg(uid.toInt()));
      setupClientSession(socket, uid);
    }
  }
}

// Potentially called during the initialization phase (before handing the connection off to the session)
void Core::clientDisconnected() {
  QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
  if(socket) {
    // here it's safe to call methods on socket!
    quInfo() << qPrintable(tr("Non-authed client disconnected.")) << qPrintable(socket->peerAddress().toString());
    blocksizes.remove(socket);
    clientInfo.remove(socket);
    socket->deleteLater();
  } else {
    // we have to crawl through the hashes and see if we find a victim to remove
    qDebug() << qPrintable(tr("Non-authed client disconnected. (socket allready destroyed)"));

    // DO NOT CALL ANY METHODS ON socket!!
    socket = static_cast<QTcpSocket *>(sender());

    QHash<QTcpSocket *, quint32>::iterator blockSizeIter = blocksizes.begin();
    while(blockSizeIter != blocksizes.end()) {
      if(blockSizeIter.key() == socket) {
	blockSizeIter = blocksizes.erase(blockSizeIter);
      } else {
	blockSizeIter++;
      }
    }

    QHash<QTcpSocket *, QVariantMap>::iterator clientInfoIter = clientInfo.begin();
    while(clientInfoIter != clientInfo.end()) {
      if(clientInfoIter.key() == socket) {
	clientInfoIter = clientInfo.erase(clientInfoIter);
      } else {
	clientInfoIter++;
      }
    }
  }


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
    qWarning() << qPrintable(tr("Could not initialize session for client:")) << qPrintable(socket->peerAddress().toString());
    socket->close();
  }
  sess->addClient(socket);
}

void Core::setupInternalClientSession(SignalProxy *proxy) {
  if(!configured) {
    stopListening();
    setupCoreForInternalUsage();
  }

  UserId uid = storage->internalUser();

  // Find or create session for validated user
  SessionThread *sess;
  if(sessions.contains(uid))
    sess = sessions[uid];
  else
    sess = createSession(uid);
  sess->addClient(proxy);
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

#ifdef HAVE_SSL
void Core::sslErrors(const QList<QSslError> &errors) {
  Q_UNUSED(errors);
  QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
  if(socket)
    socket->ignoreSslErrors();
}
#endif

void Core::socketError(QAbstractSocket::SocketError err) {
  QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(sender());
  if(socket && err != QAbstractSocket::RemoteHostClosedError)
    qWarning() << "Core::socketError()" << socket << err << socket->errorString();
}

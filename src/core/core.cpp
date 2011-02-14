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

#include <QCoreApplication>

#include "core.h"
#include "coresession.h"
#include "coresettings.h"
#include "postgresqlstorage.h"
#include "quassel.h"
#include "signalproxy.h"
#include "sqlitestorage.h"
#include "network.h"
#include "logger.h"

#include "util.h"

// migration related
#include <QFile>
#ifdef Q_OS_WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <termios.h>
#endif /* Q_OS_WIN32 */

// umask
#ifndef Q_OS_WIN32
#  include <sys/types.h>
#  include <sys/stat.h>
#endif /* Q_OS_WIN32 */

// ==============================
//  Custom Events
// ==============================
const int Core::AddClientEventId = QEvent::registerEventType();

class AddClientEvent : public QEvent {
public:
  AddClientEvent(QTcpSocket *socket, UserId uid) : QEvent(QEvent::Type(Core::AddClientEventId)), socket(socket), userId(uid) {}
  QTcpSocket *socket;
  UserId userId;
};


// ==============================
//  Core
// ==============================
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

Core::Core()
  : _storage(0)
{
#ifndef Q_OS_WIN32
  umask(S_IRWXG | S_IRWXO);
#endif /* Q_OS_WIN32 */
  _startTime = QDateTime::currentDateTime().toUTC();  // for uptime :)

  Quassel::loadTranslation(QLocale::system());

  // FIXME: MIGRATION 0.3 -> 0.4: Move database and core config to new location
  // Move settings, note this does not delete the old files
#ifdef Q_WS_MAC
    QSettings newSettings("quassel-irc.org", "quasselcore");
#else

# ifdef Q_WS_WIN
    QSettings::Format format = QSettings::IniFormat;
# else
    QSettings::Format format = QSettings::NativeFormat;
# endif
    QString newFilePath = Quassel::configDirPath() + "quasselcore"
    + ((format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
    QSettings newSettings(newFilePath, format);
#endif /* Q_WS_MAC */

  if(newSettings.value("Config/Version").toUInt() == 0) {
#   ifdef Q_WS_MAC
    QString org = "quassel-irc.org";
#   else
    QString org = "Quassel Project";
#   endif
    QSettings oldSettings(org, "Quassel Core");
    if(oldSettings.allKeys().count()) {
      qWarning() << "\n\n*** IMPORTANT: Config and data file locations have changed. Attempting to auto-migrate your core settings...";
      foreach(QString key, oldSettings.allKeys())
        newSettings.setValue(key, oldSettings.value(key));
      newSettings.setValue("Config/Version", 1);
      qWarning() << "*   Your core settings have been migrated to" << newSettings.fileName();

#ifndef Q_WS_MAC /* we don't need to move the db and cert for mac */
#ifdef Q_OS_WIN32
      QString quasselDir = qgetenv("APPDATA") + "/quassel/";
#elif defined Q_WS_MAC
      QString quasselDir = QDir::homePath() + "/Library/Application Support/Quassel/";
#else
      QString quasselDir = QDir::homePath() + "/.quassel/";
#endif

      QFileInfo info(Quassel::configDirPath() + "quassel-storage.sqlite");
      if(!info.exists()) {
      // move database, if we found it
        QFile oldDb(quasselDir + "quassel-storage.sqlite");
        if(oldDb.exists()) {
          bool success = oldDb.rename(Quassel::configDirPath() + "quassel-storage.sqlite");
          if(success)
            qWarning() << "*   Your database has been moved to" << Quassel::configDirPath() + "quassel-storage.sqlite";
          else
            qWarning() << "!!! Moving your database has failed. Please move it manually into" << Quassel::configDirPath();
        }
      }
      // move certificate
      QFileInfo certInfo(quasselDir + "quasselCert.pem");
      if(certInfo.exists()) {
        QFile cert(quasselDir + "quasselCert.pem");
        bool success = cert.rename(Quassel::configDirPath() + "quasselCert.pem");
        if(success)
          qWarning() << "*   Your certificate has been moved to" << Quassel::configDirPath() + "quasselCert.pem";
        else
          qWarning() << "!!! Moving your certificate has failed. Please move it manually into" << Quassel::configDirPath();
      }
#endif /* !Q_WS_MAC */
      qWarning() << "*** Migration completed.\n\n";
    }
  }
  // MIGRATION end

  // check settings version
  // so far, we only have 1
  CoreSettings s;
  if(s.version() != 1) {
    qCritical() << "Invalid core settings version, terminating!";
    exit(EXIT_FAILURE);
  }

  registerStorageBackends();

  connect(&_storageSyncTimer, SIGNAL(timeout()), this, SLOT(syncStorage()));
  _storageSyncTimer.start(10 * 60 * 1000); // 10 minutes
}

void Core::init() {
  CoreSettings cs;
  _configured = initStorage(cs.storageSettings().toMap());

  if(Quassel::isOptionSet("select-backend")) {
    selectBackend(Quassel::optionValue("select-backend"));
    exit(0);
  }

  if(!_configured) {
    if(!_storageBackends.count()) {
      qWarning() << qPrintable(tr("Could not initialize any storage backend! Exiting..."));
      qWarning() << qPrintable(tr("Currently, Quassel supports SQLite3 and PostgreSQL. You need to build your\n"
				  "Qt library with the sqlite or postgres plugin enabled in order for quasselcore\n"
				  "to work."));
      exit(1); // TODO make this less brutal (especially for mono client -> popup)
    }
    qWarning() << "Core is currently not configured! Please connect with a Quassel Client for basic setup.";
  }

  if(Quassel::isOptionSet("add-user")) {
    createUser();
    exit(0);
  }

  if(Quassel::isOptionSet("change-userpass")) {
    changeUserPass(Quassel::optionValue("change-userpass"));
    exit(0);
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
  if(!instance()->_configured) {
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
  setupData["Backend"] = QString("SQLite"); // mono client currently needs sqlite
  return setupCore(setupData);
}

QString Core::setupCore(QVariantMap setupData) {
  QString user = setupData.take("AdminUser").toString();
  QString password = setupData.take("AdminPasswd").toString();
  if(user.isEmpty() || password.isEmpty()) {
    return tr("Admin user or password not set.");
  }
  if(_configured || !(_configured = initStorage(setupData, true))) {
    return tr("Could not setup storage!");
  }
  CoreSettings s;
  s.setStorageSettings(setupData);
  quInfo() << qPrintable(tr("Creating admin user..."));
  _storage->addUser(user, password);
  startListening();  // TODO check when we need this
  return QString();
}

/*** Storage Handling ***/
void Core::registerStorageBackends() {
  // Register storage backends here!
  registerStorageBackend(new SqliteStorage(this));
  registerStorageBackend(new PostgreSqlStorage(this));
}

bool Core::registerStorageBackend(Storage *backend) {
  if(backend->isAvailable()) {
    _storageBackends[backend->displayName()] = backend;
    return true;
  } else {
    backend->deleteLater();
    return false;
  }
}

void Core::unregisterStorageBackends() {
  foreach(Storage *s, _storageBackends.values()) {
    s->deleteLater();
  }
  _storageBackends.clear();
}

void Core::unregisterStorageBackend(Storage *backend) {
  _storageBackends.remove(backend->displayName());
  backend->deleteLater();
}

// old db settings:
// "Type" => "sqlite"
bool Core::initStorage(const QString &backend, QVariantMap settings, bool setup) {
  _storage = 0;

  if(backend.isEmpty()) {
    return false;
  }

  Storage *storage = 0;
  if(_storageBackends.contains(backend)) {
    storage = _storageBackends[backend];
  } else {
    qCritical() << "Selected storage backend is not available:" << backend;
    return false;
  }

  Storage::State storageState = storage->init(settings);
  switch(storageState) {
  case Storage::NeedsSetup:
    if(!setup)
      return false; // trigger setup process
    if(storage->setup(settings))
      return initStorage(backend, settings, false);
    // if setup wasn't successfull we mark the backend as unavailable
  case Storage::NotAvailable:
    qCritical() << "Selected storage backend is not available:" << backend;
    storage->deleteLater();
    _storageBackends.remove(backend);
    storage = 0;
    return false;
  case Storage::IsReady:
    // delete all other backends
    _storageBackends.remove(backend);
    unregisterStorageBackends();
    connect(storage, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)), this, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)));
  }
  _storage = storage;
  return true;
}

bool Core::initStorage(QVariantMap dbSettings, bool setup) {
  return initStorage(dbSettings["Backend"].toString(), dbSettings["ConnectionProperties"].toMap(), setup);
}


void Core::syncStorage() {
  if(_storage)
    _storage->sync();
}

/*** Storage Access ***/
bool Core::createNetwork(UserId user, NetworkInfo &info) {
  NetworkId networkId = instance()->_storage->createNetwork(user, info);
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

  const QString listen = Quassel::optionValue("listen");
  const QStringList listen_list = listen.split(",", QString::SkipEmptyParts);
  if(listen_list.size() > 0) {
    foreach (const QString listen_term, listen_list) {  // TODO: handle multiple interfaces for same TCP version gracefully
      QHostAddress addr;
      if(!addr.setAddress(listen_term)) {
        qCritical() << qPrintable(
          tr("Invalid listen address %1")
            .arg(listen_term)
        );
      } else {
        switch(addr.protocol()) {
          case QAbstractSocket::IPv6Protocol:
            if(_v6server.listen(addr, port)) {
              quInfo() << qPrintable(
                tr("Listening for GUI clients on IPv6 %1 port %2 using protocol version %3")
                  .arg(addr.toString())
                  .arg(_v6server.serverPort())
                  .arg(Quassel::buildInfo().protocolVersion)
              );
              success = true;
            } else
              quWarning() << qPrintable(
                tr("Could not open IPv6 interface %1:%2: %3")
                  .arg(addr.toString())
                  .arg(port)
                  .arg(_v6server.errorString()));
            break;
          case QAbstractSocket::IPv4Protocol:
            if(_server.listen(addr, port)) {
              quInfo() << qPrintable(
                tr("Listening for GUI clients on IPv4 %1 port %2 using protocol version %3")
                  .arg(addr.toString())
                  .arg(_server.serverPort())
                  .arg(Quassel::buildInfo().protocolVersion)
              );
              success = true;
            } else {
              // if v6 succeeded on Any, the port will be already in use - don't display the error then
              if(!success || _server.serverError() != QAbstractSocket::AddressInUseError)
                quWarning() << qPrintable(
                  tr("Could not open IPv4 interface %1:%2: %3")
                  .arg(addr.toString())
                  .arg(port)
                  .arg(_server.errorString()));
            }
            break;
          default:
            qCritical() << qPrintable(
              tr("Invalid listen address %1, unknown network protocol")
                  .arg(listen_term)
            );
            break;
        }
      }
    }
  }
  if(!success)
    quError() << qPrintable(tr("Could not open any network interfaces to listen on!"));

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

    if(!_configured) {
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

    reply["ProtocolVersion"] = Quassel::buildInfo().protocolVersion;
    reply["CoreVersion"] = Quassel::buildInfo().fancyVersionString;
    reply["CoreDate"] = Quassel::buildInfo().buildDate;
    reply["CoreStartTime"] = startTime(); // v10 clients don't necessarily parse this, see below

    // FIXME: newer clients no longer use the hardcoded CoreInfo (for now), since it gets the
    //        time zone wrong. With the next protocol bump (10 -> 11), we should remove this
    //        or make it properly configurable.

    int uptime = startTime().secsTo(QDateTime::currentDateTime().toUTC());
    int updays = uptime / 86400; uptime %= 86400;
    int uphours = uptime / 3600; uptime %= 3600;
    int upmins = uptime / 60;
    reply["CoreInfo"] = tr("<b>Quassel Core Version %1</b><br>"
			   "Built: %2<br>"
			   "Up %3d%4h%5m (since %6)").arg(Quassel::buildInfo().fancyVersionString)
                                                     .arg(Quassel::buildInfo().buildDate)
      .arg(updays).arg(uphours,2,10,QChar('0')).arg(upmins,2,10,QChar('0')).arg(startTime().toString(Qt::TextDate));

    reply["CoreFeatures"] = (int)Quassel::features();

#ifdef HAVE_SSL
    SslServer *sslServer = qobject_cast<SslServer *>(&_server);
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
    bool supportSsl = (bool)sslServer && (bool)sslSocket && sslServer->isCertValid();
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
    if(!_configured) {
      reply["Configured"] = false;
      QList<QVariant> backends;
      foreach(Storage *backend, _storageBackends.values()) {
        QVariantMap v;
        v["DisplayName"] = backend->displayName();
        v["Description"] = backend->description();
	v["SetupKeys"] = backend->setupKeys();
	v["SetupDefaults"] = backend->setupDefaults();
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
    socket->flush(); // ensure that the write cache is flushed before we switch to ssl

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
      UserId uid = _storage->validateUser(msg["User"].toString(), msg["Password"].toString());
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
  if (!_configured) {
    startListening();
  }

  // TODO remove unneeded sessions - if necessary/possible...
  // Suggestion: kill sessions if they are not connected to any network and client.
}

void Core::setupClientSession(QTcpSocket *socket, UserId uid) {
  // From now on everything is handled by the client session
  disconnect(socket, 0, this, 0);
  socket->flush();
  blocksizes.remove(socket);
  clientInfo.remove(socket);

  // Find or create session for validated user
  SessionThread *session;
  if(sessions.contains(uid)) {
    session = sessions[uid];
  } else {
    session = createSession(uid);
    if(!session) {
      qWarning() << qPrintable(tr("Could not initialize session for client:")) << qPrintable(socket->peerAddress().toString());
      socket->close();
      return;
    }
  }

  // as we are currently handling an event triggered by incoming data on this socket
  // it is unsafe to directly move the socket to the client thread.
  QCoreApplication::postEvent(this, new AddClientEvent(socket, uid));
}

void Core::customEvent(QEvent *event) {
  if(event->type() == AddClientEventId) {
    AddClientEvent *addClientEvent = static_cast<AddClientEvent *>(event);
    addClientHelper(addClientEvent->socket, addClientEvent->userId);
    return;
  }
}

void Core::addClientHelper(QTcpSocket *socket, UserId uid) {
  // Find or create session for validated user
  if(!sessions.contains(uid)) {
    qWarning() << qPrintable(tr("Could not find a session for client:")) << qPrintable(socket->peerAddress().toString());
    socket->close();
    return;
  }

  SessionThread *session = sessions[uid];
  session->addClient(socket);
}

void Core::setupInternalClientSession(SignalProxy *proxy) {
  if(!_configured) {
    stopListening();
    setupCoreForInternalUsage();
  }

  UserId uid;
  if(_storage) {
    uid = _storage->internalUser();
  } else {
    qWarning() << "Core::setupInternalClientSession(): You're trying to run monolithic Quassel with an unusable Backend! Go fix it!";
    return;
  }

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

// migration / backend selection
bool Core::selectBackend(const QString &backend) {
  // reregister all storage backends
  registerStorageBackends();
  if(!_storageBackends.contains(backend)) {
    qWarning() << qPrintable(QString("Core::selectBackend(): unsupported backend: %1").arg(backend));
    qWarning() << "    supported backends are:" << qPrintable(QStringList(_storageBackends.keys()).join(", "));
    return false;
  }

  Storage *storage = _storageBackends[backend];
  QVariantMap settings = promptForSettings(storage);

  Storage::State storageState = storage->init(settings);
  switch(storageState) {
  case Storage::IsReady:
    saveBackendSettings(backend, settings);
    qWarning() << "Switched backend to:" << qPrintable(backend);
    qWarning() << "Backend already initialized. Skipping Migration";
    return true;
  case Storage::NotAvailable:
    qCritical() << "Backend is not available:" << qPrintable(backend);
    return false;
  case Storage::NeedsSetup:
    if(!storage->setup(settings)) {
      qWarning() << qPrintable(QString("Core::selectBackend(): unable to setup backend: %1").arg(backend));
      return false;
    }

    if(storage->init(settings) != Storage::IsReady) {
      qWarning() << qPrintable(QString("Core::migrateBackend(): unable to initialize backend: %1").arg(backend));
      return false;
    }

    saveBackendSettings(backend, settings);
    qWarning() << "Switched backend to:" << qPrintable(backend);
    break;
  }

  // let's see if we have a current storage object we can migrate from
  AbstractSqlMigrationReader *reader = getMigrationReader(_storage);
  AbstractSqlMigrationWriter *writer = getMigrationWriter(storage);
  if(reader && writer) {
    qDebug() << qPrintable(QString("Migrating Storage backend %1 to %2...").arg(_storage->displayName(), storage->displayName()));
    delete _storage;
    _storage = 0;
    delete storage;
    storage = 0;
    if(reader->migrateTo(writer)) {
      qDebug() << "Migration finished!";
      saveBackendSettings(backend, settings);
      return true;
    }
    return false;
    qWarning() << qPrintable(QString("Core::migrateDb(): unable to migrate storage backend! (No migration writer for %1)").arg(backend));
  }

  // inform the user why we cannot merge
  if(!_storage) {
    qWarning() << "No currently active backend. Skipping migration.";
  } else if(!reader) {
    qWarning() << "Currently active backend does not support migration:" << qPrintable(_storage->displayName());
  }
  if(writer) {
    qWarning() << "New backend does not support migration:" << qPrintable(backend);
  }

  // so we were unable to merge, but let's create a user \o/
  _storage = storage;
  createUser();
  return true;
}

void Core::createUser() {
  QTextStream out(stdout);
  QTextStream in(stdin);
  out << "Add a new user:" << endl;
  out << "Username: ";
  out.flush();
  QString username = in.readLine().trimmed();

  disableStdInEcho();
  out << "Password: ";
  out.flush();
  QString password = in.readLine().trimmed();
  out << endl;
  out << "Repeat Password: ";
  out.flush();
  QString password2 = in.readLine().trimmed();
  out << endl;
  enableStdInEcho();

  if(password != password2) {
    qWarning() << "Passwords don't match!";
    return;
  }
  if(password.isEmpty()) {
    qWarning() << "Password is empty!";
    return;
  }

  if(_configured && _storage->addUser(username, password).isValid()) {
    out << "Added user " << username << " successfully!" << endl;
  } else {
    qWarning() << "Unable to add user:" << qPrintable(username);
  }
}

void Core::changeUserPass(const QString &username) {
  QTextStream out(stdout);
  QTextStream in(stdin);
  UserId userId = _storage->getUserId(username);
  if(!userId.isValid()) {
    out << "User " << username << " does not exist." << endl;
    return;
  }

  out << "Change password for user: " << username << endl;

  disableStdInEcho();
  out << "New Password: ";
  out.flush();
  QString password = in.readLine().trimmed();
  out << endl;
  out << "Repeat Password: ";
  out.flush();
  QString password2 = in.readLine().trimmed();
  out << endl;
  enableStdInEcho();

  if(password != password2) {
    qWarning() << "Passwords don't match!";
    return;
  }
  if(password.isEmpty()) {
    qWarning() << "Password is empty!";
    return;
  }

  if(_configured && _storage->updateUser(userId, password)) {
    out << "Password changed successfully!" << endl;
  } else {
    qWarning() << "Failed to change password!";
  }
}

AbstractSqlMigrationReader *Core::getMigrationReader(Storage *storage) {
  if(!storage)
    return 0;

  AbstractSqlStorage *sqlStorage = qobject_cast<AbstractSqlStorage *>(storage);
  if(!sqlStorage) {
    qDebug() << "Core::migrateDb(): only SQL based backends can be migrated!";
    return 0;
  }

  return sqlStorage->createMigrationReader();
}

AbstractSqlMigrationWriter *Core::getMigrationWriter(Storage *storage) {
  if(!storage)
    return 0;

  AbstractSqlStorage *sqlStorage = qobject_cast<AbstractSqlStorage *>(storage);
  if(!sqlStorage) {
    qDebug() << "Core::migrateDb(): only SQL based backends can be migrated!";
    return 0;
  }

  return sqlStorage->createMigrationWriter();
}

void Core::saveBackendSettings(const QString &backend, const QVariantMap &settings) {
  QVariantMap dbsettings;
  dbsettings["Backend"] = backend;
  dbsettings["ConnectionProperties"] = settings;
  CoreSettings().setStorageSettings(dbsettings);
}

QVariantMap Core::promptForSettings(const Storage *storage) {
  QVariantMap settings;

  QStringList keys = storage->setupKeys();
  if(keys.isEmpty())
    return settings;

  QTextStream out(stdout);
  QTextStream in(stdin);
  out << "Default values are in brackets" << endl;

  QVariantMap defaults = storage->setupDefaults();
  QString value;
  foreach(QString key, keys) {
    QVariant val;
    if(defaults.contains(key)) {
      val = defaults[key];
    }
    out << key;
    if(!val.toString().isEmpty()) {
      out << " (" << val.toString() << ")";
    }
    out << ": ";
    out.flush();

    bool noEcho = QString("password").toLower().startsWith(key.toLower());
    if(noEcho) {
      disableStdInEcho();
    }
    value = in.readLine().trimmed();
    if(noEcho) {
      out << endl;
      enableStdInEcho();
    }

    if(!value.isEmpty()) {
      switch(defaults[key].type()) {
      case QVariant::Int:
	val = QVariant(value.toInt());
	break;
      default:
	val = QVariant(value);
      }
    }
    settings[key] = val;
  }
  return settings;
}


#ifdef Q_OS_WIN32
void Core::stdInEcho(bool on) {
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(hStdin, &mode);
  if(on)
    mode |= ENABLE_ECHO_INPUT;
  else
    mode &= ~ENABLE_ECHO_INPUT;
  SetConsoleMode(hStdin, mode);
}
#else
void Core::stdInEcho(bool on) {
  termios t;
  tcgetattr(STDIN_FILENO, &t);
  if(on)
    t.c_lflag |= ECHO;
  else
    t.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}
#endif /* Q_OS_WIN32 */

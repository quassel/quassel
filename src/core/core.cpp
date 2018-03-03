/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <algorithm>

#include <QCoreApplication>

#include "core.h"
#include "coreauthhandler.h"
#include "coresession.h"
#include "coresettings.h"
#include "logger.h"
#include "internalpeer.h"
#include "network.h"
#include "postgresqlstorage.h"
#include "quassel.h"
#include "sqlauthenticator.h"
#include "sqlitestorage.h"
#include "util.h"

// Currently building with LDAP bindings is optional.
#ifdef HAVE_LDAP
#include "ldapauthenticator.h"
#endif

// migration related
#include <QFile>
#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <unistd.h>
#  include <termios.h>
#endif /* Q_OS_WIN */

#ifdef HAVE_UMASK
#  include <sys/types.h>
#  include <sys/stat.h>
#endif /* HAVE_UMASK */

// ==============================
//  Custom Events
// ==============================
const int Core::AddClientEventId = QEvent::registerEventType();

class AddClientEvent : public QEvent
{
public:
    AddClientEvent(RemotePeer *p, UserId uid) : QEvent(QEvent::Type(Core::AddClientEventId)), peer(p), userId(uid) {}
    RemotePeer *peer;
    UserId userId;
};


// ==============================
//  Core
// ==============================
Core *Core::instanceptr = 0;

Core *Core::instance()
{
    if (instanceptr) return instanceptr;
    instanceptr = new Core();
    instanceptr->init();
    return instanceptr;
}


void Core::destroy()
{
    delete instanceptr;
    instanceptr = 0;
}


Core::Core()
{
#ifdef HAVE_UMASK
    umask(S_IRWXG | S_IRWXO);
#endif
    _startTime = QDateTime::currentDateTime().toUTC(); // for uptime :)

    Quassel::loadTranslation(QLocale::system());

    // FIXME: MIGRATION 0.3 -> 0.4: Move database and core config to new location
    // Move settings, note this does not delete the old files
#ifdef Q_OS_MAC
    QSettings newSettings("quassel-irc.org", "quasselcore");
#else

# ifdef Q_OS_WIN
    QSettings::Format format = QSettings::IniFormat;
# else
    QSettings::Format format = QSettings::NativeFormat;
# endif
    QString newFilePath = Quassel::configDirPath() + "quasselcore"
                          + ((format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
    QSettings newSettings(newFilePath, format);
#endif /* Q_OS_MAC */

    if (newSettings.value("Config/Version").toUInt() == 0) {
#   ifdef Q_OS_MAC
        QString org = "quassel-irc.org";
#   else
        QString org = "Quassel Project";
#   endif
        QSettings oldSettings(org, "Quassel Core");
        if (oldSettings.allKeys().count()) {
            quWarning() << "\n\n*** IMPORTANT: Config and data file locations have changed. Attempting to auto-migrate your core settings...";
            foreach(QString key, oldSettings.allKeys())
            newSettings.setValue(key, oldSettings.value(key));
            newSettings.setValue("Config/Version", 1);
            quWarning() << "*   Your core settings have been migrated to" << newSettings.fileName();

#ifndef Q_OS_MAC /* we don't need to move the db and cert for mac */
#ifdef Q_OS_WIN
            QString quasselDir = qgetenv("APPDATA") + "/quassel/";
#elif defined Q_OS_MAC
            QString quasselDir = QDir::homePath() + "/Library/Application Support/Quassel/";
#else
            QString quasselDir = QDir::homePath() + "/.quassel/";
#endif

            QFileInfo info(Quassel::configDirPath() + "quassel-storage.sqlite");
            if (!info.exists()) {
                // move database, if we found it
                QFile oldDb(quasselDir + "quassel-storage.sqlite");
                if (oldDb.exists()) {
                    bool success = oldDb.rename(Quassel::configDirPath() + "quassel-storage.sqlite");
                    if (success)
                        quWarning() << "*   Your database has been moved to" << Quassel::configDirPath() + "quassel-storage.sqlite";
                    else
                        quWarning() << "!!! Moving your database has failed. Please move it manually into" << Quassel::configDirPath();
                }
            }
            // move certificate
            QFileInfo certInfo(quasselDir + "quasselCert.pem");
            if (certInfo.exists()) {
                QFile cert(quasselDir + "quasselCert.pem");
                bool success = cert.rename(Quassel::configDirPath() + "quasselCert.pem");
                if (success)
                    quWarning() << "*   Your certificate has been moved to" << Quassel::configDirPath() + "quasselCert.pem";
                else
                    quWarning() << "!!! Moving your certificate has failed. Please move it manually into" << Quassel::configDirPath();
            }
#endif /* !Q_OS_MAC */
            quWarning() << "*** Migration completed.\n\n";
        }
    }
    // MIGRATION end

    // check settings version
    // so far, we only have 1
    CoreSettings s;
    if (s.version() != 1) {
        qCritical() << "Invalid core settings version, terminating!";
        exit(EXIT_FAILURE);
    }

    // Set up storage and authentication backends
    registerStorageBackends();
    registerAuthenticators();

    connect(&_storageSyncTimer, SIGNAL(timeout()), this, SLOT(syncStorage()));
    _storageSyncTimer.start(10 * 60 * 1000); // 10 minutes
}


void Core::init()
{
    CoreSettings cs;
    // legacy
    QVariantMap dbsettings = cs.storageSettings().toMap();
    _configured = initStorage(dbsettings.value("Backend").toString(), dbsettings.value("ConnectionProperties").toMap());

    // Not entirely sure what is 'legacy' about the above, but it seems to be the way things work!
    if (_configured) {
        QVariantMap authSettings = cs.authSettings().toMap();
        initAuthenticator(authSettings.value("Authenticator", "Database").toString(), authSettings.value("AuthProperties").toMap());
    }

    if (Quassel::isOptionSet("select-backend") || Quassel::isOptionSet("select-authenticator")) {
        if (Quassel::isOptionSet("select-backend")) {
            selectBackend(Quassel::optionValue("select-backend"));
        }
        if (Quassel::isOptionSet("select-authenticator")) {
            selectAuthenticator(Quassel::optionValue("select-authenticator"));
        }
        exit(EXIT_SUCCESS);
    }

    if (!_configured) {
        if (_registeredStorageBackends.size() == 0) {
            quWarning() << qPrintable(tr("Could not initialize any storage backend! Exiting..."));
            quWarning() << qPrintable(tr("Currently, Quassel supports SQLite3 and PostgreSQL. You need to build your\n"
                                        "Qt library with the sqlite or postgres plugin enabled in order for quasselcore\n"
                                        "to work."));
            exit(EXIT_FAILURE); // TODO make this less brutal (especially for mono client -> popup)
        }
        quWarning() << "Core is currently not configured! Please connect with a Quassel Client for basic setup.";

        if (!cs.isWritable()) {
            qWarning() << "Cannot write quasselcore configuration; probably a permission problem.";
            exit(EXIT_FAILURE);
        }

    }

    if (Quassel::isOptionSet("add-user")) {
        exit(createUser() ? EXIT_SUCCESS : EXIT_FAILURE);

    }

    if (Quassel::isOptionSet("change-userpass")) {
        exit(changeUserPass(Quassel::optionValue("change-userpass")) ?
                       EXIT_SUCCESS : EXIT_FAILURE);
    }

    connect(&_server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
    connect(&_v6server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
    if (!startListening()) exit(1);  // TODO make this less brutal

    if (Quassel::isOptionSet("oidentd")) {
        _oidentdConfigGenerator = new OidentdConfigGenerator(Quassel::isOptionSet("oidentd-strict"), this);
        if (Quassel::isOptionSet("oidentd-strict")) {
            cacheSysIdent();
        }
    }
}


Core::~Core()
{
    // FIXME do we need more cleanup for handlers?
    foreach(CoreAuthHandler *handler, _connectingClients) {
        handler->deleteLater(); // disconnect non authed clients
    }
    qDeleteAll(_sessions);
}


/*** Session Restore ***/

void Core::saveState()
{
    QVariantList activeSessions;
    foreach(UserId user, instance()->_sessions.keys())
        activeSessions << QVariant::fromValue<UserId>(user);
    instance()->_storage->setCoreState(activeSessions);
}


void Core::restoreState()
{
    if (!instance()->_configured) {
        // quWarning() << qPrintable(tr("Cannot restore a state for an unconfigured core!"));
        return;
    }
    if (instance()->_sessions.count()) {
        quWarning() << qPrintable(tr("Calling restoreState() even though active sessions exist!"));
        return;
    }
    CoreSettings s;
    /* We don't check, since we are at the first version since switching to Git
    uint statever = s.coreState().toMap()["CoreStateVersion"].toUInt();
    if(statever < 1) {
      quWarning() << qPrintable(tr("Core state too old, ignoring..."));
      return;
    }
    */

    const QList<QVariant> &activeSessionsFallback = s.coreState().toMap()["ActiveSessions"].toList();
    QVariantList activeSessions = instance()->_storage->getCoreState(activeSessionsFallback);

    if (activeSessions.count() > 0) {
        quInfo() << "Restoring previous core state...";
        foreach(QVariant v, activeSessions) {
            UserId user = v.value<UserId>();
            instance()->sessionForUser(user, true);
        }
    }
}


/*** Core Setup ***/

QString Core::setup(const QString &adminUser, const QString &adminPassword, const QString &backend, const QVariantMap &setupData, const QString &authenticator, const QVariantMap &authSetupData)
{
    return instance()->setupCore(adminUser, adminPassword, backend, setupData, authenticator, authSetupData);
}


QString Core::setupCore(const QString &adminUser, const QString &adminPassword, const QString &backend, const QVariantMap &setupData, const QString &authenticator, const QVariantMap &authSetupData)
{
    if (_configured)
        return tr("Core is already configured! Not configuring again...");

    if (adminUser.isEmpty() || adminPassword.isEmpty()) {
        return tr("Admin user or password not set.");
    }
    if (!(_configured = initStorage(backend, setupData, true))) {
        return tr("Could not setup storage!");
    }

    quInfo() << "Selected authenticator:" << authenticator;
    if (!(_configured = initAuthenticator(authenticator, authSetupData, true)))
    {
        return tr("Could not setup authenticator!");
    }

    if (!saveBackendSettings(backend, setupData)) {
        return tr("Could not save backend settings, probably a permission problem.");
    }
    saveAuthenticatorSettings(authenticator, authSetupData);

    quInfo() << qPrintable(tr("Creating admin user..."));
    _storage->addUser(adminUser, adminPassword);
    cacheSysIdent();
    startListening(); // TODO check when we need this
    return QString();
}


QString Core::setupCoreForInternalUsage()
{
    Q_ASSERT(!_registeredStorageBackends.empty());

    qsrand(QDateTime::currentDateTime().toTime_t());
    int pass = 0;
    for (int i = 0; i < 10; i++) {
        pass *= 10;
        pass += qrand() % 10;
    }

    // mono client currently needs sqlite
    return setupCore("AdminUser", QString::number(pass), "SQLite", QVariantMap(), "Database", QVariantMap());
}


/*** Storage Handling ***/

template<typename Storage>
void Core::registerStorageBackend()
{
    auto backend = makeDeferredShared<Storage>(this);
    if (backend->isAvailable())
        _registeredStorageBackends.emplace_back(std::move(backend));
    else
        backend->deleteLater();
}


void Core::registerStorageBackends()
{
    if (_registeredStorageBackends.empty()) {
        registerStorageBackend<SqliteStorage>();
        registerStorageBackend<PostgreSqlStorage>();
    }
}


DeferredSharedPtr<Storage> Core::storageBackend(const QString &backendId) const
{
    auto it = std::find_if(_registeredStorageBackends.begin(), _registeredStorageBackends.end(),
                           [backendId](const DeferredSharedPtr<Storage> &backend) {
                               return backend->displayName() == backendId;
                           });
    return it != _registeredStorageBackends.end() ? *it : nullptr;
}

// old db settings:
// "Type" => "sqlite"
bool Core::initStorage(const QString &backend, const QVariantMap &settings, bool setup)
{
    if (backend.isEmpty()) {
        quWarning() << "No storage backend selected!";
        return false;
    }

    auto storage = storageBackend(backend);
    if (!storage) {
        qCritical() << "Selected storage backend is not available:" << backend;
        return false;
    }

    Storage::State storageState = storage->init(settings);
    switch (storageState) {
    case Storage::NeedsSetup:
        if (!setup)
            return false;  // trigger setup process
        if (storage->setup(settings))
            return initStorage(backend, settings, false);
        return false;

    // if initialization wasn't successful, we quit to keep from coming up unconfigured
    case Storage::NotAvailable:
        qCritical() << "FATAL: Selected storage backend is not available:" << backend;
        if (!setup)
            exit(EXIT_FAILURE);
        return false;

    case Storage::IsReady:
        // delete all other backends
        _registeredStorageBackends.clear();
        connect(storage.get(), SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)),
                this, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)));
        break;
    }
    _storage = std::move(storage);
    return true;
}


void Core::syncStorage()
{
    if (_storage)
        _storage->sync();
}


/*** Storage Access ***/
bool Core::createNetwork(UserId user, NetworkInfo &info)
{
    NetworkId networkId = instance()->_storage->createNetwork(user, info);
    if (!networkId.isValid())
        return false;

    info.networkId = networkId;
    return true;
}


/*** Authenticators ***/

// Authentication handling, now independent from storage.
template<typename Authenticator>
void Core::registerAuthenticator()
{
    auto authenticator = makeDeferredShared<Authenticator>(this);
    if (authenticator->isAvailable())
        _registeredAuthenticators.emplace_back(std::move(authenticator));
    else
        authenticator->deleteLater();
}


void Core::registerAuthenticators()
{
    if (_registeredAuthenticators.empty()) {
        registerAuthenticator<SqlAuthenticator>();
#ifdef HAVE_LDAP
        registerAuthenticator<LdapAuthenticator>();
#endif
    }
}


DeferredSharedPtr<Authenticator> Core::authenticator(const QString &backendId) const
{
    auto it = std::find_if(_registeredAuthenticators.begin(), _registeredAuthenticators.end(),
                           [backendId](const DeferredSharedPtr<Authenticator> &authenticator) {
                               return authenticator->backendId() == backendId;
                           });
    return it != _registeredAuthenticators.end() ? *it : nullptr;
}


// FIXME: Apparently, this is the legacy way of initting storage backends?
// If there's a not-legacy way, it should be used here
bool Core::initAuthenticator(const QString &backend, const QVariantMap &settings, bool setup)
{
    if (backend.isEmpty()) {
        quWarning() << "No authenticator selected!";
        return false;
    }

    auto auth = authenticator(backend);
    if (!auth) {
        qCritical() << "Selected auth backend is not available:" << backend;
        return false;
    }

    Authenticator::State authState = auth->init(settings);
    switch (authState) {
    case Authenticator::NeedsSetup:
        if (!setup)
            return false;  // trigger setup process
        if (auth->setup(settings))
            return initAuthenticator(backend, settings, false);
        return false;

    // if initialization wasn't successful, we quit to keep from coming up unconfigured
    case Authenticator::NotAvailable:
        qCritical() << "FATAL: Selected auth backend is not available:" << backend;
        if (!setup)
            exit(EXIT_FAILURE);
        return false;

    case Authenticator::IsReady:
        // delete all other backends
        _registeredAuthenticators.clear();
        break;
    }
    _authenticator = std::move(auth);
    return true;
}


/*** Network Management ***/

bool Core::sslSupported()
{
#ifdef HAVE_SSL
    SslServer *sslServer = qobject_cast<SslServer *>(&instance()->_server);
    return sslServer && sslServer->isCertValid();
#else
    return false;
#endif
}


bool Core::reloadCerts()
{
#ifdef HAVE_SSL
    SslServer *sslServerv4 = qobject_cast<SslServer *>(&instance()->_server);
    bool retv4 = sslServerv4->reloadCerts();

    SslServer *sslServerv6 = qobject_cast<SslServer *>(&instance()->_v6server);
    bool retv6 = sslServerv6->reloadCerts();

    return retv4 && retv6;
#else
    // SSL not supported, don't mark configuration reload as failed
    return true;
#endif
}


void Core::cacheSysIdent()
{
    if (isConfigured()) {
        instance()->_authUserNames = instance()->_storage->getAllAuthUserNames();
    }
}


QString Core::strictSysIdent(UserId user) const
{
    if (_authUserNames.contains(user)) {
        return _authUserNames[user];
    }

    // A new user got added since we last pulled our cache from the database.
    // There's no way to avoid a database hit - we don't even know the authname!
    cacheSysIdent();

    if (_authUserNames.contains(user)) {
        return _authUserNames[user];
    }

    // ...something very weird is going on if we ended up here (an active CoreSession without a corresponding database entry?)
    qWarning().nospace() << "Unable to find authusername for UserId " << user << ", this should never happen!";
    return "unknown"; // Should we just terminate the program instead?
}


bool Core::startListening()
{
    // in mono mode we only start a local port if a port is specified in the cli call
    if (Quassel::runMode() == Quassel::Monolithic && !Quassel::isOptionSet("port"))
        return true;

    bool success = false;
    uint port = Quassel::optionValue("port").toUInt();

    const QString listen = Quassel::optionValue("listen");
    const QStringList listen_list = listen.split(",", QString::SkipEmptyParts);
    if (listen_list.size() > 0) {
        foreach(const QString listen_term, listen_list) { // TODO: handle multiple interfaces for same TCP version gracefully
            QHostAddress addr;
            if (!addr.setAddress(listen_term)) {
                qCritical() << qPrintable(
                    tr("Invalid listen address %1")
                    .arg(listen_term)
                    );
            }
            else {
                switch (addr.protocol()) {
                case QAbstractSocket::IPv6Protocol:
                    if (_v6server.listen(addr, port)) {
                        quInfo() << qPrintable(
                            tr("Listening for GUI clients on IPv6 %1 port %2 using protocol version %3")
                            .arg(addr.toString())
                            .arg(_v6server.serverPort())
                            .arg(Quassel::buildInfo().protocolVersion)
                            );
                        success = true;
                    }
                    else
                        quWarning() << qPrintable(
                            tr("Could not open IPv6 interface %1:%2: %3")
                            .arg(addr.toString())
                            .arg(port)
                            .arg(_v6server.errorString()));
                    break;
                case QAbstractSocket::IPv4Protocol:
                    if (_server.listen(addr, port)) {
                        quInfo() << qPrintable(
                            tr("Listening for GUI clients on IPv4 %1 port %2 using protocol version %3")
                            .arg(addr.toString())
                            .arg(_server.serverPort())
                            .arg(Quassel::buildInfo().protocolVersion)
                            );
                        success = true;
                    }
                    else {
                        // if v6 succeeded on Any, the port will be already in use - don't display the error then
                        if (!success || _server.serverError() != QAbstractSocket::AddressInUseError)
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
    if (!success)
        quError() << qPrintable(tr("Could not open any network interfaces to listen on!"));

    return success;
}


void Core::stopListening(const QString &reason)
{
    bool wasListening = false;
    if (_server.isListening()) {
        wasListening = true;
        _server.close();
    }
    if (_v6server.isListening()) {
        wasListening = true;
        _v6server.close();
    }
    if (wasListening) {
        if (reason.isEmpty())
            quInfo() << "No longer listening for GUI clients.";
        else
            quInfo() << qPrintable(reason);
    }
}


void Core::incomingConnection()
{
    QTcpServer *server = qobject_cast<QTcpServer *>(sender());
    Q_ASSERT(server);
    while (server->hasPendingConnections()) {
        QTcpSocket *socket = server->nextPendingConnection();

        CoreAuthHandler *handler = new CoreAuthHandler(socket, this);
        _connectingClients.insert(handler);

        connect(handler, SIGNAL(disconnected()), SLOT(clientDisconnected()));
        connect(handler, SIGNAL(socketError(QAbstractSocket::SocketError,QString)), SLOT(socketError(QAbstractSocket::SocketError,QString)));
        connect(handler, SIGNAL(handshakeComplete(RemotePeer*,UserId)), SLOT(setupClientSession(RemotePeer*,UserId)));

        quInfo() << qPrintable(tr("Client connected from"))  << qPrintable(socket->peerAddress().toString());

        if (!_configured) {
            stopListening(tr("Closing server for basic setup."));
        }
    }
}


// Potentially called during the initialization phase (before handing the connection off to the session)
void Core::clientDisconnected()
{
    CoreAuthHandler *handler = qobject_cast<CoreAuthHandler *>(sender());
    Q_ASSERT(handler);

    quInfo() << qPrintable(tr("Non-authed client disconnected:")) << qPrintable(handler->socket()->peerAddress().toString());
    _connectingClients.remove(handler);
    handler->deleteLater();

    // make server listen again if still not configured
    if (!_configured) {
        startListening();
    }

    // TODO remove unneeded sessions - if necessary/possible...
    // Suggestion: kill sessions if they are not connected to any network and client.
}


void Core::setupClientSession(RemotePeer *peer, UserId uid)
{
    CoreAuthHandler *handler = qobject_cast<CoreAuthHandler *>(sender());
    Q_ASSERT(handler);

    // From now on everything is handled by the client session
    disconnect(handler, 0, this, 0);
    _connectingClients.remove(handler);
    handler->deleteLater();

    // Find or create session for validated user
    sessionForUser(uid);

    // as we are currently handling an event triggered by incoming data on this socket
    // it is unsafe to directly move the socket to the client thread.
    QCoreApplication::postEvent(this, new AddClientEvent(peer, uid));
}


void Core::customEvent(QEvent *event)
{
    if (event->type() == AddClientEventId) {
        AddClientEvent *addClientEvent = static_cast<AddClientEvent *>(event);
        addClientHelper(addClientEvent->peer, addClientEvent->userId);
        return;
    }
}


void Core::addClientHelper(RemotePeer *peer, UserId uid)
{
    // Find or create session for validated user
    SessionThread *session = sessionForUser(uid);
    session->addClient(peer);
}


void Core::setupInternalClientSession(InternalPeer *clientPeer)
{
    if (!_configured) {
        stopListening();
        setupCoreForInternalUsage();
    }

    UserId uid;
    if (_storage) {
        uid = _storage->internalUser();
    }
    else {
        quWarning() << "Core::setupInternalClientSession(): You're trying to run monolithic Quassel with an unusable Backend! Go fix it!";
        return;
    }

    InternalPeer *corePeer = new InternalPeer(this);
    corePeer->setPeer(clientPeer);
    clientPeer->setPeer(corePeer);

    // Find or create session for validated user
    SessionThread *sessionThread = sessionForUser(uid);
    sessionThread->addClient(corePeer);
}


SessionThread *Core::sessionForUser(UserId uid, bool restore)
{
    if (_sessions.contains(uid))
        return _sessions[uid];

    SessionThread *session = new SessionThread(uid, restore, this);
    _sessions[uid] = session;
    session->start();
    return session;
}


void Core::socketError(QAbstractSocket::SocketError err, const QString &errorString)
{
    quWarning() << QString("Socket error %1: %2").arg(err).arg(errorString);
}


QVariantList Core::backendInfo()
{
    instance()->registerStorageBackends();

    QVariantList backendInfos;
    for (auto &&backend : instance()->_registeredStorageBackends) {
        QVariantMap v;
        v["BackendId"]   = backend->backendId();
        v["DisplayName"] = backend->displayName();
        v["Description"] = backend->description();
        v["SetupData"]   = backend->setupData(); // ignored by legacy clients

        // TODO Protocol Break: Remove legacy (cf. authenticatorInfo())
        const auto &setupData = backend->setupData();
        QStringList setupKeys;
        QVariantMap setupDefaults;
        for (int i = 0; i + 2 < setupData.size(); i += 3) {
            setupKeys << setupData[i].toString();
            setupDefaults[setupData[i].toString()] = setupData[i + 2];
        }
        v["SetupKeys"]     = setupKeys;
        v["SetupDefaults"] = setupDefaults;
        // TODO Protocol Break: Remove
        v["IsDefault"]     = (backend->backendId() == "SQLite"); // newer clients will just use the first in the list

        backendInfos << v;
    }
    return backendInfos;
}


QVariantList Core::authenticatorInfo()
{
    instance()->registerAuthenticators();

    QVariantList authInfos;
    for(auto &&backend : instance()->_registeredAuthenticators) {
        QVariantMap v;
        v["BackendId"]   = backend->backendId();
        v["DisplayName"] = backend->displayName();
        v["Description"] = backend->description();
        v["SetupData"]   = backend->setupData();
        authInfos << v;
    }
    return authInfos;
}

// migration / backend selection
bool Core::selectBackend(const QString &backend)
{
    // reregister all storage backends
    registerStorageBackends();
    auto storage = storageBackend(backend);
    if (!storage) {
        QStringList backends;
        std::transform(_registeredStorageBackends.begin(), _registeredStorageBackends.end(),
                       std::back_inserter(backends), [](const DeferredSharedPtr<Storage>& backend) {
                           return backend->displayName();
                       });
        quWarning() << qPrintable(tr("Unsupported storage backend: %1").arg(backend));
        quWarning() << qPrintable(tr("Supported backends are:")) << qPrintable(backends.join(", "));
        return false;
    }

    QVariantMap settings = promptForSettings(storage.get());

    Storage::State storageState = storage->init(settings);
    switch (storageState) {
    case Storage::IsReady:
        if (!saveBackendSettings(backend, settings)) {
            qCritical() << qPrintable(QString("Could not save backend settings, probably a permission problem."));
        }
        quWarning() << qPrintable(tr("Switched storage backend to: %1").arg(backend));
        quWarning() << qPrintable(tr("Backend already initialized. Skipping Migration..."));
        return true;
    case Storage::NotAvailable:
        qCritical() << qPrintable(tr("Storage backend is not available: %1").arg(backend));
        return false;
    case Storage::NeedsSetup:
        if (!storage->setup(settings)) {
            quWarning() << qPrintable(tr("Unable to setup storage backend: %1").arg(backend));
            return false;
        }

        if (storage->init(settings) != Storage::IsReady) {
            quWarning() << qPrintable(tr("Unable to initialize storage backend: %1").arg(backend));
            return false;
        }

        if (!saveBackendSettings(backend, settings)) {
            qCritical() << qPrintable(QString("Could not save backend settings, probably a permission problem."));
        }
        quWarning() << qPrintable(tr("Switched storage backend to: %1").arg(backend));
        break;
    }

    // let's see if we have a current storage object we can migrate from
    auto reader = getMigrationReader(_storage.get());
    auto writer = getMigrationWriter(storage.get());
    if (reader && writer) {
        qDebug() << qPrintable(tr("Migrating storage backend %1 to %2...").arg(_storage->displayName(), storage->displayName()));
        _storage.reset();
        storage.reset();
        if (reader->migrateTo(writer.get())) {
            qDebug() << "Migration finished!";
            qDebug() << qPrintable(tr("Migration finished!"));
            if (!saveBackendSettings(backend, settings)) {
                qCritical() << qPrintable(QString("Could not save backend settings, probably a permission problem."));
                return false;
            }
            return true;
        }
        quWarning() << qPrintable(tr("Unable to migrate storage backend! (No migration writer for %1)").arg(backend));
        return false;
    }

    // inform the user why we cannot merge
    if (!_storage) {
        quWarning() << qPrintable(tr("No currently active storage backend. Skipping migration..."));
    }
    else if (!reader) {
        quWarning() << qPrintable(tr("Currently active storage backend does not support migration: %1").arg(_storage->displayName()));
    }
    if (writer) {
        quWarning() << qPrintable(tr("New storage backend does not support migration: %1").arg(backend));
    }

    // so we were unable to merge, but let's create a user \o/
    _storage = std::move(storage);
    createUser();
    return true;
}

// TODO: I am not sure if this function is implemented correctly.
// There is currently no concept of migraiton between auth backends.
bool Core::selectAuthenticator(const QString &backend)
{
    // Register all authentication backends.
    registerAuthenticators();
    auto auther = authenticator(backend);
    if (!auther) {
        QStringList authenticators;
        std::transform(_registeredAuthenticators.begin(), _registeredAuthenticators.end(),
                       std::back_inserter(authenticators), [](const DeferredSharedPtr<Authenticator>& authenticator) {
                           return authenticator->displayName();
                       });
        quWarning() << qPrintable(tr("Unsupported authenticator: %1").arg(backend));
        quWarning() << qPrintable(tr("Supported authenticators are:")) << qPrintable(authenticators.join(", "));
        return false;
    }

    QVariantMap settings = promptForSettings(auther.get());

    Authenticator::State state = auther->init(settings);
    switch (state) {
    case Authenticator::IsReady:
        saveAuthenticatorSettings(backend, settings);
        quWarning() << qPrintable(tr("Switched authenticator to: %1").arg(backend));
        return true;
    case Authenticator::NotAvailable:
        qCritical() << qPrintable(tr("Authenticator is not available: %1").arg(backend));
        return false;
    case Authenticator::NeedsSetup:
        if (!auther->setup(settings)) {
            quWarning() << qPrintable(tr("Unable to setup authenticator: %1").arg(backend));
            return false;
        }

        if (auther->init(settings) != Authenticator::IsReady) {
            quWarning() << qPrintable(tr("Unable to initialize authenticator: %1").arg(backend));
            return false;
        }

        saveAuthenticatorSettings(backend, settings);
        quWarning() << qPrintable(tr("Switched authenticator to: %1").arg(backend));
    }

    _authenticator = std::move(auther);
    return true;
}


bool Core::createUser()
{
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

    if (password != password2) {
        quWarning() << "Passwords don't match!";
        return false;
    }
    if (password.isEmpty()) {
        quWarning() << "Password is empty!";
        return false;
    }

    if (_configured && _storage->addUser(username, password).isValid()) {
        out << "Added user " << username << " successfully!" << endl;
        return true;
    }
    else {
        quWarning() << "Unable to add user:" << qPrintable(username);
        return false;
    }
}


bool Core::changeUserPass(const QString &username)
{
    QTextStream out(stdout);
    QTextStream in(stdin);
    UserId userId = _storage->getUserId(username);
    if (!userId.isValid()) {
        out << "User " << username << " does not exist." << endl;
        return false;
    }

    if (!canChangeUserPassword(userId)) {
        out << "User " << username << " is configured through an auth provider that has forbidden manual password changing." << endl;
        return false;
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

    if (password != password2) {
        quWarning() << "Passwords don't match!";
        return false;
    }
    if (password.isEmpty()) {
        quWarning() << "Password is empty!";
        return false;
    }

    if (_configured && _storage->updateUser(userId, password)) {
        out << "Password changed successfully!" << endl;
        return true;
    }
    else {
        quWarning() << "Failed to change password!";
        return false;
    }
}


bool Core::changeUserPassword(UserId userId, const QString &password)
{
    if (!isConfigured() || !userId.isValid())
        return false;

    if (!canChangeUserPassword(userId))
        return false;

    return instance()->_storage->updateUser(userId, password);
}

// TODO: this code isn't currently 100% optimal because the core
// doesn't know it can have multiple auth providers configured (there aren't
// multiple auth providers at the moment anyway) and we have hardcoded the
// Database provider to be always allowed.
bool Core::canChangeUserPassword(UserId userId)
{
    QString authProvider = instance()->_storage->getUserAuthenticator(userId);
    if (authProvider != "Database") {
        if (authProvider != instance()->_authenticator->backendId()) {
            return false;
        }
        else if (instance()->_authenticator->canChangePassword()) {
            return false;
        }
    }
    return true;
}


std::unique_ptr<AbstractSqlMigrationReader> Core::getMigrationReader(Storage *storage)
{
    if (!storage)
        return nullptr;

    AbstractSqlStorage *sqlStorage = qobject_cast<AbstractSqlStorage *>(storage);
    if (!sqlStorage) {
        qDebug() << "Core::migrateDb(): only SQL based backends can be migrated!";
        return nullptr;
    }

    return sqlStorage->createMigrationReader();
}


std::unique_ptr<AbstractSqlMigrationWriter> Core::getMigrationWriter(Storage *storage)
{
    if (!storage)
        return nullptr;

    AbstractSqlStorage *sqlStorage = qobject_cast<AbstractSqlStorage *>(storage);
    if (!sqlStorage) {
        qDebug() << "Core::migrateDb(): only SQL based backends can be migrated!";
        return nullptr;
    }

    return sqlStorage->createMigrationWriter();
}


bool Core::saveBackendSettings(const QString &backend, const QVariantMap &settings)
{
    QVariantMap dbsettings;
    dbsettings["Backend"] = backend;
    dbsettings["ConnectionProperties"] = settings;
    CoreSettings s = CoreSettings();
    s.setStorageSettings(dbsettings);
    return s.sync();
}


void Core::saveAuthenticatorSettings(const QString &backend, const QVariantMap &settings)
{
    QVariantMap dbsettings;
    dbsettings["Authenticator"] = backend;
    dbsettings["AuthProperties"] = settings;
    CoreSettings().setAuthSettings(dbsettings);
}

// Generic version of promptForSettings that doesn't care what *type* of
// backend it runs over.
template<typename Backend>
QVariantMap Core::promptForSettings(const Backend *backend)
{
    QVariantMap settings;
    const QVariantList& setupData = backend->setupData();

    if (setupData.isEmpty())
        return settings;

    QTextStream out(stdout);
    QTextStream in(stdin);
    out << "Default values are in brackets" << endl;

    for (int i = 0; i + 2 < setupData.size(); i += 3) {
        QString key = setupData[i].toString();
        out << setupData[i+1].toString() << " [" << setupData[i+2].toString() << "]: " << flush;

        bool noEcho = key.toLower().contains("password");
        if (noEcho) {
            disableStdInEcho();
        }
        QString input = in.readLine().trimmed();
        if (noEcho) {
            out << endl;
            enableStdInEcho();
        }

        QVariant value{setupData[i+2]};
        if (!input.isEmpty()) {
            switch (value.type()) {
            case QVariant::Int:
                value = input.toInt();
                break;
            default:
                value = input;
            }
        }
        settings[key] = value;
    }
    return settings;
}


#ifdef Q_OS_WIN
void Core::stdInEcho(bool on)
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    if (on)
        mode |= ENABLE_ECHO_INPUT;
    else
        mode &= ~ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
}

#else
void Core::stdInEcho(bool on)
{
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    if (on)
        t.c_lflag |= ECHO;
    else
        t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

#endif /* Q_OS_WIN */

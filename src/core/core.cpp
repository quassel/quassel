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
#include "internalpeer.h"
#include "logmessage.h"
#include "network.h"
#include "postgresqlstorage.h"
#include "quassel.h"
#include "sqlauthenticator.h"
#include "sqlitestorage.h"
#include "types.h"
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

Core::Core()
    : Singleton<Core>{this}
{
    // Parent all QObject-derived attributes, so when the Core instance gets moved into another
    // thread, they get moved with it
    _server.setParent(this);
    _v6server.setParent(this);
    _storageSyncTimer.setParent(this);
}


Core::~Core()
{
    qDeleteAll(_connectingClients);
    qDeleteAll(_sessions);
    syncStorage();
}


void Core::init()
{
    _startTime = QDateTime::currentDateTime().toUTC(); // for uptime :)

    if (Quassel::runMode() == Quassel::RunMode::CoreOnly) {
        Quassel::loadTranslation(QLocale::system());
    }

    // check settings version
    // so far, we only have 1
    CoreSettings s;
    if (s.version() != 1) {
        throw ExitException{EXIT_FAILURE, tr("Invalid core settings version!")};
    }

    // Set up storage and authentication backends
    registerStorageBackends();
    registerAuthenticators();

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    bool config_from_environment = Quassel::isOptionSet("config-from-environment");

    QString db_backend;
    QVariantMap db_connectionProperties;

    QString auth_authenticator;
    QVariantMap auth_properties;

    bool writeError = false;

    if (config_from_environment) {
        db_backend = environment.value("DB_BACKEND");
        auth_authenticator = environment.value("AUTH_AUTHENTICATOR");
    }
    else {
        CoreSettings cs;

        QVariantMap dbsettings = cs.storageSettings().toMap();
        db_backend = dbsettings.value("Backend").toString();
        db_connectionProperties = dbsettings.value("ConnectionProperties").toMap();

        QVariantMap authSettings = cs.authSettings().toMap();
        auth_authenticator = authSettings.value("Authenticator", "Database").toString();
        auth_properties = authSettings.value("AuthProperties").toMap();

        writeError = !cs.isWritable();
    }

    try {
        _configured = initStorage(db_backend, db_connectionProperties, environment, config_from_environment);
        if (_configured) {
            _configured = initAuthenticator(auth_authenticator, auth_properties, environment, config_from_environment);
        }
    }
    catch (ExitException) {
        // Try again later
        _configured = false;
    }

    if (Quassel::isOptionSet("select-backend") || Quassel::isOptionSet("select-authenticator")) {
        bool success{true};
        if (Quassel::isOptionSet("select-backend")) {
            success &= selectBackend(Quassel::optionValue("select-backend"));
        }
        if (Quassel::isOptionSet("select-authenticator")) {
            success &= selectAuthenticator(Quassel::optionValue("select-authenticator"));
        }
        throw ExitException{success ? EXIT_SUCCESS : EXIT_FAILURE};
    }

    if (!_configured) {
        if (config_from_environment) {
            try {
                _configured = initStorage(db_backend, db_connectionProperties, environment, config_from_environment, true);
                if (_configured) {
                    _configured = initAuthenticator(auth_authenticator, auth_properties, environment, config_from_environment, true);
                }
            }
            catch (ExitException e) {
                throw ExitException{EXIT_FAILURE, tr("Cannot configure from environment: %1").arg(e.errorString)};
            }

            if (!_configured) {
                throw ExitException{EXIT_FAILURE, tr("Cannot configure from environment!")};
            }
        }
        else {
            if (_registeredStorageBackends.empty()) {
                throw ExitException{EXIT_FAILURE,
                                    tr("Could not initialize any storage backend! Exiting...\n"
                                       "Currently, Quassel supports SQLite3 and PostgreSQL. You need to build your\n"
                                       "Qt library with the sqlite or postgres plugin enabled in order for quasselcore\n"
                                       "to work.")};
            }

            if (writeError) {
                throw ExitException{EXIT_FAILURE, tr("Cannot write quasselcore configuration; probably a permission problem.")};
            }

            quInfo() << "Core is currently not configured! Please connect with a Quassel Client for basic setup.";
        }
    }
    else {
        if (Quassel::isOptionSet("add-user")) {
            bool success = createUser();
            throw ExitException{success ? EXIT_SUCCESS : EXIT_FAILURE};
        }

        if (Quassel::isOptionSet("change-userpass")) {
            bool success = changeUserPass(Quassel::optionValue("change-userpass"));
            throw ExitException{success ? EXIT_SUCCESS : EXIT_FAILURE};
        }

        _strictIdentEnabled = Quassel::isOptionSet("strict-ident");
        if (_strictIdentEnabled) {
            cacheSysIdent();
        }

        if (Quassel::isOptionSet("oidentd")) {
            _oidentdConfigGenerator = new OidentdConfigGenerator(this);
        }


        if (Quassel::isOptionSet("ident-daemon")) {
            _identServer = new IdentServer(this);
        }

        Quassel::registerReloadHandler([]() {
            // Currently, only reloading SSL certificates and the sysident cache is supported
            if (Core::instance()) {
                Core::instance()->cacheSysIdent();
                Core::instance()->reloadCerts();
                return true;
            }
            return false;
        });

        connect(&_storageSyncTimer, SIGNAL(timeout()), this, SLOT(syncStorage()));
        _storageSyncTimer.start(10 * 60 * 1000); // 10 minutes
    }

    connect(&_server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
    connect(&_v6server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));

    if (!startListening()) {
        throw ExitException{EXIT_FAILURE, tr("Cannot open port for listening!")};
    }

    if (_configured && !Quassel::isOptionSet("norestore")) {
        Core::restoreState();
    }

    _initialized = true;

    if (_pendingInternalConnection) {
        connectInternalPeer(_pendingInternalConnection);
        _pendingInternalConnection = {};
    }
}


void Core::initAsync()
{
    try {
        init();
    }
    catch (ExitException e) {
        emit exitRequested(e.exitCode, e.errorString);
    }
}


void Core::shutdown()
{
    quInfo() << "Core shutting down...";

    saveState();

    for (auto &&client : _connectingClients) {
        client->deleteLater();
    }
    _connectingClients.clear();

    if (_sessions.isEmpty()) {
        emit shutdownComplete();
        return;
    }

    for (auto &&session : _sessions) {
        connect(session, SIGNAL(shutdownComplete(SessionThread*)), this, SLOT(onSessionShutdown(SessionThread*)));
        session->shutdown();
    }
}


void Core::onSessionShutdown(SessionThread *session)
{
    _sessions.take(_sessions.key(session))->deleteLater();
    if (_sessions.isEmpty()) {
        quInfo() << "Core shutdown complete!";
        emit shutdownComplete();
    }
}


/*** Session Restore ***/

void Core::saveState()
{
    if (_storage) {
        QVariantList activeSessions;
        for (auto &&user : instance()->_sessions.keys())
            activeSessions << QVariant::fromValue<UserId>(user);
        _storage->setCoreState(activeSessions);
    }
}


void Core::restoreState()
{
    if (!_configured) {
        quWarning() << qPrintable(tr("Cannot restore a state for an unconfigured core!"));
        return;
    }
    if (_sessions.count()) {
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
        for(auto &&v : activeSessions) {
            UserId user = v.value<UserId>();
            sessionForUser(user, true);
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
    try {
        if (!(_configured = initStorage(backend, setupData, {}, false, true))) {
            return tr("Could not setup storage!");
        }

        quInfo() << "Selected authenticator:" << authenticator;
        if (!(_configured = initAuthenticator(authenticator, authSetupData, {}, false, true)))
        {
            return tr("Could not setup authenticator!");
        }
    }
    catch (ExitException e) {
        // Event loop is running, so trigger an exit rather than throwing an exception
        QCoreApplication::exit(e.exitCode);
        return e.errorString.isEmpty() ? tr("Fatal failure while trying to setup, terminating") : e.errorString;
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

    qsrand(QDateTime::currentDateTime().toMSecsSinceEpoch());
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


bool Core::initStorage(const QString &backend, const QVariantMap &settings,
                       const QProcessEnvironment &environment, bool loadFromEnvironment, bool setup)
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

    connect(storage.get(), SIGNAL(dbUpgradeInProgress(bool)), this, SIGNAL(dbUpgradeInProgress(bool)));

    Storage::State storageState = storage->init(settings, environment, loadFromEnvironment);
    switch (storageState) {
    case Storage::NeedsSetup:
        if (!setup)
            return false;  // trigger setup process
        if (storage->setup(settings, environment, loadFromEnvironment))
            return initStorage(backend, settings, environment, loadFromEnvironment, false);
        return false;

    case Storage::NotAvailable:
        if (!setup) {
            // If initialization wasn't successful, we quit to keep from coming up unconfigured
            throw ExitException{EXIT_FAILURE, tr("Selected storage backend %1 is not available.").arg(backend)};
        }
        qCritical() << "Selected storage backend is not available:" << backend;
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
bool Core::initAuthenticator(const QString &backend, const QVariantMap &settings,
                             const QProcessEnvironment &environment, bool loadFromEnvironment,
                             bool setup)
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

    Authenticator::State authState = auth->init(settings, environment, loadFromEnvironment);
    switch (authState) {
    case Authenticator::NeedsSetup:
        if (!setup)
            return false;  // trigger setup process
        if (auth->setup(settings, environment, loadFromEnvironment))
            return initAuthenticator(backend, settings, environment, loadFromEnvironment, false);
        return false;

    case Authenticator::NotAvailable:
        if (!setup) {
            // If initialization wasn't successful, we quit to keep from coming up unconfigured
            throw ExitException{EXIT_FAILURE, tr("Selected auth backend %1 is not available.").arg(backend)};
        }
        qCritical() << "Selected auth backend is not available:" << backend;
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
    SslServer *sslServerv4 = qobject_cast<SslServer *>(&_server);
    bool retv4 = sslServerv4->reloadCerts();

    SslServer *sslServerv6 = qobject_cast<SslServer *>(&_v6server);
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
        _authUserNames = _storage->getAllAuthUserNames();
    }
}


QString Core::strictSysIdent(UserId user) const
{
    if (_authUserNames.contains(user)) {
        return _authUserNames[user];
    }

    // A new user got added since we last pulled our cache from the database.
    // There's no way to avoid a database hit - we don't even know the authname!
    instance()->cacheSysIdent();

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

    if (_identServer) {
        _identServer->startListening();
    }

    return success;
}


void Core::stopListening(const QString &reason)
{
    if (_identServer) {
        _identServer->stopListening(reason);
    }

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


void Core::connectInternalPeer(QPointer<InternalPeer> peer)
{
    if (_initialized && peer) {
        setupInternalClientSession(peer);
    }
    else {
        _pendingInternalConnection = peer;
    }
}


void Core::setupInternalClientSession(QPointer<InternalPeer> clientPeer)
{
    if (!_configured) {
        stopListening();
        auto errorString = setupCoreForInternalUsage();
        if (!errorString.isEmpty()) {
            emit exitRequested(EXIT_FAILURE, errorString);
            return;
        }
    }

    UserId uid;
    if (_storage) {
        uid = _storage->internalUser();
    }
    else {
        quWarning() << "Core::setupInternalClientSession(): You're trying to run monolithic Quassel with an unusable Backend! Go fix it!";
        emit exitRequested(EXIT_FAILURE, tr("Cannot setup storage backend."));
        return;
    }

    if (!clientPeer) {
        quWarning() << "Client peer went away, not starting a session";
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

    return (_sessions[uid] = new SessionThread(uid, restore, strictIdentEnabled(), this));
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

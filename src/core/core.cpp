/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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
#include "sqlitestorage.h"
#include "util.h"

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
    : QObject(),
      _storage(0)
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
            qWarning() << "\n\n*** IMPORTANT: Config and data file locations have changed. Attempting to auto-migrate your core settings...";
            foreach(QString key, oldSettings.allKeys())
            newSettings.setValue(key, oldSettings.value(key));
            newSettings.setValue("Config/Version", 1);
            qWarning() << "*   Your core settings have been migrated to" << newSettings.fileName();

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
                        qWarning() << "*   Your database has been moved to" << Quassel::configDirPath() + "quassel-storage.sqlite";
                    else
                        qWarning() << "!!! Moving your database has failed. Please move it manually into" << Quassel::configDirPath();
                }
            }
            // move certificate
            QFileInfo certInfo(quasselDir + "quasselCert.pem");
            if (certInfo.exists()) {
                QFile cert(quasselDir + "quasselCert.pem");
                bool success = cert.rename(Quassel::configDirPath() + "quasselCert.pem");
                if (success)
                    qWarning() << "*   Your certificate has been moved to" << Quassel::configDirPath() + "quasselCert.pem";
                else
                    qWarning() << "!!! Moving your certificate has failed. Please move it manually into" << Quassel::configDirPath();
            }
#endif /* !Q_OS_MAC */
            qWarning() << "*** Migration completed.\n\n";
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

    registerStorageBackends();

    connect(&_storageSyncTimer, SIGNAL(timeout()), this, SLOT(syncStorage()));
    _storageSyncTimer.start(10 * 60 * 1000); // 10 minutes
}


void Core::init()
{
    CoreSettings cs;
    // legacy
    QVariantMap dbsettings = cs.storageSettings().toMap();
    _configured = initStorage(dbsettings.value("Backend").toString(), dbsettings.value("ConnectionProperties").toMap());

    if (Quassel::isOptionSet("select-backend")) {
        selectBackend(Quassel::optionValue("select-backend"));
        exit(0);
    }

    if (!_configured) {
        if (!_storageBackends.count()) {
            qWarning() << qPrintable(tr("Could not initialize any storage backend! Exiting..."));
            qWarning() << qPrintable(tr("Currently, Quassel supports SQLite3 and PostgreSQL. You need to build your\n"
                                        "Qt library with the sqlite or postgres plugin enabled in order for quasselcore\n"
                                        "to work."));
            exit(1); // TODO make this less brutal (especially for mono client -> popup)
        }
        qWarning() << "Core is currently not configured! Please connect with a Quassel Client for basic setup.";
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

    if (Quassel::isOptionSet("oidentd"))
        _oidentdConfigGenerator = new OidentdConfigGenerator(this);
}


Core::~Core()
{
    // FIXME do we need more cleanup for handlers?
    foreach(CoreAuthHandler *handler, _connectingClients) {
        handler->deleteLater(); // disconnect non authed clients
    }
    qDeleteAll(_sessions);
    qDeleteAll(_storageBackends);
}


/*** Session Restore ***/

void Core::saveState()
{
    CoreSettings s;
    QVariantMap state;
    QVariantList activeSessions;
    foreach(UserId user, instance()->_sessions.keys())
        activeSessions << QVariant::fromValue<UserId>(user);
    state["CoreStateVersion"] = 1;
    state["ActiveSessions"] = activeSessions;
    s.setCoreState(state);
}


void Core::restoreState()
{
    if (!instance()->_configured) {
        // qWarning() << qPrintable(tr("Cannot restore a state for an unconfigured core!"));
        return;
    }
    if (instance()->_sessions.count()) {
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
    if (activeSessions.count() > 0) {
        quInfo() << "Restoring previous core state...";
        foreach(QVariant v, activeSessions) {
            UserId user = v.value<UserId>();
            instance()->sessionForUser(user, true);
        }
    }
}


/*** Core Setup ***/

QString Core::setup(const QString &adminUser, const QString &adminPassword, const QString &backend, const QVariantMap &setupData)
{
    return instance()->setupCore(adminUser, adminPassword, backend, setupData);
}


QString Core::setupCore(const QString &adminUser, const QString &adminPassword, const QString &backend, const QVariantMap &setupData)
{
    if (_configured)
        return tr("Core is already configured! Not configuring again...");

    if (adminUser.isEmpty() || adminPassword.isEmpty()) {
        return tr("Admin user or password not set.");
    }
    if (!(_configured = initStorage(backend, setupData, true))) {
        return tr("Could not setup storage!");
    }

    saveBackendSettings(backend, setupData);

    quInfo() << qPrintable(tr("Creating admin user..."));
    _storage->addUser(adminUser, adminPassword);
    startListening(); // TODO check when we need this
    return QString();
}


QString Core::setupCoreForInternalUsage()
{
    Q_ASSERT(!_storageBackends.isEmpty());

    qsrand(QDateTime::currentDateTime().toTime_t());
    int pass = 0;
    for (int i = 0; i < 10; i++) {
        pass *= 10;
        pass += qrand() % 10;
    }

    // mono client currently needs sqlite
    return setupCore("AdminUser", QString::number(pass), "SQLite", QVariantMap());
}


/*** Storage Handling ***/
void Core::registerStorageBackends()
{
    // Register storage backends here!
    registerStorageBackend(new SqliteStorage(this));
    registerStorageBackend(new PostgreSqlStorage(this));
}


bool Core::registerStorageBackend(Storage *backend)
{
    if (backend->isAvailable()) {
        _storageBackends[backend->displayName()] = backend;
        return true;
    }
    else {
        backend->deleteLater();
        return false;
    }
}


void Core::unregisterStorageBackends()
{
    foreach(Storage *s, _storageBackends.values()) {
        s->deleteLater();
    }
    _storageBackends.clear();
}


void Core::unregisterStorageBackend(Storage *backend)
{
    _storageBackends.remove(backend->displayName());
    backend->deleteLater();
}


// old db settings:
// "Type" => "sqlite"
bool Core::initStorage(const QString &backend, const QVariantMap &settings, bool setup)
{
    _storage = 0;

    if (backend.isEmpty()) {
        return false;
    }

    Storage *storage = 0;
    if (_storageBackends.contains(backend)) {
        storage = _storageBackends[backend];
    }
    else {
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
    // if initialization wasn't successful, we quit to keep from coming up unconfigured
    case Storage::NotAvailable:
        qCritical() << "FATAL: Selected storage backend is not available:" << backend;
        exit(EXIT_FAILURE);
    case Storage::IsReady:
        // delete all other backends
        _storageBackends.remove(backend);
        unregisterStorageBackends();
        connect(storage, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)), this, SIGNAL(bufferInfoUpdated(UserId, const BufferInfo &)));
    }
    _storage = storage;
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
        qWarning() << "Core::setupInternalClientSession(): You're trying to run monolithic Quassel with an unusable Backend! Go fix it!";
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
    qWarning() << QString("Socket error %1: %2").arg(err).arg(errorString);
}


QVariantList Core::backendInfo()
{
    QVariantList backends;
    foreach(const Storage *backend, instance()->_storageBackends.values()) {
        QVariantMap v;
        v["DisplayName"] = backend->displayName();
        v["Description"] = backend->description();
        v["SetupKeys"] = backend->setupKeys();
        v["SetupDefaults"] = backend->setupDefaults();
        backends.append(v);
    }
    return backends;
}


// migration / backend selection
bool Core::selectBackend(const QString &backend)
{
    // reregister all storage backends
    registerStorageBackends();
    if (!_storageBackends.contains(backend)) {
        qWarning() << qPrintable(QString("Core::selectBackend(): unsupported backend: %1").arg(backend));
        qWarning() << "    supported backends are:" << qPrintable(QStringList(_storageBackends.keys()).join(", "));
        return false;
    }

    Storage *storage = _storageBackends[backend];
    QVariantMap settings = promptForSettings(storage);

    Storage::State storageState = storage->init(settings);
    switch (storageState) {
    case Storage::IsReady:
        saveBackendSettings(backend, settings);
        qWarning() << "Switched backend to:" << qPrintable(backend);
        qWarning() << "Backend already initialized. Skipping Migration";
        return true;
    case Storage::NotAvailable:
        qCritical() << "Backend is not available:" << qPrintable(backend);
        return false;
    case Storage::NeedsSetup:
        if (!storage->setup(settings)) {
            qWarning() << qPrintable(QString("Core::selectBackend(): unable to setup backend: %1").arg(backend));
            return false;
        }

        if (storage->init(settings) != Storage::IsReady) {
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
    if (reader && writer) {
        qDebug() << qPrintable(QString("Migrating Storage backend %1 to %2...").arg(_storage->displayName(), storage->displayName()));
        delete _storage;
        _storage = 0;
        delete storage;
        storage = 0;
        if (reader->migrateTo(writer)) {
            qDebug() << "Migration finished!";
            saveBackendSettings(backend, settings);
            return true;
        }
        return false;
        qWarning() << qPrintable(QString("Core::migrateDb(): unable to migrate storage backend! (No migration writer for %1)").arg(backend));
    }

    // inform the user why we cannot merge
    if (!_storage) {
        qWarning() << "No currently active backend. Skipping migration.";
    }
    else if (!reader) {
        qWarning() << "Currently active backend does not support migration:" << qPrintable(_storage->displayName());
    }
    if (writer) {
        qWarning() << "New backend does not support migration:" << qPrintable(backend);
    }

    // so we were unable to merge, but let's create a user \o/
    _storage = storage;
    createUser();
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
        qWarning() << "Passwords don't match!";
        return false;
    }
    if (password.isEmpty()) {
        qWarning() << "Password is empty!";
        return false;
    }

    if (_configured && _storage->addUser(username, password).isValid()) {
        out << "Added user " << username << " successfully!" << endl;
        return true;
    }
    else {
        qWarning() << "Unable to add user:" << qPrintable(username);
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
        qWarning() << "Passwords don't match!";
        return false;
    }
    if (password.isEmpty()) {
        qWarning() << "Password is empty!";
        return false;
    }

    if (_configured && _storage->updateUser(userId, password)) {
        out << "Password changed successfully!" << endl;
        return true;
    }
    else {
        qWarning() << "Failed to change password!";
        return false;
    }
}


bool Core::changeUserPassword(UserId userId, const QString &password)
{
    if (!isConfigured() || !userId.isValid())
        return false;

    return instance()->_storage->updateUser(userId, password);
}


AbstractSqlMigrationReader *Core::getMigrationReader(Storage *storage)
{
    if (!storage)
        return 0;

    AbstractSqlStorage *sqlStorage = qobject_cast<AbstractSqlStorage *>(storage);
    if (!sqlStorage) {
        qDebug() << "Core::migrateDb(): only SQL based backends can be migrated!";
        return 0;
    }

    return sqlStorage->createMigrationReader();
}


AbstractSqlMigrationWriter *Core::getMigrationWriter(Storage *storage)
{
    if (!storage)
        return 0;

    AbstractSqlStorage *sqlStorage = qobject_cast<AbstractSqlStorage *>(storage);
    if (!sqlStorage) {
        qDebug() << "Core::migrateDb(): only SQL based backends can be migrated!";
        return 0;
    }

    return sqlStorage->createMigrationWriter();
}


void Core::saveBackendSettings(const QString &backend, const QVariantMap &settings)
{
    QVariantMap dbsettings;
    dbsettings["Backend"] = backend;
    dbsettings["ConnectionProperties"] = settings;
    CoreSettings().setStorageSettings(dbsettings);
}


QVariantMap Core::promptForSettings(const Storage *storage)
{
    QVariantMap settings;

    QStringList keys = storage->setupKeys();
    if (keys.isEmpty())
        return settings;

    QTextStream out(stdout);
    QTextStream in(stdin);
    out << "Default values are in brackets" << endl;

    QVariantMap defaults = storage->setupDefaults();
    QString value;
    foreach(QString key, keys) {
        QVariant val;
        if (defaults.contains(key)) {
            val = defaults[key];
        }
        out << key;
        if (!val.toString().isEmpty()) {
            out << " (" << val.toString() << ")";
        }
        out << ": ";
        out.flush();

        bool noEcho = QString("password").toLower().startsWith(key.toLower());
        if (noEcho) {
            disableStdInEcho();
        }
        value = in.readLine().trimmed();
        if (noEcho) {
            out << endl;
            enableStdInEcho();
        }

        if (!value.isEmpty()) {
            switch (defaults[key].type()) {
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

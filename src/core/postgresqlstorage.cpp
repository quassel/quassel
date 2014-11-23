/***************************************************************************
 *   Copyright (C) 2005-2014 by the Quassel Project                        *
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

#include "postgresqlstorage.h"

#include <QtSql>

#include "logger.h"
#include "network.h"
#include "quassel.h"

PostgreSqlStorage::PostgreSqlStorage(QObject *parent)
    : AbstractSqlStorage(parent),
    _port(-1)
{
}


PostgreSqlStorage::~PostgreSqlStorage()
{
}


AbstractSqlMigrationWriter *PostgreSqlStorage::createMigrationWriter()
{
    PostgreSqlMigrationWriter *writer = new PostgreSqlMigrationWriter();
    QVariantMap properties;
    properties["Username"] = _userName;
    properties["Password"] = _password;
    properties["Hostname"] = _hostName;
    properties["Port"] = _port;
    properties["Database"] = _databaseName;
    writer->setConnectionProperties(properties);
    return writer;
}


bool PostgreSqlStorage::isAvailable() const
{
    qDebug() << QSqlDatabase::drivers();
    if (!QSqlDatabase::isDriverAvailable("QPSQL")) return false;
    return true;
}


QString PostgreSqlStorage::displayName() const
{
    return QString("PostgreSQL");
}


QString PostgreSqlStorage::description() const
{
    // FIXME: proper description
    return tr("PostgreSQL Turbo Bomber HD!");
}


QStringList PostgreSqlStorage::setupKeys() const
{
    QStringList keys;
    keys << "Username"
         << "Password"
         << "Hostname"
         << "Port"
         << "Database";
    return keys;
}


QVariantMap PostgreSqlStorage::setupDefaults() const
{
    QVariantMap map;
    map["Username"] = QVariant(QString("quassel"));
    map["Hostname"] = QVariant(QString("localhost"));
    map["Port"] = QVariant(5432);
    map["Database"] = QVariant(QString("quassel"));
    return map;
}


bool PostgreSqlStorage::initDbSession(QSqlDatabase &db)
{
    // check whether the Qt driver performs string escaping or not.
    // i.e. test if it doubles slashes.
    QSqlField testField;
    testField.setType(QVariant::String);
    testField.setValue("\\");
    QString formattedString = db.driver()->formatValue(testField);
    switch(formattedString.count('\\')) {
    case 2:
        // yes it does... and we cannot do anything to change the behavior of Qt.
        // If this is a legacy DB (Postgres < 8.2), then everything is already ok,
        // as this is the expected behavior.
        // If it is a newer version, switch to legacy mode.

        quWarning() << "Switching Postgres to legacy mode. (set standard conforming strings to off)";
        // If the following calls fail, it is a legacy DB anyways, so it doesn't matter
        // and no need to check the outcome.
        db.exec("set standard_conforming_strings = off");
        db.exec("set escape_string_warning = off");
        break;
    case 1:
        // ok, so Qt does not escape...
        // That means we have to ensure that postgres uses standard conforming strings...
        {
            QSqlQuery query = db.exec("set standard_conforming_strings = on");
            if (query.lastError().isValid()) {
                // We cannot enable standard conforming strings...
                // since Quassel does no escaping by itself, this would yield a major vulnerability.
                quError() << "Failed to enable standard_conforming_strings for the Postgres db!";
                return false;
            }
        }
        break;
    default:
        // The slash got replaced with 0 or more than 2 slashes! o_O
        quError() << "Your version of Qt does something _VERY_ strange to slashes in QSqlQueries! You should consult your trusted doctor!";
        return false;
        break;
    }
    return true;
}


void PostgreSqlStorage::setConnectionProperties(const QVariantMap &properties)
{
    _userName = properties["Username"].toString();
    _password = properties["Password"].toString();
    _hostName = properties["Hostname"].toString();
    _port = properties["Port"].toInt();
    _databaseName = properties["Database"].toString();
}


int PostgreSqlStorage::installedSchemaVersion()
{
    // These queries don't use the executePreparedQuery() infrastructure to prevent log SPAM if the DB
    // schema isn't created yet.  They also can't use transactions because of the possibility that
    // the tables don't exist yet.
    QSqlQuery query = logDb().exec("SELECT value FROM coreinfo WHERE key = 'schemaversion'");
    if (query.first())
        return query.value(0).toInt();

    // maybe it's really old... (schema version 0)
    query = logDb().exec("SELECT MAX(version) FROM coreinfo");
    if (query.first())
        return query.value(0).toInt();

    return AbstractSqlStorage::installedSchemaVersion();
}


bool PostgreSqlStorage::updateSchemaVersion(int newVersion)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::updateSchemaVersion(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QSqlQuery query = executePreparedQuery("update_schema_version", newVersion, db);

    if (!watchQuery(query)) {
        qCritical() << "PostgreSqlStorage::updateSchemaVersion(): Updating schema version failed!";
        db.rollback();
        return false;
    }
    
    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::updateSchemaVersion(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    return true;
}


bool PostgreSqlStorage::setupSchemaVersion(int version)
{
    // This query is already inside the transaction created by setup() in AbstractSqlStorage
    QSqlDatabase db = logDb();

    QSqlQuery query = executePreparedQuery("insert_schema_version", version, db);

    if (!watchQuery(query)) {
        qCritical() << "PostgreSqlStorage::setupSchemaVersion(int): Updating schema version failed!";
        return false;
    }
    
    return true;
}


UserId PostgreSqlStorage::addUser(const QString &user, const QString &password)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::addUser(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return 0;
    }

    QVariantList params;
    params << user
           << cryptedPassword(password);
    QSqlQuery query = executePreparedQuery("insert_quasseluser", params, db);
    
    if (!watchQuery(query)) {
        db.rollback();
        return 0;
    }

    query.first();
    UserId uid = query.value(0).toInt();

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::addUser(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return 0;
    }

    emit userAdded(uid, user);
    return uid;
}


bool PostgreSqlStorage::updateUser(UserId user, const QString &password)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::updateUser(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QVariantList params;
    params << cryptedPassword(password)
           << user.toInt();
    QSqlQuery query = executePreparedQuery("update_userpassword", params, db);
    
    if (!watchQuery(query)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::updateUser(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    return query.numRowsAffected() != 0;
}


void PostgreSqlStorage::renameUser(UserId user, const QString &newName)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::renameUser(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << newName
           << user.toInt();
    QSqlQuery query = executePreparedQuery("update_username", params, db);
    
    if (!watchQuery(query)) {
        db.rollback();
        return;
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::renameUser(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    emit userRenamed(user, newName);
}


UserId PostgreSqlStorage::validateUser(const QString &user, const QString &password)
{
    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::validateUser(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return 0;
    }

    QVariantList params;
    params << user
           << cryptedPassword(password);
    QSqlQuery query = executePreparedQuery("select_authuser", params, db);

    watchQuery(query);

    if (query.first()) {
        db.commit();
        return query.value(0).toInt();
    }
    else {
        db.rollback();
        return 0;
    }
}


UserId PostgreSqlStorage::getUserId(const QString &user)
{
    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::getUserId(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return 0;
    }

    QSqlQuery query = executePreparedQuery("select_userid", user, db);

    watchQuery(query);

    if (query.first()) {
        db.commit();
        return query.value(0).toInt();
    }
    else {
        db.rollback();
        return 0;
    }
}


UserId PostgreSqlStorage::internalUser()
{
    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::internalUser(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return 0;
    }

    QSqlQuery query = executePreparedQuery("select_internaluser", QVariant(), db);

    watchQuery(query);

    if (query.first()) {
        db.commit();
        return query.value(0).toInt();
    }
    else {
        db.rollback();
        return 0;
    }
}


void PostgreSqlStorage::delUser(UserId user)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::delUser(): cannot start transaction!";
        return;
    }

    QSqlQuery query = executePreparedQuery("delete_quasseluser", user.toInt(), db);

    if (!watchQuery(query)) {
        db.rollback();
        return;
    }
    else {
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::delUser(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
            return;
        }
        emit userRemoved(user);
    }
}


void PostgreSqlStorage::setUserSetting(UserId userId, const QString &settingName, const QVariant &data)
{
    QByteArray rawData;
    QDataStream out(&rawData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_2);
    out << data;

    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setUserSetting(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList selectParams;
    selectParams << userId.toInt()
                 << settingName;
    QSqlQuery selectQuery = executePreparedQuery("select_user_setting", selectParams, db);

    watchQuery(selectQuery);

    QVariantList setParams;
    setParams << rawData
              << userId.toInt()
              << settingName;

    QSqlQuery insertUpdateQuery;
    if (!selectQuery.first()) {
        insertUpdateQuery = executePreparedQuery("insert_user_setting", setParams, db);
    }
    else {
        insertUpdateQuery = executePreparedQuery("update_user_setting", setParams, db);
    }

    watchQuery(insertUpdateQuery);
    
    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setUserSetting(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


QVariant PostgreSqlStorage::getUserSetting(UserId userId, const QString &settingName, const QVariant &defaultData)
{
    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::getUserSetting(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return defaultData;
    }

    QVariantList params;
    params << userId.toInt()
           << settingName;
    QSqlQuery query = executePreparedQuery("select_user_setting", params, db);

    watchQuery(query);

    if (query.first()) {
        db.commit();
        QVariant data;
        QByteArray rawData = query.value(0).toByteArray();
        QDataStream in(&rawData, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_4_2);
        in >> data;
        return data;
    }
    else {
        db.rollback();
        return defaultData;
    }
}


IdentityId PostgreSqlStorage::createIdentity(UserId user, CoreIdentity &identity)
{
    IdentityId identityId;

    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::createIdentity(): Unable to start Transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return identityId;
    }

    QVariantList params;
    params << user.toInt()
           << identity.identityName()
           << identity.realName()
           << identity.awayNick()
           << identity.awayNickEnabled()
           << identity.awayReason()
           << identity.awayReasonEnabled()
           << identity.awayReasonEnabled()
           << identity.autoAwayTime()
           << identity.autoAwayReason()
           << identity.autoAwayReasonEnabled()
           << identity.detachAwayEnabled()
           << identity.detachAwayReason()
           << identity.detachAwayReasonEnabled()
           << identity.ident()
           << identity.kickReason()
           << identity.partReason()
           << identity.quitReason()
#ifdef HAVE_SSL
           << identity.sslCert().toPem()
           << identity.sslKey().toPem();
#else
           << QByteArray()
           << QByteArray();
#endif
    QSqlQuery query = executePreparedQuery("insert_identity", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return IdentityId();
    }

    query.first();
    identityId = query.value(0).toInt();
    identity.setId(identityId);

    if (!identityId.isValid()) {
        db.rollback();
        return IdentityId();
    }

    foreach(QString nick, identity.nicks()) {
        QVariantList insertNickParams;
        insertNickParams << identityId.toInt()
                         << nick;
        QSqlQuery insertNickQuery = executePreparedQuery("insert_nick", insertNickParams, db);
        if (!watchQuery(insertNickQuery)) {
            db.rollback();
            return IdentityId();
        }
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::createIdentity(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return IdentityId();
    }
    return identityId;
}


bool PostgreSqlStorage::updateIdentity(UserId user, const CoreIdentity &identity)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::updateIdentity(): Unable to start Transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QVariantList checkParams;
    checkParams << identity.id().toInt()
                << user.toInt();
    QSqlQuery checkQuery = executePreparedQuery("select_checkidentity", checkParams, db);

    watchQuery(checkQuery);

    // there should be exactly one identity for the given id and user
    if (!checkQuery.first() || checkQuery.value(0).toInt() != 1) {
        db.rollback();
        return false;
    }

    QVariantList params;
    params << identity.identityName()
           << identity.realName()
           << identity.awayNick()
           << identity.awayNickEnabled()
           << identity.awayReason()
           << identity.awayReasonEnabled()
           << identity.awayReasonEnabled()
           << identity.autoAwayTime()
           << identity.autoAwayReason()
           << identity.autoAwayReasonEnabled()
           << identity.detachAwayEnabled()
           << identity.detachAwayReason()
           << identity.detachAwayReasonEnabled()
           << identity.ident()
           << identity.kickReason()
           << identity.partReason()
           << identity.quitReason()
#ifdef HAVE_SSL
           << identity.sslCert().toPem()
           << identity.sslKey().toPem()
#else
           << QByteArray()
           << QByteArray()
#endif
           << identity.id().toInt();
    QSqlQuery query = executePreparedQuery("update_identity", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return false;
    }

    QVariantList deleteNickParams;
    deleteNickParams << identity.id().toInt();
    QSqlQuery deleteNickQuery = executePreparedQuery("delete_nicks", deleteNickParams, db);

    if (!watchQuery(deleteNickQuery)) {
        db.rollback();
        return false;
    }

    foreach(QString nick, identity.nicks()) {
        QVariantList insertNickParams;
        insertNickParams << identity.id().toInt()
                         << nick;
        QSqlQuery insertNickQuery = executePreparedQuery("insert_nick", insertNickParams, db);

        if (!watchQuery(insertNickQuery)) {
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::updateIdentity(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    return true;
}


void PostgreSqlStorage::removeIdentity(UserId user, IdentityId identityId)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::removeIdentity(): Unable to start Transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << identityId.toInt()
           << user.toInt();
    QSqlQuery query = executePreparedQuery("delete_identity", params, db);

    if (!watchQuery(query)) {
        db.rollback();
    }
    else {
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::removeIdentity(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
        }
    }
}


QList<CoreIdentity> PostgreSqlStorage::identities(UserId user)
{
    QList<CoreIdentity> identities;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::identites(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return identities;
    }

    QSqlQuery query = executePreparedQuery("select_identities", user.toInt(), db);

    while (query.next()) {
        CoreIdentity identity(IdentityId(query.value(0).toInt()));

        identity.setIdentityName(query.value(1).toString());
        identity.setRealName(query.value(2).toString());
        identity.setAwayNick(query.value(3).toString());
        identity.setAwayNickEnabled(!!query.value(4).toInt());
        identity.setAwayReason(query.value(5).toString());
        identity.setAwayReasonEnabled(!!query.value(6).toInt());
        identity.setAutoAwayEnabled(!!query.value(7).toInt());
        identity.setAutoAwayTime(query.value(8).toInt());
        identity.setAutoAwayReason(query.value(9).toString());
        identity.setAutoAwayReasonEnabled(!!query.value(10).toInt());
        identity.setDetachAwayEnabled(!!query.value(11).toInt());
        identity.setDetachAwayReason(query.value(12).toString());
        identity.setDetachAwayReasonEnabled(!!query.value(13).toInt());
        identity.setIdent(query.value(14).toString());
        identity.setKickReason(query.value(15).toString());
        identity.setPartReason(query.value(16).toString());
        identity.setQuitReason(query.value(17).toString());
#ifdef HAVE_SSL
        identity.setSslCert(query.value(18).toByteArray());
        identity.setSslKey(query.value(19).toByteArray());
#endif

        QVariantList nickParams;
        nickParams << identity.id().toInt();
        QSqlQuery nickQuery = executePreparedQuery("select_nicks", nickParams, db);

        watchQuery(nickQuery);

        QList<QString> nicks;
        while (nickQuery.next()) {
            nicks << nickQuery.value(0).toString();
        }
        identity.setNicks(nicks);
        identities << identity;
    }
    db.commit();
    return identities;
}


NetworkId PostgreSqlStorage::createNetwork(UserId user, const NetworkInfo &info)
{
    NetworkId networkId;

    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::createNetwork(): failed to begin transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QVariantList params;
    bindNetworkInfo(params, user, info);
    QSqlQuery query = executePreparedQuery("insert_network", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return NetworkId();
    }

    query.first();
    networkId = query.value(0).toInt();

    if (!networkId.isValid()) {
        db.rollback();
        return NetworkId();
    }

    foreach(Network::Server server, info.serverList) {
        QVariantList insertServersParams;
        insertServersParams << user.toInt()
                            << networkId.toInt();
        bindServerInfo(insertServersParams, server);
        QSqlQuery insertServersQuery = executePreparedQuery("insert_server", insertServersParams, db);

        if (!watchQuery(insertServersQuery)) {
            db.rollback();
            return NetworkId();
        }
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::createNetwork(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return NetworkId();
    }
    return networkId;
}


void PostgreSqlStorage::bindNetworkInfo(QVariantList &params, const UserId &user, const NetworkInfo &info)
{
    params << info.networkName;
    if (info.identity.isValid())
        params << info.identity.toInt();
    else
        params << QVariant();
    params << QString(info.codecForServer)
           << QString(info.codecForEncoding)
           << QString(info.codecForDecoding)
           << info.useRandomServer
           << info.perform.join("\n")
           << info.useAutoIdentify
           << info.autoIdentifyService
           << info.autoIdentifyPassword
           << info.useAutoReconnect
           << info.autoReconnectInterval
           << info.autoReconnectRetries
           << info.unlimitedReconnectRetries
           << info.rejoinChannels
           << info.useSasl
           << info.saslAccount
           << info.saslPassword
           << user.toInt();
    if (info.networkId.isValid())
        params << info.networkId.toInt();
}


void PostgreSqlStorage::bindServerInfo(QVariantList &params, const Network::Server &server)
{
    params << server.host
           << server.port
           << server.password
           << server.useSsl
           << server.sslVersion
           << server.useProxy
           << server.proxyType
           << server.proxyHost
           << server.proxyPort
           << server.proxyUser
           << server.proxyPass;
}


bool PostgreSqlStorage::updateNetwork(UserId user, const NetworkInfo &info)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::updateNetwork(): failed to begin transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QVariantList updateParams;
    bindNetworkInfo(updateParams, user, info);
    QSqlQuery updateQuery = executePreparedQuery("update_network", updateParams, db);

    if (!watchQuery(updateQuery)) {
        db.rollback();
        return false;
    }
    if (updateQuery.numRowsAffected() != 1) {
        // seems this is not our network...
        db.rollback();
        return false;
    }

    QVariantList dropServersParams;
    dropServersParams << info.networkId.toInt();
    QSqlQuery dropServersQuery = executePreparedQuery("delete_ircservers_for_network", dropServersParams, db);

    if (!watchQuery(dropServersQuery)) {
        db.rollback();
        return false;
    }

    foreach(Network::Server server, info.serverList) {
        QVariantList insertServersParams;
        insertServersParams << user.toInt()
                            << info.networkId.toInt();
        bindServerInfo(insertServersParams, server);
        QSqlQuery insertServersQuery = executePreparedQuery("insert_server", insertServersParams, db);

        if (!watchQuery(insertServersQuery)) {
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::updateNetwork(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    return true;
}


bool PostgreSqlStorage::removeNetwork(UserId user, const NetworkId &networkId)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::removeNetwork(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QVariantList params;
    params << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("delete_network", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::removeNetwork(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    return true;
}


QList<NetworkInfo> PostgreSqlStorage::networks(UserId user)
{
    QList<NetworkInfo> nets;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::networks(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return nets;
    }

    QVariantList networksParams;
    networksParams << user.toInt();
    QSqlQuery networksQuery = executePreparedQuery("select_networks_for_user", networksParams, db);

    if (!watchQuery(networksQuery)) {
        db.rollback();
        return nets;
    }

    while (networksQuery.next()) {
        NetworkInfo net;
        net.networkId = networksQuery.value(0).toInt();
        net.networkName = networksQuery.value(1).toString();
        net.identity = networksQuery.value(2).toInt();
        net.codecForServer = networksQuery.value(3).toString().toLatin1();
        net.codecForEncoding = networksQuery.value(4).toString().toLatin1();
        net.codecForDecoding = networksQuery.value(5).toString().toLatin1();
        net.useRandomServer = networksQuery.value(6).toBool();
        net.perform = networksQuery.value(7).toString().split("\n");
        net.useAutoIdentify = networksQuery.value(8).toBool();
        net.autoIdentifyService = networksQuery.value(9).toString();
        net.autoIdentifyPassword = networksQuery.value(10).toString();
        net.useAutoReconnect = networksQuery.value(11).toBool();
        net.autoReconnectInterval = networksQuery.value(12).toUInt();
        net.autoReconnectRetries = networksQuery.value(13).toInt();
        net.unlimitedReconnectRetries = networksQuery.value(14).toBool();
        net.rejoinChannels = networksQuery.value(15).toBool();
        net.useSasl = networksQuery.value(16).toBool();
        net.saslAccount = networksQuery.value(17).toString();
        net.saslPassword = networksQuery.value(18).toString();

        QVariantList serversParams;
        serversParams << net.networkId.toInt();
        QSqlQuery serversQuery = executePreparedQuery("select_servers_for_network", serversParams, db);

        if (!watchQuery(serversQuery)) {
            db.rollback();
            return nets;
        }

        Network::ServerList servers;
        while (serversQuery.next()) {
            Network::Server server;
            server.host = serversQuery.value(0).toString();
            server.port = serversQuery.value(1).toUInt();
            server.password = serversQuery.value(2).toString();
            server.useSsl = serversQuery.value(3).toBool();
            server.sslVersion = serversQuery.value(4).toInt();
            server.useProxy = serversQuery.value(5).toBool();
            server.proxyType = serversQuery.value(6).toInt();
            server.proxyHost = serversQuery.value(7).toString();
            server.proxyPort = serversQuery.value(8).toUInt();
            server.proxyUser = serversQuery.value(9).toString();
            server.proxyPass = serversQuery.value(10).toString();
            servers << server;
        }
        net.serverList = servers;
        nets << net;
    }
    db.commit();
    return nets;
}


QList<NetworkId> PostgreSqlStorage::connectedNetworks(UserId user)
{
    QList<NetworkId> connectedNets;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::connectedNetworks(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return connectedNets;
    }

    QSqlQuery query = executePreparedQuery("select_connected_networks", user.toInt(), db);

    watchQuery(query);

    while (query.next()) {
        connectedNets << query.value(0).toInt();
    }

    db.commit();
    return connectedNets;
}


void PostgreSqlStorage::setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setNetworkConnected(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << isConnected
           << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("update_network_connected", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setNetworkConnected(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


QHash<QString, QString> PostgreSqlStorage::persistentChannels(UserId user, const NetworkId &networkId)
{
    QHash<QString, QString> persistentChans;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::persistentChannels(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return persistentChans;
    }

    QVariantList params;
    params << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("select_persistent_channels", params, db);

    watchQuery(query);

    while (query.next()) {
        persistentChans[query.value(0).toString()] = query.value(1).toString();
    }

    db.commit();
    return persistentChans;
}


void PostgreSqlStorage::setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setChannelPersistent(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << isJoined
           << user.toInt()
           << networkId.toInt()
           << channel.toLower();
    QSqlQuery query = executePreparedQuery("update_buffer_persistent_channel", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setChannelPersistent(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


void PostgreSqlStorage::setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setPersistentChannelKey(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << key
           << user.toInt()
           << networkId.toInt()
           << channel.toLower();
    QSqlQuery query = executePreparedQuery("update_buffer_set_channel_key", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setPersistentChannelKey(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


QString PostgreSqlStorage::awayMessage(UserId user, NetworkId networkId)
{
    QString awayMsg;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::awayMessage(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return awayMsg;
    }

    QVariantList params;
    params << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("select_network_awaymsg", params, db);

    watchQuery(query);

    if (query.first())
        awayMsg = query.value(0).toString();
    db.commit();
    return awayMsg;
}


void PostgreSqlStorage::setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setAwayMessage(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << awayMsg
           << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("update_network_set_awaymsg", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setAwayMessage(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


QString PostgreSqlStorage::userModes(UserId user, NetworkId networkId)
{
    QString modes;
    
    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::userModes(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return modes;
    }

    QVariantList params;
    params << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("select_network_usermode", params, db);

    watchQuery(query);

    if (query.first())
        modes = query.value(0).toString();
    db.commit();
    return modes;
}


void PostgreSqlStorage::setUserModes(UserId user, NetworkId networkId, const QString &userModes)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setUserModes(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << userModes
           << user.toInt()
           << networkId.toInt();
    QSqlQuery query = executePreparedQuery("update_network_set_usermode", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setUserModes(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


BufferInfo PostgreSqlStorage::bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer, bool create)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::bufferInfo(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return BufferInfo();
    }

    QVariantList params;
    params << networkId.toInt()
           << user.toInt()
           << buffer.toLower();
    QSqlQuery query = executePreparedQuery("select_bufferByName", params, db);

    watchQuery(query);

    if (query.first()) {
        BufferInfo bufferInfo = BufferInfo(query.value(0).toInt(), networkId, (BufferInfo::Type)query.value(1).toInt(), 0, buffer);
        if (query.next()) {
            qCritical() << "PostgreSqlStorage::bufferInfo(): received more then one Buffer!";
            qCritical() << "         Query:" << query.lastQuery();
            qCritical() << "  bound Values:";
            QList<QVariant> list = query.boundValues().values();
            for (int i = 0; i < list.size(); ++i)
                qCritical() << i << ":" << list.at(i).toString().toLatin1().data();
            Q_ASSERT(false);
        }
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::bufferInfo(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
        }
        return bufferInfo;
    }

    if (!create) {
        db.rollback();
        return BufferInfo();
    }

    QVariantList createParams;
    createParams << user.toInt()
                 << networkId.toInt()
                 << buffer
                 << buffer.toLower()
                 << (int)type;
    if (type & BufferInfo::ChannelBuffer)
        createParams << true;
    else
        createParams << false;
    QSqlQuery createQuery = executePreparedQuery("insert_buffer", createParams, db);

    if (!watchQuery(createQuery)) {
        qWarning() << "PostgreSqlStorage::bufferInfo(): unable to create buffer";
        db.rollback();
        return BufferInfo();
    }

    createQuery.first();

    BufferInfo bufferInfo = BufferInfo(createQuery.value(0).toInt(), networkId, type, 0, buffer);
    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::bufferInfo(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
    return bufferInfo;
}


BufferInfo PostgreSqlStorage::getBufferInfo(UserId user, const BufferId &bufferId)
{
    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::getBufferInfo(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return BufferInfo();
    }

    QVariantList params;
    params << user.toInt()
           << bufferId.toInt();
    QSqlQuery query = executePreparedQuery("select_buffer_by_id", params, db);

    if (!watchQuery(query) || !query.first()) {
        db.rollback();
        return BufferInfo();
    }

    BufferInfo bufferInfo(query.value(0).toInt(), query.value(1).toInt(), (BufferInfo::Type)query.value(2).toInt(), 0, query.value(4).toString());
    Q_ASSERT(!query.next());

    db.commit();
    return bufferInfo;
}


QList<BufferInfo> PostgreSqlStorage::requestBuffers(UserId user)
{
    QList<BufferInfo> bufferlist;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::requestBuffers(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return bufferlist;
    }

    QSqlQuery query = executePreparedQuery("select_buffers", user.toInt(), db);

    watchQuery(query);

    while (query.next()) {
        bufferlist << BufferInfo(query.value(0).toInt(), query.value(1).toInt(), (BufferInfo::Type)query.value(2).toInt(), query.value(3).toInt(), query.value(4).toString());
    }
    db.commit();
    return bufferlist;
}


QList<BufferId> PostgreSqlStorage::requestBufferIdsForNetwork(UserId user, NetworkId networkId)
{
    QList<BufferId> bufferList;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::requestBufferIdsForNetwork(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return bufferList;
    }

    QVariantList params;
    params << networkId.toInt()
           << user.toInt();
    QSqlQuery query = executePreparedQuery("select_buffers_for_network", params, db);

    watchQuery(query);

    while (query.next()) {
        bufferList << BufferId(query.value(0).toInt());
    }
    db.commit();
    return bufferList;
}


bool PostgreSqlStorage::removeBuffer(const UserId &user, const BufferId &bufferId)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::removeBuffer(): cannot start transaction!";
        return false;
    }

    QVariantList params;
    params << user.toInt()
           << bufferId.toInt();
    QSqlQuery query = executePreparedQuery("delete_buffer_for_bufferid", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return false;
    }

    int numRows = query.numRowsAffected();
    switch (numRows) {
    case 0:
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::removeBuffer(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
        }
        return false;
    case 1:
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::removeBuffer(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
            return false;
        }
        return true;
    default:
        // there was more then one buffer deleted...
        qWarning() << "PostgreSqlStorage::removeBuffer(): Userid" << user << "BufferId" << "caused deletion of" << numRows << "Buffers! Rolling back transaction...";
        db.rollback();
        return false;
    }
}


bool PostgreSqlStorage::renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::renameBuffer(): cannot start transaction!";
        return false;
    }

    QVariantList params;
    params << newName
           << newName.toLower()
           << user.toInt()
           << bufferId.toInt();
    QSqlQuery query = executePreparedQuery("update_buffer_name", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return false;
    }

    int numRows = query.numRowsAffected();
    switch (numRows) {
    case 0:
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::renameBuffer(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
        }
        return false;
    case 1:
        if (!db.commit()) {
            qWarning() << "PostgreSqlStorage::renameBuffer(): committing data failed!";
            qWarning() << " -" << qPrintable(db.lastError().text());
            return false;
        }
        return true;
    default:
        // there was more then one buffer deleted...
        qWarning() << "PostgreSqlStorage::renameBuffer(): Userid" << user << "BufferId" << "affected" << numRows << "Buffers! Rolling back transaction...";
        db.rollback();
        return false;
    }
}


bool PostgreSqlStorage::mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::mergeBuffersPermanently(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QVariantList checkParams;
    checkParams << user.toInt()
                << bufferId1.toInt()
                << bufferId2.toInt();
    QSqlQuery checkQuery = executePreparedQuery("select_buffer_merge_check", checkParams, db);

    if (!watchQuery(checkQuery)) {
        db.rollback();
        return false;
    }
    checkQuery.first();
    if (checkQuery.value(0).toInt() != 2) {
        db.rollback();
        return false;
    }

    QVariantList params;
    params << bufferId1.toInt()
           << bufferId2.toInt();
    QSqlQuery query = executePreparedQuery("update_backlog_bufferid", params, db);

    if (!watchQuery(query)) {
        db.rollback();
        return false;
    }

    QVariantList delBufferParams;
    delBufferParams << user.toInt()
                    << bufferId2.toInt();
    QSqlQuery delBufferQuery = executePreparedQuery("delete_buffer_for_bufferid", delBufferParams, db);

    if (!watchQuery(delBufferQuery)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::mergeBuffersPermanently(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    return true;
}


void PostgreSqlStorage::setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setBufferLastSeenMsg(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << msgId.toInt()
           << user.toInt()
           << bufferId.toInt();
    QSqlQuery query = executePreparedQuery("update_buffer_lastseen", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setBufferLastSeenMsg(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


QHash<BufferId, MsgId> PostgreSqlStorage::bufferLastSeenMsgIds(UserId user)
{
    QHash<BufferId, MsgId> lastSeenHash;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::bufferLastSeenMsgIds(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return lastSeenHash;
    }

    QSqlQuery query = executePreparedQuery("select_buffer_lastseen_messages", user.toInt(), db);

    if (!watchQuery(query)) {
        db.rollback();
        return lastSeenHash;
    }

    while (query.next()) {
        lastSeenHash[query.value(0).toInt()] = query.value(1).toInt();
    }

    db.commit();
    return lastSeenHash;
}


void PostgreSqlStorage::setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::setBufferMarkerLineMsg(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return;
    }

    QVariantList params;
    params << msgId.toInt()
           << user.toInt()
           << bufferId.toInt();
    QSqlQuery query = executePreparedQuery("update_buffer_markerlinemsgid", params, db);

    watchQuery(query);

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::setBufferMarkerLineMsg(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
    }
}


QHash<BufferId, MsgId> PostgreSqlStorage::bufferMarkerLineMsgIds(UserId user)
{
    QHash<BufferId, MsgId> markerLineHash;

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::bufferMarkerLineMsgIds(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return markerLineHash;
    }

    QSqlQuery query = executePreparedQuery("select_buffer_markerlinemsgids", user.toInt(), db);

    if (!watchQuery(query)) {
        db.rollback();
        return markerLineHash;
    }

    while (query.next()) {
        markerLineHash[query.value(0).toInt()] = query.value(1).toInt();
    }

    db.commit();
    return markerLineHash;
}


bool PostgreSqlStorage::logMessage(Message &msg)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::logMessage(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QSqlQuery getSenderIdQuery = executePreparedQuery("select_senderid", msg.sender(), db);
    int senderId;
    if (getSenderIdQuery.first()) {
        senderId = getSenderIdQuery.value(0).toInt();
    }
    else {
        // it's possible that the sender was already added by another thread
        // since the insert might fail we're setting a savepoint
        savePoint("sender_sp1", db);
        QSqlQuery addSenderQuery = executePreparedQuery("insert_sender", msg.sender(), db);

        if (!watchQuery(addSenderQuery)) {
            rollbackSavePoint("sender_sp1", db);
            getSenderIdQuery = db.exec(getSenderIdQuery.lastQuery());
            getSenderIdQuery.first();
            senderId = getSenderIdQuery.value(0).toInt();
        }
        else {
            releaseSavePoint("sender_sp1", db);
            addSenderQuery.first();
            senderId = addSenderQuery.value(0).toInt();
        }
    }

    QVariantList params;
    params << msg.timestamp()
           << msg.bufferInfo().bufferId().toInt()
           << msg.type()
           << (int)msg.flags()
           << senderId
           << msg.contents();
    QSqlQuery logMessageQuery = executePreparedQuery("insert_message", params, db);

    if (!watchQuery(logMessageQuery)) {
        db.rollback();
        return false;
    }

    logMessageQuery.first();
    MsgId msgId = logMessageQuery.value(0).toInt();
    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::logMessage(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    if (msgId.isValid()) {
        msg.setMsgId(msgId);
        return true;
    }
    else {
        return false;
    }
}


bool PostgreSqlStorage::logMessages(MessageList &msgs)
{
    QSqlDatabase db = logDb();
    if (!beginTransaction(db)) {
        qWarning() << "PostgreSqlStorage::logMessage(): cannot start transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }

    QList<int> senderIdList;
    QHash<QString, int> senderIds;
    QSqlQuery addSenderQuery;
    QSqlQuery selectSenderQuery;
    for (int i = 0; i < msgs.count(); i++) {
        const QString &sender = msgs.at(i).sender();
        if (senderIds.contains(sender)) {
            senderIdList << senderIds[sender];
            continue;
        }

        selectSenderQuery = executePreparedQuery("select_senderid", sender, db);
        watchQuery(selectSenderQuery);
        if (selectSenderQuery.first()) {
            senderIdList << selectSenderQuery.value(0).toInt();
            senderIds[sender] = selectSenderQuery.value(0).toInt();
        }
        else {
            savePoint("sender_sp", db);
            addSenderQuery = executePreparedQuery("insert_sender", sender, db);
            if (!watchQuery(addSenderQuery)) {
                // seems it was inserted meanwhile... by a different thread
                rollbackSavePoint("sender_sp", db);
                selectSenderQuery = db.exec(selectSenderQuery.lastQuery());
                selectSenderQuery.first();
                senderIdList << selectSenderQuery.value(0).toInt();
                senderIds[sender] = selectSenderQuery.value(0).toInt();
            }
            else {
                releaseSavePoint("sender_sp", db);
                addSenderQuery.first();
                senderIdList << addSenderQuery.value(0).toInt();
                senderIds[sender] = addSenderQuery.value(0).toInt();
            }
        }
    }

    // yes we loop twice over the same list. This avoids alternating queries.
    bool error = false;
    for (int i = 0; i < msgs.count(); i++) {
        Message &msg = msgs[i];
        QVariantList params;
        params << msg.timestamp()
               << msg.bufferInfo().bufferId().toInt()
               << msg.type()
               << (int)msg.flags()
               << senderIdList.at(i)
               << msg.contents();
        QSqlQuery logMessageQuery = executePreparedQuery("insert_message", params, db);
        if (!watchQuery(logMessageQuery)) {
            db.rollback();
            error = true;
            break;
        }
        else {
            logMessageQuery.first();
            msg.setMsgId(logMessageQuery.value(0).toInt());
        }
    }

    if (error) {
        // we had a rollback in the db so we need to reset all msgIds
        for (int i = 0; i < msgs.count(); i++) {
            msgs[i].setMsgId(MsgId());
        }
        return false;
    }

    if (!db.commit()) {
        qWarning() << "PostgreSqlStorage::logMessages(): committing data failed!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return false;
    }
    return true;
}


QList<Message> PostgreSqlStorage::requestMsgs(UserId user, BufferId bufferId, MsgId first, MsgId last, int limit)
{
    QList<Message> messagelist;

    BufferInfo bufferInfo = getBufferInfo(user, bufferId);
    if (!bufferInfo.isValid()) {
        return messagelist;
    }

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::requestMsgs(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return messagelist;
	}

    QString queryName;
    QVariantList params;
    if (last == -1 && first == -1) {
        queryName = "select_messages";
    }
    else if (last == -1) {
        queryName = "select_messagesNewerThan";
        params << first.toInt();
    }
    else {
        queryName = "select_messagesRange";
        params << first.toInt();
        params << last.toInt();
    }
    params << bufferId.toInt();
    if (limit != -1)
        params << limit;
    else
        params << QVariant(QVariant::Int);

    QSqlQuery query = executePreparedQuery(queryName, params, db);

    if (!watchQuery(query)) {
        qDebug() << "select_messages failed";
        db.rollback();
        return messagelist;
    }

    QDateTime timestamp;
    while (query.next()) {
        timestamp = query.value(1).toDateTime();
        timestamp.setTimeSpec(Qt::UTC);
        Message msg(timestamp,
            bufferInfo,
            (Message::Type)query.value(2).toUInt(),
            query.value(5).toString(),
            query.value(4).toString(),
            (Message::Flags)query.value(3).toUInt());
        msg.setMsgId(query.value(0).toInt());
        messagelist << msg;
    }

    db.commit();
    return messagelist;
}


QList<Message> PostgreSqlStorage::requestAllMsgs(UserId user, MsgId first, MsgId last, int limit)
{
    QList<Message> messagelist;

    // requestBuffers uses it's own transaction.
    QHash<BufferId, BufferInfo> bufferInfoHash;
    foreach(BufferInfo bufferInfo, requestBuffers(user)) {
        bufferInfoHash[bufferInfo.bufferId()] = bufferInfo;
    }

    QSqlDatabase db = logDb();
    if (!beginReadOnlyTransaction(db)) {
        qWarning() << "PostgreSqlStorage::requestAllMsgs(): cannot start read only transaction!";
        qWarning() << " -" << qPrintable(db.lastError().text());
        return messagelist;
    }

    QVariantList params;
    params << user.toInt();
    params << first.toInt();

    QSqlQuery query;
    if (last == -1) {
        query = executePreparedQuery("select_messagesAllNew", params, db);
    }
    else {
        params << last.toInt();
        query = executePreparedQuery("select_messagesAll", params, db);
    }

    if (!watchQuery(query)) {
        db.rollback();
        return messagelist;
    }

    QDateTime timestamp;
    for (int i = 0; i < limit && query.next(); i++) {
        timestamp = query.value(1).toDateTime();
        timestamp.setTimeSpec(Qt::UTC);
        Message msg(timestamp,
            bufferInfoHash[query.value(1).toInt()],
            (Message::Type)query.value(3).toUInt(),
            query.value(6).toString(),
            query.value(5).toString(),
            (Message::Flags)query.value(4).toUInt());
        msg.setMsgId(query.value(0).toInt());
        messagelist << msg;
    }

    db.commit();
    return messagelist;
}


// void PostgreSqlStorage::safeExec(QSqlQuery &query) {
//   qDebug() << "PostgreSqlStorage::safeExec";
//   qDebug() << "   executing:\n" << query.executedQuery();
//   qDebug() << "   bound Values:";
//   QList<QVariant> list = query.boundValues().values();
//   for (int i = 0; i < list.size(); ++i)
//     qCritical() << i << ": " << list.at(i).toString().toLatin1().data();

//   query.exec();

//   qDebug() << "Success:" << !query.lastError().isValid();
//   qDebug();

//   if(!query.lastError().isValid())
//     return;

//   qDebug() << "==================== ERROR ====================";
//   watchQuery(query);
//   qDebug() << "===============================================";
//   qDebug();
//   return;
// }

bool PostgreSqlStorage::beginTransaction(QSqlDatabase &db)
{
    bool result = db.transaction();
    if (!db.isOpen()) {
        db = logDb();
        result = db.transaction();
    }
    return result;
}

bool PostgreSqlStorage::beginReadOnlyTransaction(QSqlDatabase &db)
{
    QSqlQuery query = db.exec("BEGIN TRANSACTION READ ONLY");
    if (!db.isOpen()) {
        db = logDb();
        query = db.exec("BEGIN TRANSACTION READ ONLY");
    }
    return !query.lastError().isValid();
}

QSqlQuery PostgreSqlStorage::prepareAndExecuteQuery(const QString &queryname, const QString &paramstring, QSqlDatabase &db)
{
    // Query preparing is done lazily. That means that instead of always checking if the query is already prepared
    // we just EXECUTE and catch the error
    QSqlQuery query;

    db.exec("SAVEPOINT quassel_prepare_query");
    if (paramstring.isNull()) {
        query = db.exec(QString("EXECUTE quassel_%1").arg(queryname));
    }
    else {
        query = db.exec(QString("EXECUTE quassel_%1 (%2)").arg(queryname).arg(paramstring));
    }

    if (!db.isOpen() || db.lastError().isValid()) {
        // If the query failed because the DB connection was down, reopen the connection and start a new transaction.
        if (!db.isOpen()) {
            db = logDb();
            if (!beginTransaction(db)) {
                qWarning() << "PostgreSqlStorage::prepareAndExecuteQuery(): cannot start transaction while recovering from connection loss!";
                qWarning() << " -" << qPrintable(db.lastError().text());
                return query;
            }
            db.exec("SAVEPOINT quassel_prepare_query");
        } else {
            db.exec("ROLLBACK TO SAVEPOINT quassel_prepare_query");
        }
        
        // and once again: Qt leaves us without error codes so we either parse (language dependent(!)) strings
        // or we just guess the error. As we're only interested in unprepared queries, this will be our guess. :)
        QSqlQuery checkQuery = db.exec(QString("SELECT count(name) FROM pg_prepared_statements WHERE name = 'quassel_%1' AND from_sql = TRUE").arg(queryname.toLower()));
        checkQuery.first();
        if (checkQuery.value(0).toInt() == 0) {
            db.exec(QString("PREPARE quassel_%1 AS %2").arg(queryname).arg(queryString(queryname)));
            if (db.lastError().isValid()) {
                qWarning() << "PostgreSqlStorage::prepareQuery(): unable to prepare query:" << queryname << "AS" << queryString(queryname);
                qWarning() << "  Error:" << db.lastError().text();
                return QSqlQuery(db);
            }
        }
        // we always execute the query again, even if the query was already prepared.
        // this ensures, that the error is properly propagated to the calling function
        // (otherwise the last call would be the testing select to pg_prepared_statements
        // which always gives a proper result and the error would be lost)
        if (paramstring.isNull()) {
            query = db.exec(QString("EXECUTE quassel_%1").arg(queryname));
        }
        else {
            query = db.exec(QString("EXECUTE quassel_%1 (%2)").arg(queryname).arg(paramstring));
        }
    }
    else {
        // only release the SAVEPOINT
        db.exec("RELEASE SAVEPOINT quassel_prepare_query");
    }
    return query;
}


QSqlQuery PostgreSqlStorage::executePreparedQuery(const QString &queryname, const QVariantList &params, QSqlDatabase &db)
{
    QSqlDriver *driver = db.driver();

    QStringList paramStrings;
    QSqlField field;
    for (int i = 0; i < params.count(); i++) {
        const QVariant &value = params.at(i);
        field.setType(value.type());
        if (value.isNull())
            field.clear();
        else
            field.setValue(value);

        paramStrings << driver->formatValue(field);
    }

    if (params.isEmpty()) {
        return prepareAndExecuteQuery(queryname, db);
    }
    else {
        return prepareAndExecuteQuery(queryname, paramStrings.join(", "), db);
    }
}


QSqlQuery PostgreSqlStorage::executePreparedQuery(const QString &queryname, const QVariant &param, QSqlDatabase &db)
{
    QSqlField field;
    field.setType(param.type());
    if (param.isNull())
        field.clear();
    else
        field.setValue(param);

    QString paramString = db.driver()->formatValue(field);
    return prepareAndExecuteQuery(queryname, paramString, db);
}


void PostgreSqlStorage::deallocateQuery(const QString &queryname, const QSqlDatabase &db)
{
    db.exec(QString("DEALLOCATE quassel_%1").arg(queryname));
}


// ========================================
//  PostgreSqlMigrationWriter
// ========================================
PostgreSqlMigrationWriter::PostgreSqlMigrationWriter()
    : PostgreSqlStorage()
{
}


bool PostgreSqlMigrationWriter::prepareQuery(MigrationObject mo)
{
    QString query;
    switch (mo) {
    case QuasselUser:
        query = queryString("migrate_write_quasseluser");
        break;
    case Sender:
        query = queryString("migrate_write_sender");
        break;
    case Identity:
        _validIdentities.clear();
        query = queryString("migrate_write_identity");
        break;
    case IdentityNick:
        query = queryString("migrate_write_identity_nick");
        break;
    case Network:
        query = queryString("migrate_write_network");
        break;
    case Buffer:
        query = queryString("migrate_write_buffer");
        break;
    case Backlog:
        query = queryString("migrate_write_backlog");
        break;
    case IrcServer:
        query = queryString("migrate_write_ircserver");
        break;
    case UserSetting:
        query = queryString("migrate_write_usersetting");
        break;
    }
    newQuery(query, logDb());
    return true;
}


//bool PostgreSqlMigrationWriter::writeUser(const QuasselUserMO &user) {
bool PostgreSqlMigrationWriter::writeMo(const QuasselUserMO &user)
{
    bindValue(0, user.id.toInt());
    bindValue(1, user.username);
    bindValue(2, user.password);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeSender(const SenderMO &sender) {
bool PostgreSqlMigrationWriter::writeMo(const SenderMO &sender)
{
    bindValue(0, sender.senderId);
    bindValue(1, sender.sender);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeIdentity(const IdentityMO &identity) {
bool PostgreSqlMigrationWriter::writeMo(const IdentityMO &identity)
{
    _validIdentities << identity.id.toInt();
    bindValue(0, identity.id.toInt());
    bindValue(1, identity.userid.toInt());
    bindValue(2, identity.identityname);
    bindValue(3, identity.realname);
    bindValue(4, identity.awayNick);
    bindValue(5, identity.awayNickEnabled);
    bindValue(6, identity.awayReason);
    bindValue(7, identity.awayReasonEnabled);
    bindValue(8, identity.autoAwayEnabled);
    bindValue(9, identity.autoAwayTime);
    bindValue(10, identity.autoAwayReason);
    bindValue(11, identity.autoAwayReasonEnabled);
    bindValue(12, identity.detachAwayEnabled);
    bindValue(13, identity.detachAwayReason);
    bindValue(14, identity.detchAwayReasonEnabled);
    bindValue(15, identity.ident);
    bindValue(16, identity.kickReason);
    bindValue(17, identity.partReason);
    bindValue(18, identity.quitReason);
    bindValue(19, identity.sslCert);
    bindValue(20, identity.sslKey);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeIdentityNick(const IdentityNickMO &identityNick) {
bool PostgreSqlMigrationWriter::writeMo(const IdentityNickMO &identityNick)
{
    bindValue(0, identityNick.nickid);
    bindValue(1, identityNick.identityId.toInt());
    bindValue(2, identityNick.nick);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeNetwork(const NetworkMO &network) {
bool PostgreSqlMigrationWriter::writeMo(const NetworkMO &network)
{
    bindValue(0, network.networkid.toInt());
    bindValue(1, network.userid.toInt());
    bindValue(2, network.networkname);
    if (_validIdentities.contains(network.identityid.toInt()))
        bindValue(3, network.identityid.toInt());
    else
        bindValue(3, QVariant());
    bindValue(4, network.encodingcodec);
    bindValue(5, network.decodingcodec);
    bindValue(6, network.servercodec);
    bindValue(7, network.userandomserver);
    bindValue(8, network.perform);
    bindValue(9, network.useautoidentify);
    bindValue(10, network.autoidentifyservice);
    bindValue(11, network.autoidentifypassword);
    bindValue(12, network.useautoreconnect);
    bindValue(13, network.autoreconnectinterval);
    bindValue(14, network.autoreconnectretries);
    bindValue(15, network.unlimitedconnectretries);
    bindValue(16, network.rejoinchannels);
    bindValue(17, network.connected);
    bindValue(18, network.usermode);
    bindValue(19, network.awaymessage);
    bindValue(20, network.attachperform);
    bindValue(21, network.detachperform);
    bindValue(22, network.usesasl);
    bindValue(23, network.saslaccount);
    bindValue(24, network.saslpassword);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeBuffer(const BufferMO &buffer) {
bool PostgreSqlMigrationWriter::writeMo(const BufferMO &buffer)
{
    bindValue(0, buffer.bufferid.toInt());
    bindValue(1, buffer.userid.toInt());
    bindValue(2, buffer.groupid);
    bindValue(3, buffer.networkid.toInt());
    bindValue(4, buffer.buffername);
    bindValue(5, buffer.buffercname);
    bindValue(6, (int)buffer.buffertype);
    bindValue(7, buffer.lastseenmsgid);
    bindValue(8, buffer.markerlinemsgid);
    bindValue(9, buffer.key);
    bindValue(10, buffer.joined);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeBacklog(const BacklogMO &backlog) {
bool PostgreSqlMigrationWriter::writeMo(const BacklogMO &backlog)
{
    bindValue(0, backlog.messageid.toInt());
    bindValue(1, backlog.time);
    bindValue(2, backlog.bufferid.toInt());
    bindValue(3, backlog.type);
    bindValue(4, (int)backlog.flags);
    bindValue(5, backlog.senderid);
    bindValue(6, backlog.message);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeIrcServer(const IrcServerMO &ircserver) {
bool PostgreSqlMigrationWriter::writeMo(const IrcServerMO &ircserver)
{
    bindValue(0, ircserver.serverid);
    bindValue(1, ircserver.userid.toInt());
    bindValue(2, ircserver.networkid.toInt());
    bindValue(3, ircserver.hostname);
    bindValue(4, ircserver.port);
    bindValue(5, ircserver.password);
    bindValue(6, ircserver.ssl);
    bindValue(7, ircserver.sslversion);
    bindValue(8, ircserver.useproxy);
    bindValue(9, ircserver.proxytype);
    bindValue(10, ircserver.proxyhost);
    bindValue(11, ircserver.proxyport);
    bindValue(12, ircserver.proxyuser);
    bindValue(13, ircserver.proxypass);
    return exec();
}


//bool PostgreSqlMigrationWriter::writeUserSetting(const UserSettingMO &userSetting) {
bool PostgreSqlMigrationWriter::writeMo(const UserSettingMO &userSetting)
{
    bindValue(0, userSetting.userid.toInt());
    bindValue(1, userSetting.settingname);
    bindValue(2, userSetting.settingvalue);
    return exec();
}


bool PostgreSqlMigrationWriter::postProcess()
{
    QSqlDatabase db = logDb();
    QList<Sequence> sequences;
    sequences << Sequence("backlog", "messageid")
              << Sequence("buffer", "bufferid")
              << Sequence("identity", "identityid")
              << Sequence("identity_nick", "nickid")
              << Sequence("ircserver", "serverid")
              << Sequence("network", "networkid")
              << Sequence("quasseluser", "userid")
              << Sequence("sender", "senderid");
    QList<Sequence>::const_iterator iter;
    for (iter = sequences.constBegin(); iter != sequences.constEnd(); iter++) {
        resetQuery();
        newQuery(QString("SELECT setval('%1_%2_seq', max(%2)) FROM %1").arg(iter->table, iter->field), db);
        if (!exec())
            return false;
    }
    return true;
}

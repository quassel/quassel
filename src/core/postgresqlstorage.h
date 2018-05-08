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

#pragma once

#include "abstractsqlstorage.h"

#include <QSqlDatabase>
#include <QSqlQuery>

class PostgreSqlStorage : public AbstractSqlStorage
{
    Q_OBJECT

public:
    PostgreSqlStorage(QObject *parent = 0);
    ~PostgreSqlStorage() override;

    std::unique_ptr<AbstractSqlMigrationWriter> createMigrationWriter() override;

public slots:
    /* General */
    bool isAvailable() const override;
    QString backendId() const override;
    QString displayName() const override;
    QString description() const override;
    QVariantList setupData() const override;

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */

    UserId addUser(const QString &user, const QString &password, const QString &authenticator = "Database") override;
    bool updateUser(UserId user, const QString &password) override;
    void renameUser(UserId user, const QString &newName) override;
    UserId validateUser(const QString &user, const QString &password) override;
    UserId getUserId(const QString &username) override;
    QString getUserAuthenticator(const UserId userid) override;
    UserId internalUser() override;
    void delUser(UserId user) override;
    void setUserSetting(UserId userId, const QString &settingName, const QVariant &data) override;
    QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &defaultData = QVariant()) override;

    /* Identity handling */
    IdentityId createIdentity(UserId user, CoreIdentity &identity) override;
    bool updateIdentity(UserId user, const CoreIdentity &identity) override;
    void removeIdentity(UserId user, IdentityId identityId) override;
    QList<CoreIdentity> identities(UserId user) override;

    /* Network handling */
    NetworkId createNetwork(UserId user, const NetworkInfo &info) override;
    bool updateNetwork(UserId user, const NetworkInfo &info) override;
    bool removeNetwork(UserId user, const NetworkId &networkId) override;
    QList<NetworkInfo> networks(UserId user) override;
    QList<NetworkId> connectedNetworks(UserId user) override;
    void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected) override;

    /* persistent channels */
    QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId) override;
    void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined) override;
    void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key) override;

    /* persistent user states */
    QString awayMessage(UserId user, NetworkId networkId) override;
    void setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg) override;
    QString userModes(UserId user, NetworkId networkId) override;
    void setUserModes(UserId user, NetworkId networkId, const QString &userModes) override;

    /* Buffer handling */
    BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "", bool create = true) override;
    BufferInfo getBufferInfo(UserId user, const BufferId &bufferId) override;
    QList<BufferInfo> requestBuffers(UserId user) override;
    QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId) override;
    bool removeBuffer(const UserId &user, const BufferId &bufferId) override;
    bool renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName) override;
    bool mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2) override;
    void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId) override;
    QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user) override;
    void setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId) override;
    QHash<BufferId, MsgId> bufferMarkerLineMsgIds(UserId user) override;
    void setBufferActivity(UserId id, BufferId bufferId, Message::Types type) override;
    QHash<BufferId, Message::Types> bufferActivities(UserId id) override;
    Message::Types bufferActivity(BufferId bufferId, MsgId lastSeenMsgId) override;
    QHash<QString, QByteArray> bufferCiphers(UserId user, const NetworkId &networkId) override;
    void setBufferCipher(UserId user, const NetworkId &networkId, const QString &bufferName, const QByteArray &cipher) override;

    /* Message handling */
    bool logMessage(Message &msg) override;
    bool logMessages(MessageList &msgs) override;
    QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1) override;
    QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1) override;

    /* Sysident handling */
    QMap<UserId, QString> getAllAuthUserNames() override;
    QString getAuthUserName(UserId user) override;

protected:
    bool initDbSession(QSqlDatabase &db) override;
    void setConnectionProperties(const QVariantMap &properties) override;
    QString driverName()  override { return "QPSQL"; }
    QString hostName()  override { return _hostName; }
    int port()  override { return _port; }
    QString databaseName()  override { return _databaseName; }
    QString userName()  override { return _userName; }
    QString password()  override { return _password; }
    int installedSchemaVersion() override;
    bool updateSchemaVersion(int newVersion) override;
    bool setupSchemaVersion(int version) override;
    void safeExec(QSqlQuery &query);

    bool beginTransaction(QSqlDatabase &db);
    bool beginReadOnlyTransaction(QSqlDatabase &db);

    QSqlQuery executePreparedQuery(const QString &queryname, const QVariantList &params, QSqlDatabase &db);
    QSqlQuery executePreparedQuery(const QString &queryname, const QVariant &param, QSqlDatabase &db);
    void deallocateQuery(const QString &queryname, const QSqlDatabase &db);

    void savePoint(const QString &handle, const QSqlDatabase &db) { db.exec(QString("SAVEPOINT %1").arg(handle)); }
    void rollbackSavePoint(const QString &handle, const QSqlDatabase &db) { db.exec(QString("ROLLBACK TO SAVEPOINT %1").arg(handle)); }
    void releaseSavePoint(const QString &handle, const QSqlDatabase &db) { db.exec(QString("RELEASE SAVEPOINT %1").arg(handle)); }

private:
    void bindNetworkInfo(QSqlQuery &query, const NetworkInfo &info);
    void bindServerInfo(QSqlQuery &query, const Network::Server &server);
    QSqlQuery prepareAndExecuteQuery(const QString &queryname, const QString &paramstring, QSqlDatabase &db);
    QSqlQuery prepareAndExecuteQuery(const QString &queryname, QSqlDatabase &db) { return prepareAndExecuteQuery(queryname, QString(), db); }

    QString _hostName;
    int _port;
    QString _databaseName;
    QString _userName;
    QString _password;
};


// ========================================
//  PostgreSqlMigration
// ========================================
class PostgreSqlMigrationWriter : public PostgreSqlStorage, public AbstractSqlMigrationWriter
{
    Q_OBJECT

public:
    PostgreSqlMigrationWriter();

    bool writeMo(const QuasselUserMO &user) override;
    bool writeMo(const SenderMO &sender) override;
    bool writeMo(const IdentityMO &identity) override;
    bool writeMo(const IdentityNickMO &identityNick) override;
    bool writeMo(const NetworkMO &network) override;
    bool writeMo(const BufferMO &buffer) override;
    bool writeMo(const BacklogMO &backlog) override;
    bool writeMo(const IrcServerMO &ircserver) override;
    bool writeMo(const UserSettingMO &userSetting) override;

    bool prepareQuery(MigrationObject mo) override;

    bool postProcess() override;

protected:
    inline bool transaction()  override { return logDb().transaction(); }
    inline void rollback()  override { logDb().rollback(); }
    inline bool commit()  override { return logDb().commit(); }

private:
    // helper struct
    struct Sequence {
        QLatin1String table;
        QLatin1String field;
        Sequence(const char *table, const char *field) : table(table), field(field) {}
    };

    QSet<int> _validIdentities;
};

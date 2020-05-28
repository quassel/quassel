/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <QSqlDatabase>
#include <QSqlQuery>

#include "abstractsqlstorage.h"

class PostgreSqlStorage : public AbstractSqlStorage
{
    Q_OBJECT

public:
    PostgreSqlStorage(QObject* parent = nullptr);

    std::unique_ptr<AbstractSqlMigrationWriter> createMigrationWriter() override;

    /* General */
    bool isAvailable() const override;
    QString backendId() const override;
    QString displayName() const override;
    QString description() const override;
    QVariantList setupData() const override;

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */

    UserId addUser(const QString& user, const QString& password, const QString& authenticator = "Database") override;
    bool updateUser(UserId user, const QString& password) override;
    void renameUser(UserId user, const QString& newName) override;
    UserId validateUser(const QString& user, const QString& password) override;
    UserId getUserId(const QString& username) override;
    QString getUserAuthenticator(const UserId userid) override;
    UserId internalUser() override;
    void delUser(UserId user) override;
    void setUserSetting(UserId userId, const QString& settingName, const QVariant& data) override;
    QVariant getUserSetting(UserId userId, const QString& settingName, const QVariant& defaultData = QVariant()) override;
    void setCoreState(const QVariantList& data) override;
    QVariantList getCoreState(const QVariantList& data) override;

    /* Identity handling */
    IdentityId createIdentity(UserId user, CoreIdentity& identity) override;
    bool updateIdentity(UserId user, const CoreIdentity& identity) override;
    void removeIdentity(UserId user, IdentityId identityId) override;
    std::vector<CoreIdentity> identities(UserId user) override;

    /* Network handling */
    NetworkId createNetwork(UserId user, const NetworkInfo& info) override;
    bool updateNetwork(UserId user, const NetworkInfo& info) override;
    bool removeNetwork(UserId user, const NetworkId& networkId) override;
    std::vector<NetworkInfo> networks(UserId user) override;
    std::vector<NetworkId> connectedNetworks(UserId user) override;
    void setNetworkConnected(UserId user, const NetworkId& networkId, bool isConnected) override;

    /* persistent channels */
    QHash<QString, QString> persistentChannels(UserId user, const NetworkId& networkId) override;
    void setChannelPersistent(UserId user, const NetworkId& networkId, const QString& channel, bool isJoined) override;
    void setPersistentChannelKey(UserId user, const NetworkId& networkId, const QString& channel, const QString& key) override;

    /* persistent user states */
    QString awayMessage(UserId user, NetworkId networkId) override;
    void setAwayMessage(UserId user, NetworkId networkId, const QString& awayMsg) override;
    QString userModes(UserId user, NetworkId networkId) override;
    void setUserModes(UserId user, NetworkId networkId, const QString& userModes) override;

    /* Buffer handling */
    BufferInfo bufferInfo(UserId user, const NetworkId& networkId, BufferInfo::Type type, const QString& buffer = "", bool create = true) override;
    BufferInfo getBufferInfo(UserId user, const BufferId& bufferId) override;
    std::vector<BufferInfo> requestBuffers(UserId user) override;
    std::vector<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId) override;
    bool removeBuffer(const UserId& user, const BufferId& bufferId) override;
    bool renameBuffer(const UserId& user, const BufferId& bufferId, const QString& newName) override;
    bool mergeBuffersPermanently(const UserId& user, const BufferId& bufferId1, const BufferId& bufferId2) override;
    QHash<BufferId, MsgId> bufferLastMsgIds(UserId user) override;
    void setBufferLastSeenMsg(UserId user, const BufferId& bufferId, const MsgId& msgId) override;
    QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user) override;
    void setBufferMarkerLineMsg(UserId user, const BufferId& bufferId, const MsgId& msgId) override;
    QHash<BufferId, MsgId> bufferMarkerLineMsgIds(UserId user) override;
    void setBufferActivity(UserId id, BufferId bufferId, Message::Types type) override;
    QHash<BufferId, Message::Types> bufferActivities(UserId id) override;
    Message::Types bufferActivity(BufferId bufferId, MsgId lastSeenMsgId) override;
    void setHighlightCount(UserId id, BufferId bufferId, int count) override;
    QHash<BufferId, int> highlightCounts(UserId id) override;
    int highlightCount(BufferId bufferId, MsgId lastSeenMsgId) override;
    QHash<QString, QByteArray> bufferCiphers(UserId user, const NetworkId& networkId) override;
    void setBufferCipher(UserId user, const NetworkId& networkId, const QString& bufferName, const QByteArray& cipher) override;

    /* Message handling */
    bool logMessage(Message& msg) override;
    bool logMessages(MessageList& msgs) override;
    std::vector<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1) override;
    std::vector<Message> requestMsgsFiltered(UserId user,
                                             BufferId bufferId,
                                             MsgId first = -1,
                                             MsgId last = -1,
                                             int limit = -1,
                                             Message::Types type = Message::Types{-1},
                                             Message::Flags flags = Message::Flags{-1}) override;
    std::vector<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1) override;
    std::vector<Message> requestAllMsgsFiltered(UserId user,
                                                MsgId first = -1,
                                                MsgId last = -1,
                                                int limit = -1,
                                                Message::Types type = Message::Types{-1},
                                                Message::Flags flags = Message::Flags{-1}) override;

    /* Sysident handling */
    QMap<UserId, QString> getAllAuthUserNames() override;

protected:
    bool initDbSession(QSqlDatabase& db) override;
    void setConnectionProperties(const QVariantMap& properties, const QProcessEnvironment& environment, bool loadFromEnvironment) override;
    QString driverName() override { return "QPSQL"; }
    QString hostName() override { return _hostName; }
    int port() override { return _port; }
    QString databaseName() override { return _databaseName; }
    QString userName() override { return _userName; }
    QString password() override { return _password; }
    int installedSchemaVersion() override;
    bool updateSchemaVersion(int newVersion, bool clearUpgradeStep) override;
    bool setupSchemaVersion(int version) override;

    /**
     * Gets the last successful schema upgrade step, or an empty string if no upgrade is in progress
     *
     * @return Filename of last successful schema upgrade query, or empty string if not upgrading
     */
    QString schemaVersionUpgradeStep() override;

    /**
     * Sets the last successful schema upgrade step
     *
     * @param upgradeQuery  The filename of the last successful schema upgrade query
     * @return True if successfully set, otherwise false
     */
    virtual bool setSchemaVersionUpgradeStep(QString upgradeQuery) override;

    void safeExec(QSqlQuery& query);

    bool beginTransaction(QSqlDatabase& db);
    bool beginReadOnlyTransaction(QSqlDatabase& db);

    QSqlQuery executePreparedQuery(const QString& queryname, const QVariantList& params, QSqlDatabase& db);
    QSqlQuery executePreparedQuery(const QString& queryname, const QVariant& param, QSqlDatabase& db);
    void deallocateQuery(const QString& queryname, const QSqlDatabase& db);

    void savePoint(const QString& handle, const QSqlDatabase& db) { db.exec(QString("SAVEPOINT %1").arg(handle)); }
    void rollbackSavePoint(const QString& handle, const QSqlDatabase& db) { db.exec(QString("ROLLBACK TO SAVEPOINT %1").arg(handle)); }
    void releaseSavePoint(const QString& handle, const QSqlDatabase& db) { db.exec(QString("RELEASE SAVEPOINT %1").arg(handle)); }

private:
    void bindNetworkInfo(QSqlQuery& query, const NetworkInfo& info);
    void bindServerInfo(QSqlQuery& query, const Network::Server& server);
    QSqlQuery prepareAndExecuteQuery(const QString& queryname, const QString& paramstring, QSqlDatabase& db);
    QSqlQuery prepareAndExecuteQuery(const QString& queryname, QSqlDatabase& db)
    {
        return prepareAndExecuteQuery(queryname, QString(), db);
    }

    QString _hostName;
    int _port{-1};
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

    bool writeMo(const QuasselUserMO& user) override;
    bool writeMo(const SenderMO& sender) override;
    bool writeMo(const IdentityMO& identity) override;
    bool writeMo(const IdentityNickMO& identityNick) override;
    bool writeMo(const NetworkMO& network) override;
    bool writeMo(const BufferMO& buffer) override;
    bool writeMo(const BacklogMO& backlog) override;
    bool writeMo(const IrcServerMO& ircserver) override;
    bool writeMo(const UserSettingMO& userSetting) override;
    bool writeMo(const CoreStateMO& coreState) override;

    bool prepareQuery(MigrationObject mo) override;

    bool postProcess() override;

protected:
    inline bool transaction() override { return logDb().transaction(); }
    inline void rollback() override { logDb().rollback(); }
    inline bool commit() override { return logDb().commit(); }

private:
    // helper struct
    struct Sequence
    {
        QLatin1String table;
        QLatin1String field;
        Sequence(const char* table, const char* field)
            : table(table)
            , field(field)
        {}
    };

    QSet<int> _validIdentities;
};

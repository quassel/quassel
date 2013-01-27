/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef POSTGRESQLSTORAGE_H
#define POSTGRESQLSTORAGE_H

#include "abstractsqlstorage.h"

#include <QSqlDatabase>
#include <QSqlQuery>

class PostgreSqlStorage : public AbstractSqlStorage
{
    Q_OBJECT

public:
    PostgreSqlStorage(QObject *parent = 0);
    virtual ~PostgreSqlStorage();

    virtual AbstractSqlMigrationWriter *createMigrationWriter();

public slots:
    /* General */
    virtual bool isAvailable() const;
    virtual QString displayName() const;
    virtual QString description() const;
    virtual QStringList setupKeys() const;
    virtual QVariantMap setupDefaults() const;

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */

    virtual UserId addUser(const QString &user, const QString &password);
    virtual bool updateUser(UserId user, const QString &password);
    virtual void renameUser(UserId user, const QString &newName);
    virtual UserId validateUser(const QString &user, const QString &password);
    virtual UserId getUserId(const QString &username);
    virtual UserId internalUser();
    virtual void delUser(UserId user);
    virtual void setUserSetting(UserId userId, const QString &settingName, const QVariant &data);
    virtual QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &defaultData = QVariant());

    /* Identity handling */
    virtual IdentityId createIdentity(UserId user, CoreIdentity &identity);
    virtual bool updateIdentity(UserId user, const CoreIdentity &identity);
    virtual void removeIdentity(UserId user, IdentityId identityId);
    virtual QList<CoreIdentity> identities(UserId user);

    /* Network handling */
    virtual NetworkId createNetwork(UserId user, const NetworkInfo &info);
    virtual bool updateNetwork(UserId user, const NetworkInfo &info);
    virtual bool removeNetwork(UserId user, const NetworkId &networkId);
    virtual QList<NetworkInfo> networks(UserId user);
    virtual QList<NetworkId> connectedNetworks(UserId user);
    virtual void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected);

    /* persistent channels */
    virtual QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId);
    virtual void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined);
    virtual void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key);

    /* persistent user states */
    virtual QString awayMessage(UserId user, NetworkId networkId);
    virtual void setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg);
    virtual QString userModes(UserId user, NetworkId networkId);
    virtual void setUserModes(UserId user, NetworkId networkId, const QString &userModes);

    /* Buffer handling */
    virtual BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "", bool create = true);
    virtual BufferInfo getBufferInfo(UserId user, const BufferId &bufferId);
    virtual QList<BufferInfo> requestBuffers(UserId user);
    virtual QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId);
    virtual bool removeBuffer(const UserId &user, const BufferId &bufferId);
    virtual bool renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName);
    virtual bool mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2);
    virtual void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId);
    virtual QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user);
    virtual void setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId);
    virtual QHash<BufferId, MsgId> bufferMarkerLineMsgIds(UserId user);

    /* Message handling */
    virtual bool logMessage(Message &msg);
    virtual bool logMessages(MessageList &msgs);
    virtual QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1);
    virtual QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1);

protected:
    virtual void initDbSession(QSqlDatabase &db);
    virtual void setConnectionProperties(const QVariantMap &properties);
    inline virtual QString driverName() { return "QPSQL"; }
    inline virtual QString hostName() { return _hostName; }
    inline virtual int port() { return _port; }
    inline virtual QString databaseName() { return _databaseName; }
    inline virtual QString userName() { return _userName; }
    inline virtual QString password() { return _password; }
    virtual int installedSchemaVersion();
    virtual bool updateSchemaVersion(int newVersion);
    virtual bool setupSchemaVersion(int version);
    void safeExec(QSqlQuery &query);

    bool beginReadOnlyTransaction(QSqlDatabase &db);

    QSqlQuery executePreparedQuery(const QString &queryname, const QVariantList &params, const QSqlDatabase &db);
    QSqlQuery executePreparedQuery(const QString &queryname, const QVariant &param, const QSqlDatabase &db);
    void deallocateQuery(const QString &queryname, const QSqlDatabase &db);

    inline void savePoint(const QString &handle, const QSqlDatabase &db) { db.exec(QString("SAVEPOINT %1").arg(handle)); }
    inline void rollbackSavePoint(const QString &handle, const QSqlDatabase &db) { db.exec(QString("ROLLBACK TO SAVEPOINT %1").arg(handle)); }
    inline void releaseSavePoint(const QString &handle, const QSqlDatabase &db) { db.exec(QString("RELEASE SAVEPOINT %1").arg(handle)); }

private:
    void bindNetworkInfo(QSqlQuery &query, const NetworkInfo &info);
    void bindServerInfo(QSqlQuery &query, const Network::Server &server);
    QSqlQuery prepareAndExecuteQuery(const QString &queryname, const QString &paramstring, const QSqlDatabase &db);
    inline QSqlQuery prepareAndExecuteQuery(const QString &queryname, const QSqlDatabase &db) { return prepareAndExecuteQuery(queryname, QString(), db); }

    QString _hostName;
    int _port;
    QString _databaseName;
    QString _userName;
    QString _password;
};


inline void PostgreSqlStorage::safeExec(QSqlQuery &query) { query.exec(); }

// ========================================
//  PostgreSqlMigration
// ========================================
class PostgreSqlMigrationWriter : public PostgreSqlStorage, public AbstractSqlMigrationWriter
{
    Q_OBJECT

public:
    PostgreSqlMigrationWriter();

    virtual bool writeMo(const QuasselUserMO &user);
    virtual bool writeMo(const SenderMO &sender);
    virtual bool writeMo(const IdentityMO &identity);
    virtual bool writeMo(const IdentityNickMO &identityNick);
    virtual bool writeMo(const NetworkMO &network);
    virtual bool writeMo(const BufferMO &buffer);
    virtual bool writeMo(const BacklogMO &backlog);
    virtual bool writeMo(const IrcServerMO &ircserver);
    virtual bool writeMo(const UserSettingMO &userSetting);

    bool prepareQuery(MigrationObject mo);

    virtual bool postProcess();

protected:
    virtual inline bool transaction() { return logDb().transaction(); }
    virtual inline void rollback() { logDb().rollback(); }
    virtual inline bool commit() { return logDb().commit(); }

private:
    // helper struct
    struct Sequence {
        QLatin1String table;
        QLatin1String field;
        Sequence(const char *table, const char *field) : table(table), field(field) {}
    };

    QSet<int> _validIdentities;
};


#endif

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

#ifndef SQLITESTORAGE_H
#define SQLITESTORAGE_H

#include "abstractsqlstorage.h"

#include <QSqlDatabase>

class QSqlQuery;

class SqliteStorage : public AbstractSqlStorage
{
    Q_OBJECT

public:
    SqliteStorage(QObject *parent = 0);
    virtual ~SqliteStorage();

    virtual AbstractSqlMigrationReader *createMigrationReader();

public slots:
    /* General */

    bool isAvailable() const;
    QString displayName() const;
    virtual inline QStringList setupKeys() const { return QStringList(); }
    virtual inline QVariantMap setupDefaults() const { return QVariantMap(); }
    QString description() const;

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
    inline virtual void setConnectionProperties(const QVariantMap & /* properties */) {}
    inline virtual QString driverName() { return "QSQLITE"; }
    inline virtual QString databaseName() { return backlogFile(); }
    virtual int installedSchemaVersion();
    virtual bool updateSchemaVersion(int newVersion);
    virtual bool setupSchemaVersion(int version);
    bool safeExec(QSqlQuery &query, int retryCount = 0);

private:
    static QString backlogFile();
    void bindNetworkInfo(QSqlQuery &query, const NetworkInfo &info);
    void bindServerInfo(QSqlQuery &query, const Network::Server &server);

    inline void lockForRead() { _dbLock.lockForRead(); }
    inline void lockForWrite() { _dbLock.lockForWrite(); }
    inline void unlock() { _dbLock.unlock(); }
    QReadWriteLock _dbLock;
    static int _maxRetryCount;
};


// ========================================
//  SqliteMigration
// ========================================
class SqliteMigrationReader : public SqliteStorage, public AbstractSqlMigrationReader
{
    Q_OBJECT

public:
    SqliteMigrationReader();

    virtual bool readMo(QuasselUserMO &user);
    virtual bool readMo(SenderMO &sender);
    virtual bool readMo(IdentityMO &identity);
    virtual bool readMo(IdentityNickMO &identityNick);
    virtual bool readMo(NetworkMO &network);
    virtual bool readMo(BufferMO &buffer);
    virtual bool readMo(BacklogMO &backlog);
    virtual bool readMo(IrcServerMO &ircserver);
    virtual bool readMo(UserSettingMO &userSetting);

    virtual bool prepareQuery(MigrationObject mo);

    inline int stepSize() { return 50000; }

protected:
    virtual inline bool transaction() { return logDb().transaction(); }
    virtual inline void rollback() { logDb().rollback(); }
    virtual inline bool commit() { return logDb().commit(); }

private:
    void setMaxId(MigrationObject mo);
    int _maxId;
};


inline AbstractSqlMigrationReader *SqliteStorage::createMigrationReader()
{
    return new SqliteMigrationReader();
}


#endif

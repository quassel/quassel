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

#include "storage.h"

#include <memory>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class AbstractSqlMigrationReader;
class AbstractSqlMigrationWriter;

class AbstractSqlStorage : public Storage
{
    Q_OBJECT

public:
    AbstractSqlStorage(QObject *parent = 0);
    virtual ~AbstractSqlStorage();

    virtual std::unique_ptr<AbstractSqlMigrationReader> createMigrationReader() { return {}; }
    virtual std::unique_ptr<AbstractSqlMigrationWriter> createMigrationWriter() { return {}; }

public slots:
    virtual State init(const QVariantMap &settings = QVariantMap(),
                       const QProcessEnvironment &environment = {},
                       bool loadFromEnvironment = false);
    virtual bool setup(const QVariantMap &settings = QVariantMap(),
                       const QProcessEnvironment &environment = {},
                       bool loadFromEnvironment = false);

protected:
    inline virtual void sync() {};

    QSqlDatabase logDb();

    /**
     * Fetch an SQL query string by name and optional schema version
     *
     * Loads the named SQL query from the built-in SQL resource collection, returning it as a
     * string.  If a version is specified, it'll be loaded from the schema version-specific folder
     * instead.
     *
     * @see schemaVersion()
     *
     * @param[in] queryName  File name of the SQL query, minus the .sql extension
     * @param[in] version
     * @parblock
     * SQL schema version; if 0, fetches from current version, otherwise loads from the specified
     * schema version instead of the current schema files.
     * @endparblock
     * @return String with the requested SQL query, ready for parameter substitution
     */
    QString queryString(const QString &queryName, int version = 0);

    QStringList setupQueries();

    QStringList upgradeQueries(int ver);
    bool upgradeDb();

    bool watchQuery(QSqlQuery &query);

    int schemaVersion();
    virtual int installedSchemaVersion() { return -1; };
    virtual bool updateSchemaVersion(int newVersion) = 0;
    virtual bool setupSchemaVersion(int version) = 0;

    virtual void setConnectionProperties(const QVariantMap &properties,
                                         const QProcessEnvironment &environment,
                                         bool loadFromEnvironment) = 0;
    virtual QString driverName() = 0;
    inline virtual QString hostName() { return QString(); }
    inline virtual int port() { return -1; }
    virtual QString databaseName() = 0;
    inline virtual QString userName() { return QString(); }
    inline virtual QString password() { return QString(); }

    //! Initialize db specific features on connect
    /** This is called every time a connection to a specific SQL backend is established
     *  the default implementation does nothing.
     *
     *  When reimplementing this method, don't use logDB() inside this function as
     *  this would cause as we're just about to initialize that DB connection.
     */
    inline virtual bool initDbSession(QSqlDatabase & /* db */) { return true; }

private slots:
    void connectionDestroyed();

private:
    void addConnectionToPool();
    void dbConnect(QSqlDatabase &db);

    int _schemaVersion;
    bool _debug;

    static int _nextConnectionId;
    QMutex _connectionPoolMutex;
    // we let a Connection Object manage each actual db connection
    // those objects reside in the thread the connection belongs to
    // which allows us thread safe termination of a connection
    class Connection;
    QHash<QThread *, Connection *> _connectionPool;
};

struct SenderData {
    QString sender;
    QString realname;
    QString avatarurl;

    friend uint qHash(const SenderData &key);
    friend bool operator==(const SenderData &a, const SenderData &b);
};

// ========================================
//  AbstractSqlStorage::Connection
// ========================================
class AbstractSqlStorage::Connection : public QObject
{
    Q_OBJECT

public:
    Connection(const QString &name, QObject *parent = 0);
    ~Connection();

    inline QLatin1String name() const { return QLatin1String(_name); }

private:
    QByteArray _name;
};


// ========================================
//  AbstractSqlMigrator
// ========================================
class AbstractSqlMigrator
{
public:
    // migration objects
    struct QuasselUserMO {
        UserId id;
        QString username;
        QString password;
        int hashversion;
        QString authenticator;
    };

    struct SenderMO {
        qint64 senderId;
        QString sender;
        QString realname;
        QString avatarurl;
        SenderMO() : senderId(0) {}
    };

    struct IdentityMO {
        IdentityId id;
        UserId userid;
        QString identityname;
        QString realname;
        QString awayNick;
        bool awayNickEnabled;
        QString awayReason;
        bool awayReasonEnabled;
        bool autoAwayEnabled;
        int autoAwayTime;
        QString autoAwayReason;
        bool autoAwayReasonEnabled;
        bool detachAwayEnabled;
        QString detachAwayReason;
        bool detachAwayReasonEnabled;
        QString ident;
        QString kickReason;
        QString partReason;
        QString quitReason;
        QByteArray sslCert;
        QByteArray sslKey;
    };

    struct IdentityNickMO {
        int nickid;
        IdentityId identityId;
        QString nick;
    };

    struct NetworkMO {
        NetworkId networkid;
        UserId userid;
        QString networkname;
        IdentityId identityid;
        QString encodingcodec;
        QString decodingcodec;
        QString servercodec;
        bool userandomserver;
        QString perform;
        bool useautoidentify;
        QString autoidentifyservice;
        QString autoidentifypassword;
        bool useautoreconnect;
        int autoreconnectinterval;
        int autoreconnectretries;
        bool unlimitedconnectretries;
        bool rejoinchannels;
        // Custom rate limiting
        bool usecustommessagerate;
        int messagerateburstsize;
        int messageratedelay;
        bool unlimitedmessagerate;
        // ...
        bool connected;
        QString usermode;
        QString awaymessage;
        QString attachperform;
        QString detachperform;
        bool usesasl;
        QString saslaccount;
        QString saslpassword;
    };

    struct BufferMO {
        BufferId bufferid;
        UserId userid;
        int groupid;
        NetworkId networkid;
        QString buffername;
        QString buffercname;
        int buffertype;
        qint64 lastmsgid;
        qint64 lastseenmsgid;
        qint64 markerlinemsgid;
        int bufferactivity;
        int highlightcount;
        QString key;
        bool joined;
        QString cipher;
    };

    struct BacklogMO {
        MsgId messageid;
        QDateTime time; // has to be in UTC!
        BufferId bufferid;
        int type;
        int flags;
        qint64 senderid;
        QString senderprefixes;
        QString message;
    };

    struct IrcServerMO {
        int serverid;
        UserId userid;
        NetworkId networkid;
        QString hostname;
        int port;
        QString password;
        bool ssl;
        bool sslverify;     /// If true, validate SSL certificates
        int sslversion;
        bool useproxy;
        int proxytype;
        QString proxyhost;
        int proxyport;
        QString proxyuser;
        QString proxypass;
    };

    struct UserSettingMO {
        UserId userid;
        QString settingname;
        QByteArray settingvalue;
    };

    struct CoreStateMO {
        QString key;
        QByteArray value;
    };

    enum MigrationObject {
        QuasselUser,
        Sender,
        Identity,
        IdentityNick,
        Network,
        Buffer,
        Backlog,
        IrcServer,
        UserSetting,
        CoreState
    };

    AbstractSqlMigrator();
    virtual ~AbstractSqlMigrator() {}

    static QString migrationObject(MigrationObject moType);

protected:
    void newQuery(const QString &query, QSqlDatabase db);
    virtual void resetQuery();
    virtual bool prepareQuery(MigrationObject mo) = 0;
    bool exec();
    inline bool next() { return _query->next(); }
    inline QVariant value(int index) { return _query->value(index); }
    inline void bindValue(const QString &placeholder, const QVariant &val) { _query->bindValue(placeholder, val); }
    inline void bindValue(int pos, const QVariant &val) { _query->bindValue(pos, val); }

    inline QSqlError lastError() { return _query ? _query->lastError() : QSqlError(); }
    void dumpStatus();
    inline QString executedQuery() { return _query ? _query->executedQuery() : QString(); }
    inline QVariantList boundValues();

    virtual bool transaction() = 0;
    virtual void rollback() = 0;
    virtual bool commit() = 0;

private:
    QSqlQuery *_query;
};


class AbstractSqlMigrationReader : public AbstractSqlMigrator
{
public:
    AbstractSqlMigrationReader();

    virtual bool readMo(QuasselUserMO &user) = 0;
    virtual bool readMo(IdentityMO &identity) = 0;
    virtual bool readMo(IdentityNickMO &identityNick) = 0;
    virtual bool readMo(NetworkMO &network) = 0;
    virtual bool readMo(BufferMO &buffer) = 0;
    virtual bool readMo(SenderMO &sender) = 0;
    virtual bool readMo(BacklogMO &backlog) = 0;
    virtual bool readMo(IrcServerMO &ircserver) = 0;
    virtual bool readMo(UserSettingMO &userSetting) = 0;
    virtual bool readMo(CoreStateMO &coreState) = 0;

    bool migrateTo(AbstractSqlMigrationWriter *writer);

private:
    void abortMigration(const QString &errorMsg = QString());
    bool finalizeMigration();

    template<typename T> bool transferMo(MigrationObject moType, T &mo);

    AbstractSqlMigrationWriter *_writer;
};


class AbstractSqlMigrationWriter : public AbstractSqlMigrator
{
public:
    virtual bool writeMo(const QuasselUserMO &user) = 0;
    virtual bool writeMo(const IdentityMO &identity) = 0;
    virtual bool writeMo(const IdentityNickMO &identityNick) = 0;
    virtual bool writeMo(const NetworkMO &network) = 0;
    virtual bool writeMo(const BufferMO &buffer) = 0;
    virtual bool writeMo(const SenderMO &sender) = 0;
    virtual bool writeMo(const BacklogMO &backlog) = 0;
    virtual bool writeMo(const IrcServerMO &ircserver) = 0;
    virtual bool writeMo(const UserSettingMO &userSetting) = 0;
    virtual bool writeMo(const CoreStateMO &coreState) = 0;

    inline bool migrateFrom(AbstractSqlMigrationReader *reader) { return reader->migrateTo(this); }

    // called after migration process
    virtual inline bool postProcess() { return true; }
    friend class AbstractSqlMigrationReader;
};

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

#include "abstractsqlstorage.h"
#include "quassel.h"

#include "logger.h"

#include <QMutexLocker>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>

int AbstractSqlStorage::_nextConnectionId = 0;
AbstractSqlStorage::AbstractSqlStorage(QObject *parent)
    : Storage(parent),
    _schemaVersion(0)
{
}


AbstractSqlStorage::~AbstractSqlStorage()
{
    // disconnect the connections, so their deletion is no longer interessting for us
    QHash<QThread *, Connection *>::iterator conIter;
    for (conIter = _connectionPool.begin(); conIter != _connectionPool.end(); ++conIter) {
        QSqlDatabase::removeDatabase(conIter.value()->name());
        disconnect(conIter.value(), 0, this, 0);
    }
}


QSqlDatabase AbstractSqlStorage::logDb()
{
    if (!_connectionPool.contains(QThread::currentThread()))
        addConnectionToPool();

    QSqlDatabase db = QSqlDatabase::database(_connectionPool[QThread::currentThread()]->name(),false);

    if (!db.isOpen()) {
        qWarning() << "Database connection" << displayName() << "for thread" << QThread::currentThread() << "was lost, attempting to reconnect...";
        dbConnect(db);
    }

    return db;
}


void AbstractSqlStorage::addConnectionToPool()
{
    QMutexLocker locker(&_connectionPoolMutex);
    // we have to recheck if the connection pool already contains a connection for
    // this thread. Since now (after the lock) we can only tell for sure
    if (_connectionPool.contains(QThread::currentThread()))
        return;

    QThread *currentThread = QThread::currentThread();

    int connectionId = _nextConnectionId++;

    Connection *connection = new Connection(QLatin1String(QString("quassel_%1_con_%2").arg(driverName()).arg(connectionId).toLatin1()));
    connection->moveToThread(currentThread);
    connect(this, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
    connect(currentThread, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
    connect(connection, SIGNAL(destroyed()), this, SLOT(connectionDestroyed()));
    _connectionPool[currentThread] = connection;

    QSqlDatabase db = QSqlDatabase::addDatabase(driverName(), connection->name());
    db.setDatabaseName(databaseName());

    if (!hostName().isEmpty())
        db.setHostName(hostName());

    if (port() != -1)
        db.setPort(port());

    if (!userName().isEmpty()) {
        db.setUserName(userName());
        db.setPassword(password());
    }

    dbConnect(db);
}


void AbstractSqlStorage::dbConnect(QSqlDatabase &db)
{
    if (!db.open()) {
        quWarning() << "Unable to open database" << displayName() << "for thread" << QThread::currentThread();
        quWarning() << "-" << db.lastError().text();
    }
    else {
        if (!initDbSession(db)) {
            quWarning() << "Unable to initialize database" << displayName() << "for thread" << QThread::currentThread();
            db.close();
        }
    }
}


Storage::State AbstractSqlStorage::init(const QVariantMap &settings)
{
    setConnectionProperties(settings);

    _debug = Quassel::isOptionSet("debug");

    QSqlDatabase db = logDb();
    if (!db.isValid() || !db.isOpen())
        return NotAvailable;

    if (installedSchemaVersion() == -1) {
        qCritical() << "Storage Schema is missing!";
        return NeedsSetup;
    }

    if (installedSchemaVersion() > schemaVersion()) {
        qCritical() << "Installed Schema is newer then any known Version.";
        return NotAvailable;
    }

    if (installedSchemaVersion() < schemaVersion()) {
        qWarning() << qPrintable(tr("Installed Schema (version %1) is not up to date. Upgrading to version %2...").arg(installedSchemaVersion()).arg(schemaVersion()));
        if (!upgradeDb()) {
            qWarning() << qPrintable(tr("Upgrade failed..."));
            return NotAvailable;
        }
    }

    quInfo() << qPrintable(displayName()) << "storage backend is ready. Schema version:" << installedSchemaVersion();
    return IsReady;
}


QString AbstractSqlStorage::queryString(const QString &queryName, int version)
{
    QFileInfo queryInfo;

    // The current schema is stored in the root folder, while upgrade queries are stored in the
    // 'versions/##' subfolders.
    if (version == 0) {
        // Use the current SQL schema, not a versioned request
        queryInfo = QFileInfo(QString(":/SQL/%1/%2.sql").arg(displayName()).arg(queryName));
        // If version is needed later, get it via version = schemaVersion();
    } else {
        // Use the specified schema version, not the general folder
        queryInfo = QFileInfo(QString(":/SQL/%1/version/%2/%3.sql")
                              .arg(displayName()).arg(version).arg(queryName));
    }

    if (!queryInfo.exists() || !queryInfo.isFile() || !queryInfo.isReadable()) {
        qCritical() << "Unable to read SQL-Query" << queryName << "for engine" << displayName();
        return QString();
    }

    QFile queryFile(queryInfo.filePath());
    if (!queryFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QString query = QTextStream(&queryFile).readAll();
    queryFile.close();

    return query.trimmed();
}


QStringList AbstractSqlStorage::setupQueries()
{
    QStringList queries;
    // The current schema is stored in the root folder, including setup scripts.
    QDir dir = QDir(QString(":/SQL/%1/").arg(displayName()));
    foreach(QFileInfo fileInfo, dir.entryInfoList(QStringList() << "setup*", QDir::NoFilter, QDir::Name)) {
        queries << queryString(fileInfo.baseName());
    }
    return queries;
}


bool AbstractSqlStorage::setup(const QVariantMap &settings)
{
    setConnectionProperties(settings);
    QSqlDatabase db = logDb();
    if (!db.isOpen()) {
        qCritical() << "Unable to setup Logging Backend!";
        return false;
    }

    db.transaction();
    foreach(QString queryString, setupQueries()) {
        QSqlQuery query = db.exec(queryString);
        if (!watchQuery(query)) {
            qCritical() << "Unable to setup Logging Backend!";
            db.rollback();
            return false;
        }
    }
    bool success = setupSchemaVersion(schemaVersion());
    if (success)
        db.commit();
    else
        db.rollback();
    return success;
}


QStringList AbstractSqlStorage::upgradeQueries(int version)
{
    QStringList queries;
    // Upgrade queries are stored in the 'version/##' subfolders.
    QDir dir = QDir(QString(":/SQL/%1/version/%2/").arg(displayName()).arg(version));
    foreach(QFileInfo fileInfo, dir.entryInfoList(QStringList() << "upgrade*", QDir::NoFilter, QDir::Name)) {
        queries << queryString(fileInfo.baseName(), version);
    }
    return queries;
}


bool AbstractSqlStorage::upgradeDb()
{
    if (schemaVersion() <= installedSchemaVersion())
        return true;

    QSqlDatabase db = logDb();

    for (int ver = installedSchemaVersion() + 1; ver <= schemaVersion(); ver++) {
        foreach(QString queryString, upgradeQueries(ver)) {
            QSqlQuery query = db.exec(queryString);
            if (!watchQuery(query)) {
                qCritical() << "Unable to upgrade Logging Backend!";
                return false;
            }
        }
    }
    return updateSchemaVersion(schemaVersion());
}


int AbstractSqlStorage::schemaVersion()
{
    // returns the newest Schema Version!
    // not the currently used one! (though it can be the same)
    if (_schemaVersion > 0)
        return _schemaVersion;

    int version;
    bool ok;
    // Schema versions are stored in the 'version/##' subfolders.
    QDir dir = QDir(QString(":/SQL/%1/version/").arg(displayName()));
    foreach(QFileInfo fileInfo, dir.entryInfoList()) {
        if (!fileInfo.isDir())
            continue;

        version = fileInfo.fileName().toInt(&ok);
        if (!ok)
            continue;

        if (version > _schemaVersion)
            _schemaVersion = version;
    }
    return _schemaVersion;
}


bool AbstractSqlStorage::watchQuery(QSqlQuery &query)
{
    bool queryError = query.lastError().isValid();
    if (queryError || _debug) {
        if (queryError)
            qCritical() << "unhandled Error in QSqlQuery!";
        qCritical() << "                  last Query:\n" << qPrintable(query.lastQuery());
        qCritical() << "              executed Query:\n" << qPrintable(query.executedQuery());
        QVariantMap boundValues = query.boundValues();
        QStringList valueStrings;
        QVariantMap::const_iterator iter;
        for (iter = boundValues.constBegin(); iter != boundValues.constEnd(); ++iter) {
            QString value;
            QSqlField field;
            if (query.driver()) {
                // let the driver do the formatting
                field.setType(iter.value().type());
                if (iter.value().isNull())
                    field.clear();
                else
                    field.setValue(iter.value());
                value =  query.driver()->formatValue(field);
            }
            else {
                switch (iter.value().type()) {
                case QVariant::Invalid:
                    value = "NULL";
                    break;
                case QVariant::Int:
                    value = iter.value().toString();
                    break;
                default:
                    value = QString("'%1'").arg(iter.value().toString());
                }
            }
            valueStrings << QString("%1=%2").arg(iter.key(), value);
        }
        qCritical() << "                bound Values:" << qPrintable(valueStrings.join(", "));
        qCritical() << "                Error Number:" << query.lastError().number();
        qCritical() << "               Error Message:" << qPrintable(query.lastError().text());
        qCritical() << "              Driver Message:" << qPrintable(query.lastError().driverText());
        qCritical() << "                  DB Message:" << qPrintable(query.lastError().databaseText());

        return !queryError;
    }
    return true;
}


void AbstractSqlStorage::connectionDestroyed()
{
    QMutexLocker locker(&_connectionPoolMutex);
    _connectionPool.remove(sender()->thread());
}


// ========================================
//  AbstractSqlStorage::Connection
// ========================================
AbstractSqlStorage::Connection::Connection(const QString &name, QObject *parent)
    : QObject(parent),
    _name(name.toLatin1())
{
}


AbstractSqlStorage::Connection::~Connection()
{
    {
        QSqlDatabase db = QSqlDatabase::database(name(), false);
        if (db.isOpen()) {
            db.commit();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(name());
}


// ========================================
//  AbstractSqlMigrator
// ========================================
AbstractSqlMigrator::AbstractSqlMigrator()
    : _query(0)
{
}


void AbstractSqlMigrator::newQuery(const QString &query, QSqlDatabase db)
{
    Q_ASSERT(!_query);
    _query = new QSqlQuery(db);
    _query->prepare(query);
}


void AbstractSqlMigrator::resetQuery()
{
    delete _query;
    _query = 0;
}


bool AbstractSqlMigrator::exec()
{
    Q_ASSERT(_query);
    _query->exec();
    return !_query->lastError().isValid();
}


QString AbstractSqlMigrator::migrationObject(MigrationObject moType)
{
    switch (moType) {
    case QuasselUser:
        return "QuasselUser";
    case Sender:
        return "Sender";
    case Identity:
        return "Identity";
    case IdentityNick:
        return "IdentityNick";
    case Network:
        return "Network";
    case Buffer:
        return "Buffer";
    case Backlog:
        return "Backlog";
    case IrcServer:
        return "IrcServer";
    case UserSetting:
        return "UserSetting";
    };
    return QString();
}


QVariantList AbstractSqlMigrator::boundValues()
{
    QVariantList values;
    if (!_query)
        return values;

    int numValues = _query->boundValues().count();
    for (int i = 0; i < numValues; i++) {
        values << _query->boundValue(i);
    }
    return values;
}


void AbstractSqlMigrator::dumpStatus()
{
    qWarning() << "  executed Query:";
    qWarning() << qPrintable(executedQuery());
    qWarning() << "  bound Values:";
    QList<QVariant> list = boundValues();
    for (int i = 0; i < list.size(); ++i)
        qWarning() << i << ": " << list.at(i).toString().toLatin1().data();
    qWarning() << "  Error Number:"   << lastError().number();
    qWarning() << "  Error Message:"   << lastError().text();
}


// ========================================
//  AbstractSqlMigrationReader
// ========================================
AbstractSqlMigrationReader::AbstractSqlMigrationReader()
    : AbstractSqlMigrator(),
    _writer(0)
{
}


bool AbstractSqlMigrationReader::migrateTo(AbstractSqlMigrationWriter *writer)
{
    if (!transaction()) {
        qWarning() << "AbstractSqlMigrationReader::migrateTo(): unable to start reader's transaction!";
        return false;
    }
    if (!writer->transaction()) {
        qWarning() << "AbstractSqlMigrationReader::migrateTo(): unable to start writer's transaction!";
        rollback(); // close the reader transaction;
        return false;
    }

    _writer = writer;

    // due to the incompatibility across Migration objects we can't run this in a loop... :/
    QuasselUserMO quasselUserMo;
    if (!transferMo(QuasselUser, quasselUserMo))
        return false;

    IdentityMO identityMo;
    if (!transferMo(Identity, identityMo))
        return false;

    IdentityNickMO identityNickMo;
    if (!transferMo(IdentityNick, identityNickMo))
        return false;

    NetworkMO networkMo;
    if (!transferMo(Network, networkMo))
        return false;

    BufferMO bufferMo;
    if (!transferMo(Buffer, bufferMo))
        return false;

    SenderMO senderMo;
    if (!transferMo(Sender, senderMo))
        return false;

    BacklogMO backlogMo;
    if (!transferMo(Backlog, backlogMo))
        return false;

    IrcServerMO ircServerMo;
    if (!transferMo(IrcServer, ircServerMo))
        return false;

    UserSettingMO userSettingMo;
    if (!transferMo(UserSetting, userSettingMo))
        return false;

    if (!_writer->postProcess())
        abortMigration();
    return finalizeMigration();
}


void AbstractSqlMigrationReader::abortMigration(const QString &errorMsg)
{
    qWarning() << "Migration Failed!";
    if (!errorMsg.isNull()) {
        qWarning() << qPrintable(errorMsg);
    }
    if (lastError().isValid()) {
        qWarning() << "ReaderError:";
        dumpStatus();
    }

    if (_writer->lastError().isValid()) {
        qWarning() << "WriterError:";
        _writer->dumpStatus();
    }

    rollback();
    _writer->rollback();
    _writer = 0;
}


bool AbstractSqlMigrationReader::finalizeMigration()
{
    resetQuery();
    _writer->resetQuery();

    commit();
    if (!_writer->commit()) {
        _writer = 0;
        return false;
    }
    _writer = 0;
    return true;
}


template<typename T>
bool AbstractSqlMigrationReader::transferMo(MigrationObject moType, T &mo)
{
    resetQuery();
    _writer->resetQuery();

    if (!prepareQuery(moType)) {
        abortMigration(QString("AbstractSqlMigrationReader::migrateTo(): unable to prepare reader query of type %1!").arg(AbstractSqlMigrator::migrationObject(moType)));
        return false;
    }
    if (!_writer->prepareQuery(moType)) {
        abortMigration(QString("AbstractSqlMigrationReader::migrateTo(): unable to prepare writer query of type %1!").arg(AbstractSqlMigrator::migrationObject(moType)));
        return false;
    }

    qDebug() << qPrintable(QString("Transferring %1...").arg(AbstractSqlMigrator::migrationObject(moType)));
    int i = 0;
    QFile file;
    file.open(stdout, QIODevice::WriteOnly);

    while (readMo(mo)) {
        if (!_writer->writeMo(mo)) {
            abortMigration(QString("AbstractSqlMigrationReader::transferMo(): unable to transfer Migratable Object of type %1!").arg(AbstractSqlMigrator::migrationObject(moType)));
            return false;
        }
        i++;
        if (i % 1000 == 0) {
            file.write("*");
            file.flush();
        }
    }
    if (i > 1000) {
        file.write("\n");
        file.flush();
    }

    qDebug() << "Done.";
    return true;
}

uint qHash(const SenderData &key) {
    return qHash(QString(key.sender + "\n" + key.realname + "\n" + key.avatarurl));
}

bool operator==(const SenderData &a, const SenderData &b) {
    return a.sender == b.sender &&
        a.realname == b.realname &&
        a.avatarurl == b.avatarurl;
}

/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "abstractsqlstorage.h"

#include "logger.h"

#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>

AbstractSqlStorage::AbstractSqlStorage(QObject *parent)
  : Storage(parent),
    _schemaVersion(0),
    _nextConnectionId(0)
{
}

AbstractSqlStorage::~AbstractSqlStorage() {
  // disconnect the connections, so their deletion is no longer interessting for us
  QHash<QThread *, Connection *>::iterator conIter;
  for(conIter = _connectionPool.begin(); conIter != _connectionPool.end(); conIter++) {
    disconnect(conIter.value(), 0, this, 0);
  }
}

QSqlDatabase AbstractSqlStorage::logDb() {
  if(!_connectionPool.contains(QThread::currentThread()))
    addConnectionToPool();

  return QSqlDatabase::database(_connectionPool[QThread::currentThread()]->name());
}

void AbstractSqlStorage::addConnectionToPool() {
  QMutexLocker locker(&_connectionPoolMutex);
  // we have to recheck if the connection pool already contains a connection for
  // this thread. Since now (after the lock) we can only tell for sure
  if(_connectionPool.contains(QThread::currentThread()))
    return;

  QThread *currentThread = QThread::currentThread();

  int connectionId = _nextConnectionId++;

  Connection *connection = new Connection(QLatin1String(QString("quassel_connection_%1").arg(connectionId).toLatin1()));
  connection->moveToThread(currentThread);
  connect(this, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
  connect(currentThread, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
  connect(connection, SIGNAL(destroyed()), this, SLOT(connectionDestroyed()));
  _connectionPool[currentThread] = connection;

  QSqlDatabase db = QSqlDatabase::addDatabase(driverName(), connection->name());
  db.setDatabaseName(databaseName());

  if(!hostName().isEmpty())
    db.setHostName(hostName());

  if(!userName().isEmpty()) {
    db.setUserName(userName());
    db.setPassword(password());
  }

  if(!db.open()) {
    qWarning() << "Unable to open database" << displayName() << "for thread" << QThread::currentThread();
    qWarning() << "-" << db.lastError().text();
  }
}

bool AbstractSqlStorage::init(const QVariantMap &settings) {
  Q_UNUSED(settings)
  QSqlDatabase db = logDb();
  if(!db.isValid() || !db.isOpen())
    return false;

  if(installedSchemaVersion() == -1) {
    qCritical() << "Storage Schema is missing!";
    return false;
  }

  if(installedSchemaVersion() > schemaVersion()) {
    qCritical() << "Installed Schema is newer then any known Version.";
    return false;
  }
  
  if(installedSchemaVersion() < schemaVersion()) {
    qWarning() << "Installed Schema is not up to date. Upgrading...";
    if(!upgradeDb())
      return false;
  }
  
  quInfo() << "Storage Backend is ready. Quassel Schema Version:" << installedSchemaVersion();
  return true;
}

QString AbstractSqlStorage::queryString(const QString &queryName, int version) {
  if(version == 0)
    version = schemaVersion();
    
  QFileInfo queryInfo(QString(":/SQL/%1/%2/%3.sql").arg(displayName()).arg(version).arg(queryName));
  if(!queryInfo.exists() || !queryInfo.isFile() || !queryInfo.isReadable()) {
    qCritical() << "Unable to read SQL-Query" << queryName << "for engine" << displayName();
    return QString();
  }

  QFile queryFile(queryInfo.filePath());
  if(!queryFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();
  QString query = QTextStream(&queryFile).readAll();
  queryFile.close();
  
  return query.trimmed();
}

QStringList AbstractSqlStorage::setupQueries() {
  QStringList queries;
  QDir dir = QDir(QString(":/SQL/%1/%2/").arg(displayName()).arg(schemaVersion()));
  foreach(QFileInfo fileInfo, dir.entryInfoList(QStringList() << "setup*", QDir::NoFilter, QDir::Name)) {
    queries << queryString(fileInfo.baseName());
  }
  return queries;
}

bool AbstractSqlStorage::setup(const QVariantMap &settings) {
  Q_UNUSED(settings)
  QSqlDatabase db = logDb();
  if(!db.isOpen()) {
    qCritical() << "Unable to setup Logging Backend!";
    return false;
  }

  foreach(QString queryString, setupQueries()) {
    QSqlQuery query = db.exec(queryString);
    if(!watchQuery(query)) {
      qCritical() << "Unable to setup Logging Backend!";
      return false;
    }
  }
  return true;
}

QStringList AbstractSqlStorage::upgradeQueries(int version) {
  QStringList queries;
  QDir dir = QDir(QString(":/SQL/%1/%2/").arg(displayName()).arg(version));
  foreach(QFileInfo fileInfo, dir.entryInfoList(QStringList() << "upgrade*", QDir::NoFilter, QDir::Name)) {
    queries << queryString(fileInfo.baseName(), version);
  }
  return queries;
}

bool AbstractSqlStorage::upgradeDb() {
  if(schemaVersion() <= installedSchemaVersion())
    return true;

  QSqlDatabase db = logDb();

  for(int ver = installedSchemaVersion() + 1; ver <= schemaVersion(); ver++) {
    foreach(QString queryString, upgradeQueries(ver)) {
      QSqlQuery query = db.exec(queryString);
      if(!watchQuery(query)) {
	qCritical() << "Unable to upgrade Logging Backend!";
	return false;
      }
    }
  }
  return true;
}


int AbstractSqlStorage::schemaVersion() {
  // returns the newest Schema Version!
  // not the currently used one! (though it can be the same)
  if(_schemaVersion > 0)
    return _schemaVersion;

  int version;
  bool ok;
  QDir dir = QDir(":/SQL/" + displayName());
  foreach(QFileInfo fileInfo, dir.entryInfoList()) {
    if(!fileInfo.isDir())
      continue;

    version = fileInfo.fileName().toInt(&ok);
    if(!ok)
      continue;

    if(version > _schemaVersion)
      _schemaVersion = version;
  }
  return _schemaVersion;
}

bool AbstractSqlStorage::watchQuery(QSqlQuery &query) {
  if(query.lastError().isValid()) {
    qCritical() << "unhandled Error in QSqlQuery!";
    qCritical() << "                  last Query:\n" << query.lastQuery();
    qCritical() << "              executed Query:\n" << query.executedQuery();
    qCritical() << "                bound Values:";
    QList<QVariant> list = query.boundValues().values();
    for (int i = 0; i < list.size(); ++i)
      qCritical() << i << ": " << list.at(i).toString().toAscii().data();
    qCritical() << "                Error Number:"   << query.lastError().number();
    qCritical() << "               Error Message:"   << query.lastError().text();
    qCritical() << "              Driver Message:"   << query.lastError().driverText();
    qCritical() << "                  DB Message:"   << query.lastError().databaseText();
    
    return false;
  }
  return true;
}

void AbstractSqlStorage::connectionDestroyed() {
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

AbstractSqlStorage::Connection::~Connection() {
  {
    QSqlDatabase db = QSqlDatabase::database(name(), false);
    if(db.isOpen()) {
      db.commit();
      db.close();
    }
  }
  QSqlDatabase::removeDatabase(name());
}

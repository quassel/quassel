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

#include <QSqlError>
#include <QSqlQuery>

AbstractSqlStorage::AbstractSqlStorage(QObject *parent)
  : Storage(parent),
    _schemaVersion(0)
{
}

AbstractSqlStorage::~AbstractSqlStorage() {
  QHash<QPair<QString, int>, QSqlQuery *>::iterator iter = _queryCache.begin();
  while(iter != _queryCache.end()) {
    delete *iter;
    iter = _queryCache.erase(iter);
  }
  
  {
    QSqlDatabase db = QSqlDatabase::database("quassel_connection");
    db.commit();
    db.close();
  }
  QSqlDatabase::removeDatabase("quassel_connection");  
}

QSqlDatabase AbstractSqlStorage::logDb() {
  QSqlDatabase db = QSqlDatabase::database("quassel_connection");
  if(db.isValid() && db.isOpen())
    return db;

  if(!openDb()) {
    quWarning() << "Unable to Open Database" << displayName();
    quWarning() << "-" << db.lastError().text();
  }

  return QSqlDatabase::database("quassel_connection");
}

bool AbstractSqlStorage::openDb() {
  QSqlDatabase db = QSqlDatabase::database("quassel_connection");
  if(db.isValid() && !db.isOpen())
    return db.open();

  db = QSqlDatabase::addDatabase(driverName(), "quassel_connection");
  db.setDatabaseName(databaseName());

  if(!hostName().isEmpty())
    db.setHostName(hostName());

  if(!userName().isEmpty()) {
    db.setUserName(userName());
    db.setPassword(password());
  }

  return db.open();
}

bool AbstractSqlStorage::init(const QVariantMap &settings) {
  Q_UNUSED(settings)
  QSqlDatabase db = logDb();
  if(!db.isValid() || !db.isOpen())
    return false;

  if(installedSchemaVersion() == -1) {
    quError() << "Storage Schema is missing!";
    return false;
  }

  if(installedSchemaVersion() > schemaVersion()) {
    quError() << "Installed Schema is newer then any known Version.";
    return false;
  }
  
  if(installedSchemaVersion() < schemaVersion()) {
    quWarning() << "Installed Schema is not up to date. Upgrading...";
    if(!upgradeDb())
      return false;
  }
  
  quInfo() << "Storage Backend is ready. Quassel Schema Version:" << installedSchemaVersion();
  return true;
}

void AbstractSqlStorage::sync() {
  QHash<QPair<QString, int>, QSqlQuery *>::iterator iter = _queryCache.begin();
  while(iter != _queryCache.end()) {
    delete *iter;
    iter = _queryCache.erase(iter);
  }

  logDb().commit();
}

QString AbstractSqlStorage::queryString(const QString &queryName, int version) {
  if(version == 0)
    version = schemaVersion();
    
  QFileInfo queryInfo(QString(":/SQL/%1/%2/%3.sql").arg(displayName()).arg(version).arg(queryName));
  if(!queryInfo.exists() || !queryInfo.isFile() || !queryInfo.isReadable()) {
    quError() << "Unable to read SQL-Query" << queryName << "for engine" << displayName();
    return QString();
  }

  QFile queryFile(queryInfo.filePath());
  if(!queryFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();
  QString query = QTextStream(&queryFile).readAll();
  queryFile.close();
  
  return query.trimmed();
}

QString AbstractSqlStorage::queryString(const QString &queryName) {
  return queryString(queryName, 0);
}

QSqlQuery *AbstractSqlStorage::cachedQuery(const QString &queryName, int version) {
  QPair<QString, int> queryId = qMakePair(queryName, version);
  if(!_queryCache.contains(queryId)) {
    QSqlQuery *query = new QSqlQuery(logDb());
    query->prepare(queryString(queryName, version));
    _queryCache[queryId] = query;
  }
  return _queryCache[queryId];
}

QSqlQuery *AbstractSqlStorage::cachedQuery(const QString &queryName) {
  return cachedQuery(queryName, 0);
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
    quError() << "Unable to setup Logging Backend!";
    return false;
  }

  foreach(QString queryString, setupQueries()) {
    QSqlQuery query = db.exec(queryString);
    if(!watchQuery(&query)) {
      quError() << "Unable to setup Logging Backend!";
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
      if(!watchQuery(&query)) {
	quError() << "Unable to upgrade Logging Backend!";
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

bool AbstractSqlStorage::watchQuery(QSqlQuery *query) {
  if(query->lastError().isValid()) {
    quError() << "unhandled Error in QSqlQuery!";
    quError() << "                  last Query:\n" << query->lastQuery();
    quError() << "              executed Query:\n" << query->executedQuery();
    quError() << "                bound Values:";
    QList<QVariant> list = query->boundValues().values();
    for (int i = 0; i < list.size(); ++i)
      quError() << i << ": " << list.at(i).toString().toAscii().data();
    quError() << "                Error Number:"   << query->lastError().number();
    quError() << "               Error Message:"   << query->lastError().text();
    quError() << "              Driver Message:"   << query->lastError().driverText();
    quError() << "                  DB Message:"   << query->lastError().databaseText();
    
    return false;
  }
  return true;
}

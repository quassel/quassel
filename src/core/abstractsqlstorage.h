/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel Project                          *
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

#ifndef ABSTRACTSQLSTORAGE_H
#define ABSTRACTSQLSTORAGE_H

#include "storage.h"

#include <QSqlDatabase>

class QSqlQuery;

class AbstractSqlStorage : public Storage {
  Q_OBJECT

public:
  AbstractSqlStorage(QObject *parent = 0);
  virtual ~AbstractSqlStorage();

protected:
  virtual bool init(const QVariantMap &settings = QVariantMap());
  inline virtual void sync() {};
  
  QSqlDatabase logDb();
  
  QString queryString(const QString &queryName, int version);
  inline QString queryString(const QString &queryName) { return queryString(queryName, 0); }

  QStringList setupQueries();
  bool setup(const QVariantMap &settings = QVariantMap());

  QStringList upgradeQueries(int ver);
  bool upgradeDb();

  bool watchQuery(QSqlQuery &query);
  
  int schemaVersion();
  virtual int installedSchemaVersion() { return -1; };

  virtual QString driverName() = 0;
  inline virtual QString hostName() { return QString(); }
  virtual QString databaseName() = 0;
  inline virtual QString userName() { return QString(); }
  inline virtual QString password() { return QString(); }

private slots:
  void connectionDestroyed();

private:
  void addConnectionToPool();

  int _schemaVersion;

  int _nextConnectionId;
  QMutex _connectionPoolMutex;
  // we let a Connection Object manage each actual db connection
  // those objects reside in the thread the connection belongs to
  // which allows us thread safe termination of a connection
  class Connection;
  QHash<QThread *, Connection *> _connectionPool;
};

// ========================================
//  AbstractSqlStorage::Connection
// ========================================
class AbstractSqlStorage::Connection : public QObject {
  Q_OBJECT

public:
  Connection(const QString &name, QObject *parent = 0);
  ~Connection();
  
  inline QLatin1String name() const { return QLatin1String(_name); }

private:
  QByteArray _name;
};

#endif

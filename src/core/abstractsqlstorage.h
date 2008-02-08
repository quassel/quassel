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
  bool init(const QVariantMap &settings = QVariantMap());
  virtual void sync();
  
  QSqlDatabase logDb();
  
  QString queryString(const QString &queryName, int version);
  QString queryString(const QString &queryName);

  QSqlQuery *cachedQuery(const QString &queryName, int version);
  QSqlQuery *cachedQuery(const QString &queryName);

  QStringList setupQueries();
  bool setup(const QVariantMap &settings = QVariantMap());

  QStringList upgradeQueries(int ver);
  bool upgradeDb();

  bool watchQuery(QSqlQuery *query);
  
  int schemaVersion();
  virtual int installedSchemaVersion() { return -1; };

  virtual QString driverName() = 0;
  inline virtual QString hostName() { return QString(); }
  virtual QString databaseName() = 0;
  inline virtual QString userName() { return QString(); }
  inline virtual QString password() { return QString(); }

private:
  bool openDb();

  int _schemaVersion;

  QHash<QPair<QString, int>, QSqlQuery *> _queryCache;

};


#endif

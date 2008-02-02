/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef SQLITESTORAGE_H
#define SQLITESTORAGE_H

#include "abstractsqlstorage.h"

#include <QSqlDatabase>

class QSqlQuery;

class SqliteStorage : public AbstractSqlStorage {
  Q_OBJECT

public:
  SqliteStorage(QObject *parent = 0);
  virtual ~SqliteStorage();
			  
public slots:
  /* General */
  
  static bool isAvailable();
  static QString displayName();
  virtual QString engineName() ;
  // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc
  
  /* User handling */
  
  virtual UserId addUser(const QString &user, const QString &password);
  virtual void updateUser(UserId user, const QString &password);
  virtual void renameUser(UserId user, const QString &newName);
  virtual UserId validateUser(const QString &user, const QString &password);
  virtual void delUser(UserId user);
  
  /* Network handling */
  virtual NetworkId createNetworkId(UserId user, const NetworkInfo &info);
  
  /* Buffer handling */
  virtual BufferInfo getBufferInfo(UserId user, const NetworkId &networkId, const QString &buffer = "");
  virtual QList<BufferInfo> requestBuffers(UserId user, QDateTime since = QDateTime());
  
  /* Message handling */
  
  virtual MsgId logMessage(Message msg);
  virtual QList<Message> requestMsgs(BufferInfo buffer, int lastmsgs = -1, int offset = -1);
  virtual QList<Message> requestMsgs(BufferInfo buffer, QDateTime since, int offset = -1);
  virtual QList<Message> requestMsgRange(BufferInfo buffer, int first, int last);

protected:
  inline virtual QString driverName() { return "QSQLITE"; }
  inline virtual QString databaseName() { return backlogFile(); }
  virtual int installedSchemaVersion();
  
private:
  static QString backlogFile();
  NetworkId getNetworkId(UserId user, const QString &network);
  void createBuffer(UserId user, const NetworkId &networkId, const QString &buffer);
};

#endif

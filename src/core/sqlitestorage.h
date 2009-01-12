/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

  bool isAvailable() const;
  QString displayName() const;
  QString description() const;

  // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

  /* User handling */

  virtual UserId addUser(const QString &user, const QString &password);
  virtual void updateUser(UserId user, const QString &password);
  virtual void renameUser(UserId user, const QString &newName);
  virtual UserId validateUser(const QString &user, const QString &password);
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

  /* Message handling */

  virtual MsgId logMessage(Message msg);
  virtual QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1);
  virtual QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1);

protected:
  inline virtual QString driverName() { return "QSQLITE"; }
  inline virtual QString databaseName() { return backlogFile(); }
  virtual int installedSchemaVersion();
  bool safeExec(QSqlQuery &query, int retryCount = 0);

private:
  static QString backlogFile();
  bool isValidNetwork(UserId user, const NetworkId &networkId);
  bool isValidBuffer(const UserId &user, const BufferId &bufferId);
  NetworkId getNetworkId(UserId user, const QString &network);
  void createBuffer(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer);

  static int _maxRetryCount;
};

#endif

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

#ifndef POSTGRESQLSTORAGE_H
#define POSTGRESQLSTORAGE_H

#include "abstractsqlstorage.h"

#include <QSqlDatabase>
#include <QSqlQuery>

class PostgreSqlStorage : public AbstractSqlStorage {
  Q_OBJECT

public:
  PostgreSqlStorage(QObject *parent = 0);
  virtual ~PostgreSqlStorage();

public slots:
  /* General */

  bool isAvailable() const;
  QString displayName() const;
  QString description() const;
  QVariantMap setupKeys() const;
  
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

  /* Message handling */

  virtual MsgId logMessage(Message msg);
  virtual QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1);
  virtual QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1);

protected:
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

private:
  void bindNetworkInfo(QSqlQuery &query, const NetworkInfo &info);
  void bindServerInfo(QSqlQuery &query, const Network::Server &server);

  QString _hostName;
  int _port;
  QString _databaseName;
  QString _userName;
  QString _password;
  
  static int _maxRetryCount;
};

inline void PostgreSqlStorage::safeExec(QSqlQuery &query) { query.exec(); }

#endif

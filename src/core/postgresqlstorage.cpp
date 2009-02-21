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

#include "postgresqlstorage.h"

#include <QtSql>

#include "logger.h"
#include "network.h"
#include "quassel.h"

int PostgreSqlStorage::_maxRetryCount = 150; // yes this is a large number... only other way to "handle" this is bailing out...

PostgreSqlStorage::PostgreSqlStorage(QObject *parent)
  : AbstractSqlStorage(parent),
    _port(-1)
{
}

PostgreSqlStorage::~PostgreSqlStorage() {
}

bool PostgreSqlStorage::isAvailable() const {
  if(!QSqlDatabase::isDriverAvailable("QPSQL")) return false;
  return true;
}

QString PostgreSqlStorage::displayName() const {
  return QString("PostgreSQL");
}

QString PostgreSqlStorage::description() const {
  // FIXME: proper description
  return tr("PostgreSQL Turbo Bomber HD!");
}

QVariantMap PostgreSqlStorage::setupKeys() const {
  QVariantMap map;
  map["Username"] = QVariant(QString("quassel"));
  map["Password"] = QVariant(QString());
  map["Hostname"] = QVariant(QString("localhost"));
  map["Port"] = QVariant(5432);
  map["Database"] = QVariant(QString("quassel"));
  return map;
}

void PostgreSqlStorage::setConnectionProperties(const QVariantMap &properties) {
  _userName = properties["Username"].toString();
  _password = properties["Password"].toString();
  _hostName = properties["Hostname"].toString();
  _port = properties["Port"].toInt();
  _databaseName = properties["Database"].toString();
}

int PostgreSqlStorage::installedSchemaVersion() {
  QSqlQuery query = logDb().exec("SELECT value FROM coreinfo WHERE key = 'schemaversion'");
  if(query.first())
    return query.value(0).toInt();

  // maybe it's really old... (schema version 0)
  query = logDb().exec("SELECT MAX(version) FROM coreinfo");
  if(query.first())
    return query.value(0).toInt();

  return AbstractSqlStorage::installedSchemaVersion();
}

bool PostgreSqlStorage::updateSchemaVersion(int newVersion) {
  QSqlQuery query(logDb());
  query.prepare("UPDATE coreinfo SET value = :version WHERE key = 'schemaversion'");
  query.bindValue(":version", newVersion);
  query.exec();

  bool success = true;
  if(query.lastError().isValid()) {
    qCritical() << "PostgreSqlStorage::updateSchemaVersion(int): Updating schema version failed!";
    success = false;
  }
  return success;
}

bool PostgreSqlStorage::setupSchemaVersion(int version) {
  QSqlQuery query(logDb());
  query.prepare("INSERT INTO coreinfo (key, value) VALUES ('schemaversion', :version)");
  query.bindValue(":version", version);
  query.exec();

  bool success = true;
  if(query.lastError().isValid()) {
    qCritical() << "PostgreSqlStorage::setupSchemaVersion(int): Updating schema version failed!";
    success = false;
  }
  return success;
}

UserId PostgreSqlStorage::addUser(const QString &user, const QString &password) {
  QSqlQuery query(logDb());
  query.prepare(queryString("insert_quasseluser"));
  query.bindValue(":username", user);
  query.bindValue(":password", cryptedPassword(password));
  safeExec(query);
  if(!watchQuery(query))
    return 0;

  query.first();
  UserId uid = query.value(0).toInt();
  emit userAdded(uid, user);
  return uid;
}

void PostgreSqlStorage::updateUser(UserId user, const QString &password) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_userpassword"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":password", cryptedPassword(password));
  safeExec(query);
}

void PostgreSqlStorage::renameUser(UserId user, const QString &newName) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_username"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":username", newName);
  safeExec(query);
  emit userRenamed(user, newName);
}

UserId PostgreSqlStorage::validateUser(const QString &user, const QString &password) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_authuser"));
  query.bindValue(":username", user);
  query.bindValue(":password", cryptedPassword(password));
  safeExec(query);

  if(query.first()) {
    return query.value(0).toInt();
  } else {
    return 0;
  }
}

UserId PostgreSqlStorage::internalUser() {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_internaluser"));
  safeExec(query);

  if(query.first()) {
    return query.value(0).toInt();
  } else {
    return 0;
  }
}

void PostgreSqlStorage::delUser(UserId user) {
  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::delUser(): cannot start transaction!";
    return;
  }

  QSqlQuery query(db);
  query.prepare(queryString("delete_quasseluser"));
  query.bindValue(":userid", user.toInt());
  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
    return;
  } else {
    db.commit();
    emit userRemoved(user);
  }
}

void PostgreSqlStorage::setUserSetting(UserId userId, const QString &settingName, const QVariant &data) {
  QByteArray rawData;
  QDataStream out(&rawData, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_2);
  out << data;

  QSqlDatabase db = logDb();  
  QSqlQuery query(db);
  query.prepare(queryString("insert_user_setting"));
  query.bindValue(":userid", userId.toInt());
  query.bindValue(":settingname", settingName);
  query.bindValue(":settingvalue", rawData);
  safeExec(query);

  if(query.lastError().isValid()) {
    QSqlQuery updateQuery(db);
    updateQuery.prepare(queryString("update_user_setting"));
    updateQuery.bindValue(":userid", userId.toInt());
    updateQuery.bindValue(":settingname", settingName);
    updateQuery.bindValue(":settingvalue", rawData);
    safeExec(updateQuery);
  }

}

QVariant PostgreSqlStorage::getUserSetting(UserId userId, const QString &settingName, const QVariant &defaultData) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_user_setting"));
  query.bindValue(":userid", userId.toInt());
  query.bindValue(":settingname", settingName);
  safeExec(query);

  if(query.first()) {
    QVariant data;
    QByteArray rawData = query.value(0).toByteArray();
    QDataStream in(&rawData, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_4_2);
    in >> data;
    return data;
  } else {
    return defaultData;
  }
}

IdentityId PostgreSqlStorage::createIdentity(UserId user, CoreIdentity &identity) {
  IdentityId identityId;

  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::createIdentity(): Unable to start Transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return identityId;
  }

  QSqlQuery query(db);
  query.prepare(queryString("insert_identity"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":identityname", identity.identityName());
  query.bindValue(":realname", identity.realName());
  query.bindValue(":awaynick", identity.awayNick());
  query.bindValue(":awaynickenabled", identity.awayNickEnabled());
  query.bindValue(":awayreason", identity.awayReason());
  query.bindValue(":awayreasonenabled", identity.awayReasonEnabled());
  query.bindValue(":autoawayenabled", identity.awayReasonEnabled());
  query.bindValue(":autoawaytime", identity.autoAwayTime());
  query.bindValue(":autoawayreason", identity.autoAwayReason());
  query.bindValue(":autoawayreasonenabled", identity.autoAwayReasonEnabled());
  query.bindValue(":detachawayenabled", identity.detachAwayEnabled());
  query.bindValue(":detachawayreason", identity.detachAwayReason());
  query.bindValue(":detachawayreasonenabled", identity.detachAwayReasonEnabled());
  query.bindValue(":ident", identity.ident());
  query.bindValue(":kickreason", identity.kickReason());
  query.bindValue(":partreason", identity.partReason());
  query.bindValue(":quitreason", identity.quitReason());
#ifdef HAVE_SSL
  query.bindValue(":sslcert", identity.sslCert().toPem());
  query.bindValue(":sslkey", identity.sslKey().toPem());
#else
  query.bindValue(":sslcert", QByteArray());
  query.bindValue(":sslkey", QByteArray());
#endif
  safeExec(query);
  if(query.lastError().isValid()) {
    watchQuery(query);
    db.rollback();
    return IdentityId();
  }

  qDebug() << "creatId" << query.first() << query.value(0).toInt();
  identityId = query.value(0).toInt();
  identity.setId(identityId);

  if(!identityId.isValid()) {
    watchQuery(query);
    db.rollback();
    return IdentityId();
  }

  QSqlQuery insertNickQuery(db);
  insertNickQuery.prepare(queryString("insert_nick"));
  foreach(QString nick, identity.nicks()) {
    insertNickQuery.bindValue(":identityid", identityId.toInt());
    insertNickQuery.bindValue(":nick", nick);
    safeExec(insertNickQuery);
    if(!watchQuery(insertNickQuery)) {
      db.rollback();
      return IdentityId();
    }
  }

  if(!db.commit()) {
    qWarning() << "PostgreSqlStorage::createIdentity(): commiting data failed!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return IdentityId();
  }
  return identityId;
}

bool PostgreSqlStorage::updateIdentity(UserId user, const CoreIdentity &identity) {
  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::updateIdentity(): Unable to start Transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }
  
  QSqlQuery checkQuery(db);
  checkQuery.prepare(queryString("select_checkidentity"));
  checkQuery.bindValue(":identityid", identity.id().toInt());
  checkQuery.bindValue(":userid", user.toInt());
  safeExec(checkQuery);

  // there should be exactly one identity for the given id and user
  if(!checkQuery.first() || checkQuery.value(0).toInt() != 1) {
    db.rollback();
    return false;
  }

  QSqlQuery query(db);
  query.prepare(queryString("update_identity"));
  query.bindValue(":identityname", identity.identityName());
  query.bindValue(":realname", identity.realName());
  query.bindValue(":awaynick", identity.awayNick());
  query.bindValue(":awaynickenabled", identity.awayNickEnabled());
  query.bindValue(":awayreason", identity.awayReason());
  query.bindValue(":awayreasonenabled", identity.awayReasonEnabled());
  query.bindValue(":autoawayenabled", identity.awayReasonEnabled());
  query.bindValue(":autoawaytime", identity.autoAwayTime());
  query.bindValue(":autoawayreason", identity.autoAwayReason());
  query.bindValue(":autoawayreasonenabled", identity.autoAwayReasonEnabled());
  query.bindValue(":detachawayenabled", identity.detachAwayEnabled());
  query.bindValue(":detachawayreason", identity.detachAwayReason());
  query.bindValue(":detachawayreasonenabled", identity.detachAwayReasonEnabled());
  query.bindValue(":ident", identity.ident());
  query.bindValue(":kickreason", identity.kickReason());
  query.bindValue(":partreason", identity.partReason());
  query.bindValue(":quitreason", identity.quitReason());
#ifdef HAVE_SSL
  query.bindValue(":sslcert", identity.sslCert().toPem());
  query.bindValue(":sslkey", identity.sslKey().toPem());
#else
  query.bindValue(":sslcert", QByteArray());
  query.bindValue(":sslkey", QByteArray());
#endif
  query.bindValue(":identityid", identity.id().toInt());

  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
    return false;
  }
  
  QSqlQuery deleteNickQuery(db);
  deleteNickQuery.prepare(queryString("delete_nicks"));
  deleteNickQuery.bindValue(":identityid", identity.id().toInt());
  safeExec(deleteNickQuery);
  if(!watchQuery(deleteNickQuery)) {
    db.rollback();
    return false;
  }

  QSqlQuery insertNickQuery(db);
  insertNickQuery.prepare(queryString("insert_nick"));
  foreach(QString nick, identity.nicks()) {
    insertNickQuery.bindValue(":identityid", identity.id().toInt());
    insertNickQuery.bindValue(":nick", nick);
    safeExec(insertNickQuery);
    if(!watchQuery(insertNickQuery)) {
      db.rollback();
      return false;
    }
  }

  if(!db.commit()) {
    qWarning() << "PostgreSqlStorage::updateIdentity(): commiting data failed!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }
  return true;
}

void PostgreSqlStorage::removeIdentity(UserId user, IdentityId identityId) {
  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::removeIdentity(): Unable to start Transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return;
  }

  QSqlQuery query(db);
  query.prepare(queryString("delete_identity"));
  query.bindValue(":identityid", identityId.toInt());
  query.bindValue(":userid", user.toInt());
  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
  } else {
    db.commit();
  }
}

QList<CoreIdentity> PostgreSqlStorage::identities(UserId user) {
  QList<CoreIdentity> identities;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::identites(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return identities;
  }

  QSqlQuery query(db);
  query.prepare(queryString("select_identities"));
  query.bindValue(":userid", user.toInt());

  QSqlQuery nickQuery(db);
  nickQuery.prepare(queryString("select_nicks"));

  safeExec(query);

  while(query.next()) {
    CoreIdentity identity(IdentityId(query.value(0).toInt()));

    identity.setIdentityName(query.value(1).toString());
    identity.setRealName(query.value(2).toString());
    identity.setAwayNick(query.value(3).toString());
    identity.setAwayNickEnabled(!!query.value(4).toInt());
    identity.setAwayReason(query.value(5).toString());
    identity.setAwayReasonEnabled(!!query.value(6).toInt());
    identity.setAutoAwayEnabled(!!query.value(7).toInt());
    identity.setAutoAwayTime(query.value(8).toInt());
    identity.setAutoAwayReason(query.value(9).toString());
    identity.setAutoAwayReasonEnabled(!!query.value(10).toInt());
    identity.setDetachAwayEnabled(!!query.value(11).toInt());
    identity.setDetachAwayReason(query.value(12).toString());
    identity.setDetachAwayReasonEnabled(!!query.value(13).toInt());
    identity.setIdent(query.value(14).toString());
    identity.setKickReason(query.value(15).toString());
    identity.setPartReason(query.value(16).toString());
    identity.setQuitReason(query.value(17).toString());
#ifdef HAVE_SSL
    identity.setSslCert(query.value(18).toByteArray());
    identity.setSslKey(query.value(19).toByteArray());
#endif

    nickQuery.bindValue(":identityid", identity.id().toInt());
    QList<QString> nicks;
    safeExec(nickQuery);
    watchQuery(nickQuery);
    while(nickQuery.next()) {
      nicks << nickQuery.value(0).toString();
    }
    identity.setNicks(nicks);
    identities << identity;
  }
  db.commit();
  return identities;
}

NetworkId PostgreSqlStorage::createNetwork(UserId user, const NetworkInfo &info) {
  NetworkId networkId;

  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::createNetwork(): failed to begin transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }

  QSqlQuery query(db);
  query.prepare(queryString("insert_network"));
  query.bindValue(":userid", user.toInt());
  bindNetworkInfo(query, info);
  safeExec(query);
  if(query.lastError().isValid()) {
    watchQuery(query);
    db.rollback();
    return NetworkId();
  }

  qDebug() << "createNet:" << query.first() << query.value(0).toInt();
  networkId = query.value(0).toInt();

  if(!networkId.isValid()) {
    watchQuery(query);
    db.rollback();
    return NetworkId();
  }

  QSqlQuery insertServersQuery(db);
  insertServersQuery.prepare(queryString("insert_server"));
  foreach(Network::Server server, info.serverList) {
    insertServersQuery.bindValue(":userid", user.toInt());
    insertServersQuery.bindValue(":networkid", networkId.toInt());
    bindServerInfo(insertServersQuery, server);
    safeExec(insertServersQuery);
    if(!watchQuery(insertServersQuery)) {
      db.rollback();
      return NetworkId();
    }
  }

  if(!db.commit()) {
    qWarning() << "PostgreSqlStorage::updateNetwork(): commiting data failed!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return NetworkId();
  }
  return networkId;
}

void PostgreSqlStorage::bindNetworkInfo(QSqlQuery &query, const NetworkInfo &info) {
  query.bindValue(":networkname", info.networkName);
  query.bindValue(":identityid", info.identity.toInt());
  query.bindValue(":encodingcodec", QString(info.codecForEncoding));
  query.bindValue(":decodingcodec", QString(info.codecForDecoding));
  query.bindValue(":servercodec", QString(info.codecForServer));
  query.bindValue(":userandomserver", info.useRandomServer);
  query.bindValue(":perform", info.perform.join("\n"));
  query.bindValue(":useautoidentify", info.useAutoIdentify);
  query.bindValue(":autoidentifyservice", info.autoIdentifyService);
  query.bindValue(":autoidentifypassword", info.autoIdentifyPassword);
  query.bindValue(":useautoreconnect", info.useAutoReconnect);
  query.bindValue(":autoreconnectinterval", info.autoReconnectInterval);
  query.bindValue(":autoreconnectretries", info.autoReconnectRetries);
  query.bindValue(":unlimitedconnectretries", info.unlimitedReconnectRetries);
  query.bindValue(":rejoinchannels", info.rejoinChannels);
  if(info.networkId.isValid())
    query.bindValue(":networkid", info.networkId.toInt());
}

void PostgreSqlStorage::bindServerInfo(QSqlQuery &query, const Network::Server &server) {
  query.bindValue(":hostname", server.host);
  query.bindValue(":port", server.port);
  query.bindValue(":password", server.password);
  query.bindValue(":ssl", server.useSsl);
  query.bindValue(":sslversion", server.sslVersion);
  query.bindValue(":useproxy", server.useProxy);
  query.bindValue(":proxytype", server.proxyType);
  query.bindValue(":proxyhost", server.proxyHost);
  query.bindValue(":proxyport", server.proxyPort);
  query.bindValue(":proxyuser", server.proxyUser);
  query.bindValue(":proxypass", server.proxyPass);
}

bool PostgreSqlStorage::updateNetwork(UserId user, const NetworkInfo &info) {
  if(!isValidNetwork(user, info.networkId))
     return false;

  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::updateNetwork(): failed to begin transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }

  QSqlQuery updateQuery(db);
  updateQuery.prepare(queryString("update_network"));
  bindNetworkInfo(updateQuery, info);
  safeExec(updateQuery);
  if(!watchQuery(updateQuery)) {
    db.rollback();
    return false;
  }

  QSqlQuery dropServersQuery(db);
  dropServersQuery.prepare("DELETE FROM ircserver WHERE networkid = :networkid");
  dropServersQuery.bindValue(":networkid", info.networkId.toInt());
  safeExec(dropServersQuery);
  if(!watchQuery(dropServersQuery)) {
    db.rollback();
    return false;
  }

  QSqlQuery insertServersQuery(db);
  insertServersQuery.prepare(queryString("insert_server"));
  foreach(Network::Server server, info.serverList) {
    insertServersQuery.bindValue(":userid", user.toInt());
    insertServersQuery.bindValue(":networkid", info.networkId.toInt());
    bindServerInfo(insertServersQuery, server);
    safeExec(insertServersQuery);
    if(!watchQuery(insertServersQuery)) {
      db.rollback();
      return false;
    }
  }

  if(!db.commit()) {
    qWarning() << "PostgreSqlStorage::updateNetwork(): commiting data failed!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }
  return true;
}

bool PostgreSqlStorage::removeNetwork(UserId user, const NetworkId &networkId) {
  if(!isValidNetwork(user, networkId))
     return false;

  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::removeNetwork(): cannot start transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }

  QSqlQuery query(db);
  query.prepare(queryString("delete_network"));
  query.bindValue(":networkid", networkId.toInt());
  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
    return false;
  }

  db.commit();
  return true;
}

QList<NetworkInfo> PostgreSqlStorage::networks(UserId user) {
  QList<NetworkInfo> nets;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::networks(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return nets;
  }
  
  QSqlQuery networksQuery(db);
  networksQuery.prepare(queryString("select_networks_for_user"));
  networksQuery.bindValue(":userid", user.toInt());

  QSqlQuery serversQuery(db);
  serversQuery.prepare(queryString("select_servers_for_network"));

  safeExec(networksQuery);
  if(!watchQuery(networksQuery)) {
    db.rollback();
    return nets;
  }

  while(networksQuery.next()) {
    NetworkInfo net;
    net.networkId = networksQuery.value(0).toInt();
    net.networkName = networksQuery.value(1).toString();
    net.identity = networksQuery.value(2).toInt();
    net.codecForServer = networksQuery.value(3).toString().toAscii();
    net.codecForEncoding = networksQuery.value(4).toString().toAscii();
    net.codecForDecoding = networksQuery.value(5).toString().toAscii();
    net.useRandomServer = networksQuery.value(6).toBool();
    net.perform = networksQuery.value(7).toString().split("\n");
    net.useAutoIdentify = networksQuery.value(8).toBool();
    net.autoIdentifyService = networksQuery.value(9).toString();
    net.autoIdentifyPassword = networksQuery.value(10).toString();
    net.useAutoReconnect = networksQuery.value(11).toBool();
    net.autoReconnectInterval = networksQuery.value(12).toUInt();
    net.autoReconnectRetries = networksQuery.value(13).toInt();
    net.unlimitedReconnectRetries = networksQuery.value(14).toBool();
    net.rejoinChannels = networksQuery.value(15).toBool();

    serversQuery.bindValue(":networkid", net.networkId.toInt());
    safeExec(serversQuery);
    if(!watchQuery(serversQuery)) {
      db.rollback();
      return nets;
    }

    Network::ServerList servers;
    while(serversQuery.next()) {
      Network::Server server;
      server.host = serversQuery.value(0).toString();
      server.port = serversQuery.value(1).toUInt();
      server.password = serversQuery.value(2).toString();
      server.useSsl = serversQuery.value(3).toBool();
      server.sslVersion = serversQuery.value(4).toInt();
      server.useProxy = serversQuery.value(5).toBool();
      server.proxyType = serversQuery.value(6).toInt();
      server.proxyHost = serversQuery.value(7).toString();
      server.proxyPort = serversQuery.value(8).toUInt();
      server.proxyUser = serversQuery.value(9).toString();
      server.proxyPass = serversQuery.value(10).toString();
      servers << server;
    }
    net.serverList = servers;
    nets << net;
  }
  db.commit();
  return nets;
}

bool PostgreSqlStorage::isValidNetwork(UserId user, const NetworkId &networkId) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_networkExists"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  safeExec(query);

  watchQuery(query);
  if(!query.first())
    return false;

  Q_ASSERT(!query.next());
  return true;
}

bool PostgreSqlStorage::isValidBuffer(const UserId &user, const BufferId &bufferId) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_bufferExists"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":bufferid", bufferId.toInt());
  safeExec(query);

  watchQuery(query);
  if(!query.first())
    return false;

  Q_ASSERT(!query.next());
  return true;
}

QList<NetworkId> PostgreSqlStorage::connectedNetworks(UserId user) {
  QList<NetworkId> connectedNets;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::connectedNetworks(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return connectedNets;
  }

  QSqlQuery query(db);
  query.prepare(queryString("select_connected_networks"));
  query.bindValue(":userid", user.toInt());
  safeExec(query);
  watchQuery(query);

  while(query.next()) {
    connectedNets << query.value(0).toInt();
  }

  db.commit();
  return connectedNets;
}

void PostgreSqlStorage::setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_network_connected"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  query.bindValue(":connected", isConnected);
  safeExec(query);
  watchQuery(query);
}

QHash<QString, QString> PostgreSqlStorage::persistentChannels(UserId user, const NetworkId &networkId) {
  QHash<QString, QString> persistentChans;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::persistentChannels(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return persistentChans;
  }

  QSqlQuery query(db);
  query.prepare(queryString("select_persistent_channels"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  safeExec(query);
  watchQuery(query);

  while(query.next()) {
    persistentChans[query.value(0).toString()] = query.value(1).toString();
  }

  db.commit();
  return persistentChans;
}

void PostgreSqlStorage::setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_buffer_persistent_channel"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkId", networkId.toInt());
  query.bindValue(":buffercname", channel.toLower());
  query.bindValue(":joined", isJoined);
  safeExec(query);
  watchQuery(query);
}

void PostgreSqlStorage::setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_buffer_set_channel_key"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkId", networkId.toInt());
  query.bindValue(":buffercname", channel.toLower());
  query.bindValue(":key", key);
  safeExec(query);
  watchQuery(query);
}

QString PostgreSqlStorage::awayMessage(UserId user, NetworkId networkId) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_network_awaymsg"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  safeExec(query);
  watchQuery(query);
  QString awayMsg;
  if(query.first())
    awayMsg = query.value(0).toString();
  return awayMsg;
}

void PostgreSqlStorage::setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_network_set_awaymsg"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  query.bindValue(":awaymsg", awayMsg);
  safeExec(query);
  watchQuery(query);
}

QString PostgreSqlStorage::userModes(UserId user, NetworkId networkId) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_network_usermode"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  safeExec(query);
  watchQuery(query);
  QString modes;
  if(query.first())
    modes = query.value(0).toString();
  return modes;
}

void PostgreSqlStorage::setUserModes(UserId user, NetworkId networkId, const QString &userModes) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_network_set_usermode"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkid", networkId.toInt());
  query.bindValue(":usermode", userModes);
  safeExec(query);
  watchQuery(query);
}

BufferInfo PostgreSqlStorage::bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer, bool create) {
  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::bufferInfo(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return BufferInfo();
  }

  QSqlQuery query(db);
  query.prepare(queryString("select_bufferByName"));
  query.bindValue(":networkid", networkId.toInt());
  query.bindValue(":userid", user.toInt());
  query.bindValue(":buffercname", buffer.toLower());
  safeExec(query);

  if(query.first()) {
    BufferInfo bufferInfo = BufferInfo(query.value(0).toInt(), networkId, (BufferInfo::Type)query.value(1).toInt(), 0, buffer);
    if(query.next()) {
      qCritical() << "PostgreSqlStorage::getBufferInfo(): received more then one Buffer!";
      qCritical() << "         Query:" << query.lastQuery();
      qCritical() << "  bound Values:";
      QList<QVariant> list = query.boundValues().values();
      for (int i = 0; i < list.size(); ++i)
	qCritical() << i << ":" << list.at(i).toString().toAscii().data();
      Q_ASSERT(false);
    }
    db.commit();
    return bufferInfo;
  }

  if(!create) {
    db.rollback();
    return BufferInfo();
  }

  QSqlQuery createQuery(db);
  createQuery.prepare(queryString("insert_buffer"));
  createQuery.bindValue(":userid", user.toInt());
  createQuery.bindValue(":networkid", networkId.toInt());
  createQuery.bindValue(":buffertype", (int)type);
  createQuery.bindValue(":buffername", buffer);
  createQuery.bindValue(":buffercname", buffer.toLower());
  safeExec(createQuery);

  if(createQuery.lastError().isValid()) {
    qWarning() << "PostgreSqlStorage::bufferInfo(): unable to create buffer";
    watchQuery(createQuery);
    db.rollback();
    return BufferInfo();
  }

  createQuery.first();

  BufferInfo bufferInfo = BufferInfo(createQuery.value(0).toInt(), networkId, type, 0, buffer);
  db.commit();
  return bufferInfo;
}

BufferInfo PostgreSqlStorage::getBufferInfo(UserId user, const BufferId &bufferId) {
  QSqlQuery query(logDb());
  query.prepare(queryString("select_buffer_by_id"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":bufferid", bufferId.toInt());
  safeExec(query);
  if(!watchQuery(query))
    return BufferInfo();

  if(!query.first())
    return BufferInfo();

  BufferInfo bufferInfo(query.value(0).toInt(), query.value(1).toInt(), (BufferInfo::Type)query.value(2).toInt(), 0, query.value(4).toString());
  Q_ASSERT(!query.next());

  return bufferInfo;
}

QList<BufferInfo> PostgreSqlStorage::requestBuffers(UserId user) {
  QList<BufferInfo> bufferlist;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::requestBuffers(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return bufferlist;
  }
  
  QSqlQuery query(db);
  query.prepare(queryString("select_buffers"));
  query.bindValue(":userid", user.toInt());

  safeExec(query);
  watchQuery(query);
  while(query.next()) {
    bufferlist << BufferInfo(query.value(0).toInt(), query.value(1).toInt(), (BufferInfo::Type)query.value(2).toInt(), query.value(3).toInt(), query.value(4).toString());
  }
  db.commit();
  return bufferlist;
}

QList<BufferId> PostgreSqlStorage::requestBufferIdsForNetwork(UserId user, NetworkId networkId) {
  QList<BufferId> bufferList;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::requestBufferIdsForNetwork(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return bufferList;
  }
  
  QSqlQuery query(db);
  query.prepare(queryString("select_buffers_for_network"));
  query.bindValue(":networkid", networkId.toInt());
  query.bindValue(":userid", user.toInt());

  safeExec(query);
  watchQuery(query);
  while(query.next()) {
    bufferList << BufferId(query.value(0).toInt());
  }
  db.commit();
  return bufferList;
}

bool PostgreSqlStorage::removeBuffer(const UserId &user, const BufferId &bufferId) {
  if(!isValidBuffer(user, bufferId))
    return false;

  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::removeBuffer(): cannot start transaction!";
    return false;
  }

  QSqlQuery query(db);
  query.prepare(queryString("delete_buffer_for_bufferid"));
  query.bindValue(":bufferid", bufferId.toInt());
  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
    return false;
  }
  db.commit();
  return true;
}

bool PostgreSqlStorage::renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName) {
  if(!isValidBuffer(user, bufferId))
    return false;

  QSqlQuery query(logDb());
  query.prepare(queryString("update_buffer_name"));
  query.bindValue(":buffername", newName);
  query.bindValue(":buffercname", newName.toLower());
  query.bindValue(":bufferid", bufferId.toInt());
  safeExec(query);
  if(query.lastError().isValid()) {
    return false;
  }
  return true;
}

bool PostgreSqlStorage::mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2) {
  if(!isValidBuffer(user, bufferId1) || !isValidBuffer(user, bufferId2))
    return false;

  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::mergeBuffersPermanently(): cannot start transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }

  QSqlQuery query(db);
  query.prepare(queryString("update_backlog_bufferid"));
  query.bindValue(":oldbufferid", bufferId2.toInt());
  query.bindValue(":newbufferid", bufferId1.toInt());
  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
    return false;
  }

  QSqlQuery delBufferQuery(logDb());
  delBufferQuery.prepare(queryString("delete_buffer_for_bufferid"));
  delBufferQuery.bindValue(":bufferid", bufferId2.toInt());
  safeExec(delBufferQuery);
  if(!watchQuery(delBufferQuery)) {
    db.rollback();
    return false;
  }

  db.commit();
  return true;
}

void PostgreSqlStorage::setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_buffer_lastseen"));

  query.bindValue(":userid", user.toInt());
  query.bindValue(":bufferid", bufferId.toInt());
  query.bindValue(":lastseenmsgid", msgId.toInt());
  safeExec(query);
  watchQuery(query);
}

QHash<BufferId, MsgId> PostgreSqlStorage::bufferLastSeenMsgIds(UserId user) {
  QHash<BufferId, MsgId> lastSeenHash;

  QSqlDatabase db = logDb();
  if(!beginReadOnlyTransaction(db)) {
    qWarning() << "PostgreSqlStorage::bufferLastSeenMsgIds(): cannot start read only transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return lastSeenHash;
  }

  QSqlQuery query(db);
  query.prepare(queryString("select_buffer_lastseen_messages"));
  query.bindValue(":userid", user.toInt());
  safeExec(query);
  if(!watchQuery(query)) {
    db.rollback();
    return lastSeenHash;
  }

  while(query.next()) {
    lastSeenHash[query.value(0).toInt()] = query.value(1).toInt();
  }

  db.commit();
  return lastSeenHash;
}

MsgId PostgreSqlStorage::logMessage(Message msg) {
  QSqlDatabase db = logDb();
  if(!db.transaction()) {
    qWarning() << "PostgreSqlStorage::logMessage(): cannot start transaction!";
    qWarning() << " -" << qPrintable(db.lastError().text());
    return false;
  }

  QSqlQuery logMessageQuery(db);
  logMessageQuery.prepare(queryString("insert_message"));
  logMessageQuery.bindValue(":time", msg.timestamp().toTime_t());
  logMessageQuery.bindValue(":bufferid", msg.bufferInfo().bufferId().toInt());
  logMessageQuery.bindValue(":type", msg.type());
  logMessageQuery.bindValue(":flags", (int)msg.flags());
  logMessageQuery.bindValue(":sender", msg.sender());
  logMessageQuery.bindValue(":message", msg.contents());
  safeExec(logMessageQuery);

  if(logMessageQuery.lastError().isValid()) {
    // first we need to reset the transaction
    db.rollback();
    db.transaction(); 

    QSqlQuery addSenderQuery(db);
    addSenderQuery.prepare(queryString("insert_sender"));
    addSenderQuery.bindValue(":sender", msg.sender());
    safeExec(addSenderQuery);
    safeExec(logMessageQuery);

    if(!watchQuery(logMessageQuery)) {
      db.rollback();
      return MsgId();
    }
  }

  logMessageQuery.first();
  MsgId msgId = logMessageQuery.value(0).toInt();
  db.commit();

  Q_ASSERT(msgId.isValid());
  return msgId;
}

QList<Message> PostgreSqlStorage::requestMsgs(UserId user, BufferId bufferId, MsgId first, MsgId last, int limit) {
  QList<Message> messagelist;

  BufferInfo bufferInfo = getBufferInfo(user, bufferId);
  if(!bufferInfo.isValid())
    return messagelist;

  QSqlQuery query(logDb());
  
  if(last == -1 && first == -1) {
    query.prepare(queryString("select_messagesNewestK"));
  } else if(last == -1) {
    query.prepare(queryString("select_messagesNewerThan"));
    query.bindValue(":firstmsg", first.toInt());
  } else {
    query.prepare(queryString("select_messages"));
    query.bindValue(":lastmsg", last.toInt());
    query.bindValue(":firstmsg", first.toInt());
  }

  query.bindValue(":bufferid", bufferId.toInt());
  query.bindValue(":limit", limit);
  safeExec(query);

  watchQuery(query);

  while(query.next()) {
    Message msg(QDateTime::fromTime_t(query.value(1).toInt()),
                bufferInfo,
                (Message::Type)query.value(2).toUInt(),
                query.value(5).toString(),
                query.value(4).toString(),
                (Message::Flags)query.value(3).toUInt());
    msg.setMsgId(query.value(0).toInt());
    messagelist << msg;
  }
  return messagelist;
}

QList<Message> PostgreSqlStorage::requestAllMsgs(UserId user, MsgId first, MsgId last, int limit) {
  QList<Message> messagelist;

  QHash<BufferId, BufferInfo> bufferInfoHash;
  foreach(BufferInfo bufferInfo, requestBuffers(user)) {
    bufferInfoHash[bufferInfo.bufferId()] = bufferInfo;
  }

  QSqlQuery query(logDb());
  if(last == -1) {
    query.prepare(queryString("select_messagesAllNew"));
  } else {
    query.prepare(queryString("select_messagesAll"));
    query.bindValue(":lastmsg", last.toInt());
  }
  query.bindValue(":userid", user.toInt());
  query.bindValue(":firstmsg", first.toInt());
  query.bindValue(":limit", limit);
  safeExec(query);

  watchQuery(query);

  while(query.next()) {
    Message msg(QDateTime::fromTime_t(query.value(2).toInt()),
                bufferInfoHash[query.value(1).toInt()],
                (Message::Type)query.value(3).toUInt(),
                query.value(6).toString(),
                query.value(5).toString(),
                (Message::Flags)query.value(4).toUInt());
    msg.setMsgId(query.value(0).toInt());
    messagelist << msg;
  }

  return messagelist;
}

// void PostgreSqlStorage::safeExec(QSqlQuery &query) {
//   qDebug() << "PostgreSqlStorage::safeExec";
//   qDebug() << "   executing:\n" << query.executedQuery();
//   qDebug() << "   bound Values:";
//   QList<QVariant> list = query.boundValues().values();
//   for (int i = 0; i < list.size(); ++i)
//     qCritical() << i << ": " << list.at(i).toString().toAscii().data();

//   query.exec();

//   qDebug() << "Success:" << !query.lastError().isValid();
//   qDebug();

//   if(!query.lastError().isValid())
//     return;

//   qDebug() << "==================== ERROR ====================";
//   watchQuery(query);
//   qDebug() << "===============================================";
//   qDebug();
//   return;
// }

bool PostgreSqlStorage::beginReadOnlyTransaction(QSqlDatabase &db) {
  QSqlQuery query = db.exec("BEGIN TRANSACTION READ ONLY");
  return !query.lastError().isValid();
}

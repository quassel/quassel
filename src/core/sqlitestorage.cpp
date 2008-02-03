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

#include "sqlitestorage.h"

#include <QCryptographicHash>
#include <QtSql>

#include "network.h"

SqliteStorage::SqliteStorage(QObject *parent)
  : AbstractSqlStorage(parent)
{
}

SqliteStorage::~SqliteStorage() {
}

bool SqliteStorage::isAvailable() {
  if(!QSqlDatabase::isDriverAvailable("QSQLITE")) return false;
  return true;
}

QString SqliteStorage::displayName() {
  return QString("SQLite");
}

QString SqliteStorage::engineName() {
  return SqliteStorage::displayName();
}

int SqliteStorage::installedSchemaVersion() {
  QSqlQuery query = logDb().exec("SELECT value FROM coreinfo WHERE key = 'schemaversion'");
  if(query.first())
    return query.value(0).toInt();

  // maybe it's really old... (schema version 0)
  query = logDb().exec("SELECT MAX(version) FROM coreinfo");
  if(query.first())
    return query.value(0).toInt();

  return AbstractSqlStorage::installedSchemaVersion();
}

UserId SqliteStorage::addUser(const QString &user, const QString &password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();

  QSqlQuery query(logDb());
  query.prepare(queryString("insert_quasseluser"));
  query.bindValue(":username", user);
  query.bindValue(":password", cryptopass);
  query.exec();
  if(query.lastError().isValid() && query.lastError().number() == 19) { // user already exists - sadly 19 seems to be the general constraint violation error...
    return 0;
  }

  query.prepare(queryString("select_userid"));
  query.bindValue(":username", user);
  query.exec();
  query.first();
  UserId uid = query.value(0).toInt();
  emit userAdded(uid, user);
  return uid;
}

void SqliteStorage::updateUser(UserId user, const QString &password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();

  QSqlQuery query(logDb());
  query.prepare(queryString("update_userpassword"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":password", cryptopass);
  query.exec();
}

void SqliteStorage::renameUser(UserId user, const QString &newName) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_username"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":username", newName);
  query.exec();
  emit userRenamed(user, newName);
}

UserId SqliteStorage::validateUser(const QString &user, const QString &password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();

  QSqlQuery query(logDb());
  query.prepare(queryString("select_authuser"));
  query.bindValue(":username", user);
  query.bindValue(":password", cryptopass);
  query.exec();

  if(query.first()) {
    return query.value(0).toInt();
  } else {
    return 0;
  }
}

void SqliteStorage::delUser(UserId user) {
  QSqlQuery query(logDb());
  query.prepare(queryString("delete_backlog_by_uid"));
  query.bindValue(":userid", user.toInt());
  query.exec();
  
  query.prepare(queryString("delete_buffers_by_uid"));
  query.bindValue(":userid", user.toInt());
  query.exec();
  
  query.prepare(queryString("delete_networks_by_uid"));
  query.bindValue(":userid", user.toInt());
  query.exec();
  
  query.prepare(queryString("delete_quasseluser"));
  query.bindValue(":userid", user.toInt());
  query.exec();
  // I hate the lack of foreign keys and on delete cascade... :(
  emit userRemoved(user);
}

NetworkId SqliteStorage::createNetworkId(UserId user, const NetworkInfo &info) {
  NetworkId networkId;
  QSqlQuery query(logDb());
  query.prepare(queryString("insert_network"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkname", info.networkName);
  query.exec();
  
  networkId = getNetworkId(user, info.networkName);
  if(!networkId.isValid()) {
    watchQuery(&query);
  }
  return networkId;
}

NetworkId SqliteStorage::getNetworkId(UserId user, const QString &network) {
  QSqlQuery query(logDb());
  query.prepare("SELECT networkid FROM network "
		"WHERE userid = :userid AND networkname = :networkname");
  query.bindValue(":userid", user.toInt());
  query.bindValue(":networkname", network);
  query.exec();
  
  if(query.first())
    return query.value(0).toInt();
  else
    return NetworkId();
}

void SqliteStorage::createBuffer(UserId user, const NetworkId &networkId, const QString &buffer) {
  QSqlQuery *query = cachedQuery("insert_buffer");
  query->bindValue(":userid", user.toInt());
  query->bindValue(":networkid", networkId.toInt());
  query->bindValue(":buffername", buffer);
  query->exec();

  watchQuery(query);
}

BufferInfo SqliteStorage::getBufferInfo(UserId user, const NetworkId &networkId, const QString &buffer) {
  QSqlQuery *query = cachedQuery("select_bufferByName");
  query->bindValue(":networkid", networkId.toInt());
  query->bindValue(":userid", user.toInt());
  query->bindValue(":buffername", buffer);
  query->exec();

  if(!query->first()) {
    createBuffer(user, networkId, buffer);
    query->exec();
    if(!query->first()) {
      watchQuery(query);
      qWarning() << "unable to create BufferInfo for:" << user << networkId << buffer;
      return BufferInfo();
    }
  }

  BufferInfo bufferInfo = BufferInfo(query->value(0).toInt(), networkId, 0, buffer);
  if(query->next()) {
    qWarning() << "SqliteStorage::getBufferInfo(): received more then one Buffer!";
    qWarning() << "         Query:" << query->lastQuery();
    qWarning() << "  bound Values:" << query->boundValues();
    Q_ASSERT(false);
  }

  return bufferInfo;
}

QList<BufferInfo> SqliteStorage::requestBuffers(UserId user, QDateTime since) {
  uint time = 0;
  if(since.isValid())
    time = since.toTime_t();
  
  QList<BufferInfo> bufferlist;
  QSqlQuery query(logDb());
  query.prepare(queryString("select_buffers"));
  query.bindValue(":userid", user.toInt());
  query.bindValue(":time", time);
  
  query.exec();
  watchQuery(&query);
  while(query.next()) {
    bufferlist << BufferInfo(query.value(0).toInt(), query.value(2).toInt(), 0, query.value(1).toString());
  }
  return bufferlist;
}

MsgId SqliteStorage::logMessage(Message msg) {
  QSqlQuery *logMessageQuery = cachedQuery("insert_message");
  logMessageQuery->bindValue(":time", msg.timestamp().toTime_t());
  logMessageQuery->bindValue(":bufferid", msg.bufferInfo().bufferId().toInt());
  logMessageQuery->bindValue(":type", msg.type());
  logMessageQuery->bindValue(":flags", msg.flags());
  logMessageQuery->bindValue(":sender", msg.sender());
  logMessageQuery->bindValue(":message", msg.text());
  logMessageQuery->exec();
  
  if(logMessageQuery->lastError().isValid()) {
    // constraint violation - must be NOT NULL constraint - probably the sender is missing...
    if(logMessageQuery->lastError().number() == 19) {
      QSqlQuery *addSenderQuery = cachedQuery("insert_sender");
      addSenderQuery->bindValue(":sender", msg.sender());
      addSenderQuery->exec();
      watchQuery(addSenderQuery);
      logMessageQuery->exec();
      if(!watchQuery(logMessageQuery))
	return 0;
    } else {
      watchQuery(logMessageQuery);
    }
  }

  QSqlQuery *getLastMessageIdQuery = cachedQuery("select_lastMessage");
  getLastMessageIdQuery->bindValue(":time", msg.timestamp().toTime_t());
  getLastMessageIdQuery->bindValue(":bufferid", msg.bufferInfo().bufferId().toInt());
  getLastMessageIdQuery->bindValue(":type", msg.type());
  getLastMessageIdQuery->bindValue(":sender", msg.sender());
  getLastMessageIdQuery->exec();

  if(getLastMessageIdQuery->first()) {
    return getLastMessageIdQuery->value(0).toInt();
  } else { // somethin went wrong... :(
    qDebug() << getLastMessageIdQuery->lastQuery() << "time/bufferid/type/sender:" << msg.timestamp().toTime_t() << msg.bufferInfo().bufferId() << msg.type() << msg.sender();
    Q_ASSERT(false);
    return 0;
  }
}

QList<Message> SqliteStorage::requestMsgs(BufferInfo buffer, int lastmsgs, int offset) {
  QList<Message> messagelist;
  // we have to determine the real offset first
  QSqlQuery *offsetQuery = cachedQuery("select_messagesOffset");
  offsetQuery->bindValue(":bufferid", buffer.bufferId().toInt());
  offsetQuery->bindValue(":messageid", offset);
  offsetQuery->exec();
  offsetQuery->first();
  offset = offsetQuery->value(0).toInt();

  // now let's select the messages
  QSqlQuery *msgQuery = cachedQuery("select_messages");
  msgQuery->bindValue(":bufferid", buffer.bufferId().toInt());
  msgQuery->bindValue(":limit", lastmsgs);
  msgQuery->bindValue(":offset", offset);
  msgQuery->exec();
  
  watchQuery(msgQuery);
  
  while(msgQuery->next()) {
    Message msg(QDateTime::fromTime_t(msgQuery->value(1).toInt()),
                buffer,
                (Message::Type)msgQuery->value(2).toUInt(),
                msgQuery->value(5).toString(),
                msgQuery->value(4).toString(),
                msgQuery->value(3).toUInt());
    msg.setMsgId(msgQuery->value(0).toInt());
    messagelist << msg;
  }
  return messagelist;
}


QList<Message> SqliteStorage::requestMsgs(BufferInfo buffer, QDateTime since, int offset) {
  QList<Message> messagelist;
  // we have to determine the real offset first
  QSqlQuery *offsetQuery = cachedQuery("select_messagesSinceOffset");
  offsetQuery->bindValue(":bufferid", buffer.bufferId().toInt());
  offsetQuery->bindValue(":since", since.toTime_t());
  offsetQuery->exec();
  offsetQuery->first();
  offset = offsetQuery->value(0).toInt();

  // now let's select the messages
  QSqlQuery *msgQuery = cachedQuery("select_messagesSince");
  msgQuery->bindValue(":bufferid", buffer.bufferId().toInt());
  msgQuery->bindValue(":since", since.toTime_t());
  msgQuery->bindValue(":offset", offset);
  msgQuery->exec();

  watchQuery(msgQuery);
  
  while(msgQuery->next()) {
    Message msg(QDateTime::fromTime_t(msgQuery->value(1).toInt()),
                buffer,
                (Message::Type)msgQuery->value(2).toUInt(),
                msgQuery->value(5).toString(),
                msgQuery->value(4).toString(),
                msgQuery->value(3).toUInt());
    msg.setMsgId(msgQuery->value(0).toInt());
    messagelist << msg;
  }

  return messagelist;
}


QList<Message> SqliteStorage::requestMsgRange(BufferInfo buffer, int first, int last) {
  QList<Message> messagelist;
  QSqlQuery *rangeQuery = cachedQuery("select_messageRange");
  rangeQuery->bindValue(":bufferid", buffer.bufferId().toInt());
  rangeQuery->bindValue(":firstmsg", first);
  rangeQuery->bindValue(":lastmsg", last);
  rangeQuery->exec();

  watchQuery(rangeQuery);
  
  while(rangeQuery->next()) {
    Message msg(QDateTime::fromTime_t(rangeQuery->value(1).toInt()),
                buffer,
                (Message::Type)rangeQuery->value(2).toUInt(),
                rangeQuery->value(5).toString(),
                rangeQuery->value(4).toString(),
                rangeQuery->value(3).toUInt());
    msg.setMsgId(rangeQuery->value(0).toInt());
    messagelist << msg;
  }

  return messagelist;
}

QString SqliteStorage::backlogFile() {
  // kinda ugly, but I currently see no other way to do that
#ifdef Q_OS_WIN32
  QString quasselDir = QDir::homePath() + qgetenv("APPDATA") + "\\quassel\\";
#else
  QString quasselDir = QDir::homePath() + "/.quassel/";
#endif

  QDir qDir(quasselDir);
  if(!qDir.exists(quasselDir))
    qDir.mkpath(quasselDir);
  
  return quasselDir + "quassel-storage.sqlite";  
}


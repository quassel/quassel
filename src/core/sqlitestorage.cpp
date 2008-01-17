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
  UserId uid = query.value(0).toUInt();
  emit userAdded(uid, user);
  return uid;
}

void SqliteStorage::updateUser(UserId user, const QString &password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();

  QSqlQuery query(logDb());
  query.prepare(queryString("update_userpassword"));
  query.bindValue(":userid", user);
  query.bindValue(":password", cryptopass);
  query.exec();
}

void SqliteStorage::renameUser(UserId user, const QString &newName) {
  QSqlQuery query(logDb());
  query.prepare(queryString("update_username"));
  query.bindValue(":userid", user);
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
    return query.value(0).toUInt();
  } else {
    return 0;
  }
}

void SqliteStorage::delUser(UserId user) {
  QSqlQuery query(logDb());
  query.prepare(queryString("delete_backlog_by_uid"));
  query.bindValue(":userid", user);
  query.exec();
  
  query.prepare(queryString("delete_buffers_by_uid"));
  query.bindValue(":userid", user);
  query.exec();
  
  query.prepare(queryString("delete_networks_by_uid"));
  query.bindValue(":userid", user);
  query.exec();
  
  query.prepare(queryString("delete_quasseluser"));
  query.bindValue(":userid", user);
  query.exec();
  // I hate the lack of foreign keys and on delete cascade... :(
  emit userRemoved(user);
}

void SqliteStorage::createBuffer(UserId user, const QString &network, const QString &buffer) {
  QSqlQuery *createBufferQuery = cachedQuery("insert_buffer");
  createBufferQuery->bindValue(":userid", user);
  createBufferQuery->bindValue(":userid2", user);  // Qt can't handle same placeholder twice (maybe sqlites fault)
  createBufferQuery->bindValue(":networkname", network);
  createBufferQuery->bindValue(":buffername", buffer);
  createBufferQuery->exec();

  if(createBufferQuery->lastError().isValid()) {
    if(createBufferQuery->lastError().number() == 19) { // Null Constraint violation
      QSqlQuery *createNetworkQuery = cachedQuery("insert_network");
      createNetworkQuery->bindValue(":userid", user);
      createNetworkQuery->bindValue(":networkname", network);
      createNetworkQuery->exec();
      createBufferQuery->exec();
      Q_ASSERT(!createNetworkQuery->lastError().isValid());
      Q_ASSERT(!createBufferQuery->lastError().isValid());
    } else {
      // do panic!
      qDebug() << "failed to create Buffer: ErrNo:" << createBufferQuery->lastError().number() << "ErrMsg:" << createBufferQuery->lastError().text();
      Q_ASSERT(false);
    }
  }
}

uint SqliteStorage::getNetworkId(UserId user, const QString &network) {
  QSqlQuery query(logDb());
  query.prepare("SELECT networkid FROM network "
		"WHERE userid = :userid AND networkname = :networkname");
  query.bindValue(":userid", user);
  query.bindValue(":networkname", network);
  query.exec();
  
  if(query.first())
    return query.value(0).toUInt();
  else {
    createBuffer(user, network, "");
    query.exec();
    if(query.first())
      return query.value(0).toUInt();
    else {
      qWarning() << "NETWORK NOT FOUND:" << network << "for User:" << user;
      return 0;
    }
  }
}

BufferInfo SqliteStorage::getBufferInfo(UserId user, const QString &network, const QString &buffer) {
  BufferInfo bufferid;
  // TODO: get rid of this hackaround
  uint networkId = getNetworkId(user, network);

  QSqlQuery *getBufferInfoQuery = cachedQuery("select_bufferByName");
  getBufferInfoQuery->bindValue(":networkname", network);
  getBufferInfoQuery->bindValue(":userid", user);
  getBufferInfoQuery->bindValue(":userid2", user); // Qt can't handle same placeholder twice... though I guess it's sqlites fault
  getBufferInfoQuery->bindValue(":buffername", buffer);
  getBufferInfoQuery->exec();

  if(!getBufferInfoQuery->first()) {
    createBuffer(user, network, buffer);
    getBufferInfoQuery->exec();
    if(getBufferInfoQuery->first()) {
      bufferid = BufferInfo(getBufferInfoQuery->value(0).toUInt(), networkId, 0, network, buffer);
      emit bufferInfoUpdated(user, bufferid);
    }
  } else {
    bufferid = BufferInfo(getBufferInfoQuery->value(0).toUInt(), networkId, 0, network, buffer);
  }

  Q_ASSERT(!getBufferInfoQuery->next());

  return bufferid;
}

QList<BufferInfo> SqliteStorage::requestBuffers(UserId user, QDateTime since) {
  uint time = 0;
  if(since.isValid())
    time = since.toTime_t();
  
  QList<BufferInfo> bufferlist;
  QSqlQuery query(logDb());
  query.prepare(queryString("select_buffers"));
  query.bindValue(":userid", user);
  query.bindValue(":time", time);
  
  query.exec();
  watchQuery(&query);
  while(query.next()) {
    bufferlist << BufferInfo(query.value(0).toUInt(), query.value(2).toUInt(), 0, query.value(3).toString(), query.value(1).toString());
  }
  return bufferlist;
}

MsgId SqliteStorage::logMessage(Message msg) {
  QSqlQuery *logMessageQuery = cachedQuery("insert_message");
  logMessageQuery->bindValue(":time", msg.timestamp().toTime_t());
  logMessageQuery->bindValue(":bufferid", msg.buffer().uid());
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
  getLastMessageIdQuery->bindValue(":bufferid", msg.buffer().uid());
  getLastMessageIdQuery->bindValue(":type", msg.type());
  getLastMessageIdQuery->bindValue(":sender", msg.sender());
  getLastMessageIdQuery->exec();

  if(getLastMessageIdQuery->first()) {
    return getLastMessageIdQuery->value(0).toUInt();
  } else { // somethin went wrong... :(
    qDebug() << getLastMessageIdQuery->lastQuery() << "time/bufferid/type/sender:" << msg.timestamp().toTime_t() << msg.buffer().uid() << msg.type() << msg.sender();
    Q_ASSERT(false);
    return 0;
  }
}

QList<Message> SqliteStorage::requestMsgs(BufferInfo buffer, int lastmsgs, int offset) {
  QList<Message> messagelist;
  // we have to determine the real offset first
  QSqlQuery *requestMsgsOffsetQuery = cachedQuery("select_messagesOffset");
  requestMsgsOffsetQuery->bindValue(":bufferid", buffer.uid());
  requestMsgsOffsetQuery->bindValue(":messageid", offset);
  requestMsgsOffsetQuery->exec();
  requestMsgsOffsetQuery->first();
  offset = requestMsgsOffsetQuery->value(0).toUInt();

  // now let's select the messages
  QSqlQuery *requestMsgsQuery = cachedQuery("select_messages");
  requestMsgsQuery->bindValue(":bufferid", buffer.uid());
  requestMsgsQuery->bindValue(":bufferid2", buffer.uid());  // Qt can't handle the same placeholder used twice
  requestMsgsQuery->bindValue(":limit", lastmsgs);
  requestMsgsQuery->bindValue(":offset", offset);
  requestMsgsQuery->exec();
  while(requestMsgsQuery->next()) {
    Message msg(QDateTime::fromTime_t(requestMsgsQuery->value(1).toInt()),
                buffer,
                (Message::Type)requestMsgsQuery->value(2).toUInt(),
                requestMsgsQuery->value(5).toString(),
                requestMsgsQuery->value(4).toString(),
                requestMsgsQuery->value(3).toUInt());
    msg.setMsgId(requestMsgsQuery->value(0).toUInt());
    messagelist << msg;
  }
  return messagelist;
}


QList<Message> SqliteStorage::requestMsgs(BufferInfo buffer, QDateTime since, int offset) {
  QList<Message> messagelist;
  // we have to determine the real offset first
  QSqlQuery *requestMsgsSinceOffsetQuery = cachedQuery("select_messagesSinceOffset");
  requestMsgsSinceOffsetQuery->bindValue(":bufferid", buffer.uid());
  requestMsgsSinceOffsetQuery->bindValue(":since", since.toTime_t());
  requestMsgsSinceOffsetQuery->exec();
  requestMsgsSinceOffsetQuery->first();
  offset = requestMsgsSinceOffsetQuery->value(0).toUInt();  

  // now let's select the messages
  QSqlQuery *requestMsgsSinceQuery = cachedQuery("select_messagesSince");
  requestMsgsSinceQuery->bindValue(":bufferid", buffer.uid());
  requestMsgsSinceQuery->bindValue(":bufferid2", buffer.uid());
  requestMsgsSinceQuery->bindValue(":since", since.toTime_t());
  requestMsgsSinceQuery->bindValue(":offset", offset);
  requestMsgsSinceQuery->exec();

  while(requestMsgsSinceQuery->next()) {
    Message msg(QDateTime::fromTime_t(requestMsgsSinceQuery->value(1).toInt()),
                buffer,
                (Message::Type)requestMsgsSinceQuery->value(2).toUInt(),
                requestMsgsSinceQuery->value(5).toString(),
                requestMsgsSinceQuery->value(4).toString(),
                requestMsgsSinceQuery->value(3).toUInt());
    msg.setMsgId(requestMsgsSinceQuery->value(0).toUInt());
    messagelist << msg;
  }

  return messagelist;
}


QList<Message> SqliteStorage::requestMsgRange(BufferInfo buffer, int first, int last) {
  QList<Message> messagelist;
  QSqlQuery *requestMsgRangeQuery = cachedQuery("select_messageRange");
  requestMsgRangeQuery->bindValue(":bufferid", buffer.uid());
  requestMsgRangeQuery->bindValue(":bufferid2", buffer.uid());
  requestMsgRangeQuery->bindValue(":firstmsg", first);
  requestMsgRangeQuery->bindValue(":lastmsg", last);

  while(requestMsgRangeQuery->next()) {
    Message msg(QDateTime::fromTime_t(requestMsgRangeQuery->value(1).toInt()),
                buffer,
                (Message::Type)requestMsgRangeQuery->value(2).toUInt(),
                requestMsgRangeQuery->value(5).toString(),
                requestMsgRangeQuery->value(4).toString(),
                requestMsgRangeQuery->value(3).toUInt());
    msg.setMsgId(requestMsgRangeQuery->value(0).toUInt());
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


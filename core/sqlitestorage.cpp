/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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


SqliteStorage::SqliteStorage() {
  // TODO I don't think that this path is failsafe for windows users :) 
  QString backlogFile = Global::quasselDir + "/quassel-backlog-newstyle.sqlite";
  logDb = QSqlDatabase::addDatabase("QSQLITE");
  logDb.setDatabaseName(backlogFile);
  bool ok = logDb.open();

  if(!ok) {
    qWarning(tr("Could not open backlog database: %1").arg(logDb.lastError().text()).toAscii());
    qWarning(tr("Disabling logging...").toAscii());
    Q_ASSERT(ok);
    return;
  }
  
  if(!logDb.transaction()) {
    qWarning(tr("Database driver does not support transactions. This might lead to a corrupt database!").toAscii());
  }
  
  // check if the db schema is up to date
  QSqlQuery query = logDb.exec("SELECT MAX(version) FROM coreinfo");
  if(query.first()) {
    // TOTO VersionCheck
    //checkVersion(query.value(0));
    qDebug() << "Sqlite is ready. Quassel Schema Version:" << query.value(0).toUInt();
  } else {
    initDb();
  }
  
  // we will need those pretty often... so let's speed things up:
  createBufferQuery = new QSqlQuery();
  createBufferQuery->prepare("INSERT INTO buffer (userid, networkid, buffername) VALUES (:userid, (SELECT networkid FROM network WHERE networkname = :networkname), :buffername)");
  
  createNetworkQuery = new QSqlQuery();
  createNetworkQuery->prepare("INSERT INTO network (userid, networkname) VALUES (:userid, :networkname)");
  
  getBufferIdQuery = new QSqlQuery();
  getBufferIdQuery->prepare("SELECT bufferid FROM buffer "
                            "JOIN network ON buffer.networkid = network.networkid "
                            "WHERE network.networkname = :networkname AND buffer.userid = :userid AND buffer.buffername = :buffername");

  
  logMessageQuery = new QSqlQuery();
  logMessageQuery->prepare("INSERT INTO backlog (time, bufferid, type, flags, senderid, message) "
                           "VALUES (:time, :bufferid, :type, :flags, (SELECT senderid FROM sender WHERE sender = :sender), :message)");
  
  addSenderQuery = new QSqlQuery();
  addSenderQuery->prepare("INSERT INTO sender (sender) VALUES (:sender)");

  getLastMessageIdQuery = new QSqlQuery();
  getLastMessageIdQuery->prepare("SELECT messageid FROM backlog "
                                 "WHERE time = :time AND bufferid = :bufferid AND type = :type AND senderid = (SELECT senderid FROM sender WHERE sender = :sender)");

  requestMsgsOffsetQuery = new QSqlQuery();
  requestMsgsOffsetQuery->prepare("SELECT count(*) FROM backlog WHERE bufferid = :bufferid AND messageid < :messageid");
  
  requestMsgsQuery = new QSqlQuery();
  requestMsgsQuery->prepare("SELECT messageid, time,  type, flags, sender, message, displayname "
                            "FROM backlog "
                            "JOIN buffer ON backlog.bufferid = buffer.bufferid "
                            "JOIN sender ON backlog.senderid = sender.senderid "
                            "LEFT JOIN buffergroup ON buffer.groupid = buffergroup.groupid "
                            "WHERE buffer.bufferid = :bufferid OR buffer.groupid = (SELECT groupid FROM buffer WHERE bufferid = :bufferid) "
                            "ORDER BY messageid DESC "
                            "LIMIT :limit OFFSET :offset");
  
  requestMsgsSinceOffsetQuery = new QSqlQuery();
  requestMsgsSinceOffsetQuery->prepare("SELECT count(*) FROM backlog WHERE bufferid = :bufferid AND time >= :since");
  
  requestMsgsSinceQuery = new QSqlQuery();
  requestMsgsSinceQuery->prepare("SELECT messageid, time,  type, flags, sender, message, displayname "
                                 "FROM backlog "
                                 "JOIN buffer ON backlog.bufferid = buffer.bufferid "
                                 "JOIN sender ON backlog.senderid = sender.senderid "
                                 "LEFT JOIN buffergroup ON buffer.groupid = buffergroup.groupid "
                                 "WHERE (buffer.bufferid = :bufferid OR buffer.groupid = (SELECT groupid FROM buffer WHERE bufferid = :bufferid)) AND "
                                 "backlog.time >= :since "
                                 "ORDER BY messageid DESC "
                                 "LIMIT -1 OFFSET :offset");
  
  requestMsgRangeQuery = new QSqlQuery();
  requestMsgRangeQuery->prepare("SELECT messageid, time,  type, flags, sender, message, displayname "
                                "FROM backlog "
                                "JOIN buffer ON backlog.bufferid = buffer.bufferid "
                                "JOIN sender ON backlog.senderid = sender.senderid "
                                "LEFT JOIN buffergroup ON buffer.groupid = buffergroup.groupid "
                                "WHERE (buffer.bufferid = :bufferid OR buffer.groupid = (SELECT groupid FROM buffer WHERE bufferid = :bufferid)) AND "
                                "backlog.messageid >= :firstmsg AND backlog.messageid <= :lastmsg "
                                "ORDER BY messageid DESC ");
  
}

SqliteStorage::~SqliteStorage() {
  delete logMessageQuery;
  delete addSenderQuery;
  delete getLastMessageIdQuery;
  delete requestMsgsQuery;
  delete requestMsgsOffsetQuery;
  delete requestMsgsSinceQuery;
  delete requestMsgsSinceOffsetQuery;
  delete requestMsgRangeQuery;
  delete createNetworkQuery;
  delete createBufferQuery;
  delete getBufferIdQuery;
  logDb.close();
}


void SqliteStorage::initDb() {
  logDb.exec("CREATE TABLE quasseluser ("
             "userid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "username TEXT UNIQUE NOT NULL,"
             "password BLOB NOT NULL)");
  
  logDb.exec("CREATE TABLE sender ("
             "senderid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "sender TEXT UNIQUE NOT NULL)");
  
  logDb.exec("CREATE TABLE network ("
             "networkid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "userid INTEGER NOT NULL,"
             "networkname TEXT NOT NULL,"
             "UNIQUE (userid, networkname))");
  
  logDb.exec("CREATE TABLE buffergroup ("
             "groupid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "userid INTEGER NOT NULL,"
             "displayname TEXT)");
  
  logDb.exec("CREATE TABLE buffer ("
             "bufferid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "userid INTEGER NOT NULL,"
             "groupid INTEGER,"
             "networkid INTEGER NOT NULL,"
             "buffername TEXT NOT NULL)");
  
  logDb.exec("CREATE UNIQUE INDEX buffer_idx "
             "ON buffer(userid, networkid, buffername)");
    
  logDb.exec("CREATE TABLE backlog ("
             "messageid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "time INTEGER NOT NULL,"
             "bufferid INTEGER NOT NULL,"
             "type INTEGER NOT NULL,"
             "flags INTEGER NOT NULL,"
             "senderid INTEGER NOT NULL,"
             "message TEXT NOT NULL)");
  
  logDb.exec("CREATE TABLE coreinfo ("
             "updateid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
             "version INTEGER NOT NULL)");
  
  logDb.exec("INSERT INTO coreinfo (version) VALUES (0)");


  // something fucked up -> no logging possible
  if(logDb.lastError().isValid()) { 
    qWarning(tr("Could not create backlog table: %1").arg(logDb.lastError().text()).toAscii());
    qWarning(tr("Disabling logging...").toAscii());
    logDb.rollback();
    Q_ASSERT(false); // quassel does require logging
  } else {
    logDb.commit();
  }
}

bool SqliteStorage::isAvailable() {
  // oh yes we're available... at least I do hope so :)
  return true;
}

QString SqliteStorage::displayName() {
  // I think the class name is a got start here
  return QString("SqliteStorage");
}

UserId SqliteStorage::addUser(QString user, QString password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();
  
  QSqlQuery query(logDb);
  query.prepare("INSERT INTO quasseluser (username, password) VALUES (:username, :password)");
  query.bindValue(":username", user);
  query.bindValue(":password", cryptopass);
  query.exec();
  if(query.lastError().isValid() && query.lastError().number() == 19) { // user already exists - sadly 19 seems to be the general constraint violation error...
    return 0;
  } 
  logDb.commit();
  
  query.prepare("SELECT userid FROM quasseluser WHERE username = :username");
  query.bindValue(":username", user);
  query.exec();
  query.first();
  return query.value(0).toUInt();
}

void SqliteStorage::updateUser(UserId user, QString password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();
  
  QSqlQuery query(logDb);
  query.prepare("UPDATE quasseluser SET password = :password WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.bindValue(":password", cryptopass);
  query.exec();
  logDb.commit();
}

UserId SqliteStorage::validateUser(QString user, QString password) {
  QByteArray cryptopass = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1);
  cryptopass = cryptopass.toHex();
  
  QSqlQuery query(logDb);
  query.prepare("SELECT userid FROM quasseluser WHERE username = :username AND password = :password");
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
  QSqlQuery query(logDb);
  query.prepare("DELETE FROM backlog WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM buffer WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM buffergroup WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM network WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM quasseluser WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  logDb.commit();
  // I hate the lack of foreign keys and on delete cascade... :(
}

void SqliteStorage::createBuffer(UserId user, QString network, QString buffer) {
  qDebug() << "creating buffer:" << user << network << buffer;
  createBufferQuery->bindValue(":userid", user);
  createBufferQuery->bindValue(":networkname", network);
  createBufferQuery->bindValue(":buffername", buffer);
  createBufferQuery->exec();
  
  if(createBufferQuery->lastError().isValid()) {
    if(createBufferQuery->lastError().number() == 19) { // Null Constraint violation
      createNetworkQuery->bindValue(":userid", user);
      createNetworkQuery->bindValue(":networkname", network);
      createNetworkQuery->exec();
      createBufferQuery->exec();
      Q_ASSERT(!createNetworkQuery->lastError().isValid());
      Q_ASSERT(!createBufferQuery->lastError().isValid());
    } else {
      // do panic!
      qDebug() << "failed to create Buffer: ErrNo:" << createBufferQuery->lastError().number() << "ErrMsg:" << createBufferQuery->lastError().text();
    }
  }
  logDb.commit();
}

BufferId SqliteStorage::getBufferId(UserId user, QString network, QString buffer) {
  getBufferIdQuery->bindValue(":networkname", network);
  getBufferIdQuery->bindValue(":userid", user);
  getBufferIdQuery->bindValue(":buffername", buffer);
  getBufferIdQuery->exec();

  if(!getBufferIdQuery->first()) {
    createBuffer(user, network, buffer);
    getBufferIdQuery->exec();
    Q_ASSERT(getBufferIdQuery->first());
  }
  
  return BufferId(getBufferIdQuery->value(0).toUInt(), network, buffer);
}

QList<BufferId> SqliteStorage::requestBuffers(UserId user, QDateTime since) {
  QList<BufferId> bufferlist;
  QSqlQuery query(logDb);
  query.prepare("SELECT bufferid, networkname, buffername FROM buffer "
                "JOIN network ON buffer.networkid = network.networkid "
                "JOIN backlog ON buffer.bufferid = backlog.bufferid "
                "WHERE buffer.userid = :userid AND time >= :time");
  query.bindValue(":userid", user);
  query.bindValue(":time", since.toTime_t());
  query.exec();
  while(query.next()) {
    bufferlist << BufferId(query.value(0).toUInt(), query.value(1).toString(), query.value(2).toString());
  }
  return bufferlist;
}

MsgId SqliteStorage::logMessage(Message msg) {
  logMessageQuery->bindValue(":time", msg.timeStamp.toTime_t());
  logMessageQuery->bindValue(":bufferid", msg.buffer.uid());
  logMessageQuery->bindValue(":type", msg.type);
  logMessageQuery->bindValue(":flags", msg.flags);
  logMessageQuery->bindValue(":sender", msg.sender);
  logMessageQuery->bindValue(":message", msg.text);
  logMessageQuery->exec();

  // constraint violation - must be NOT NULL constraint - probably the sender is missing... 
  if(logMessageQuery->lastError().isValid()) {
    if(logMessageQuery->lastError().number() == 19) { 
      addSenderQuery->bindValue(":sender", msg.sender);
      addSenderQuery->exec();
      logMessageQuery->exec();
    } else {
      qDebug() << "unhandled DB Error in logMessage(): Number:" << logMessageQuery->lastError().number() << "ErrMsg:" << logMessageQuery->lastError().text();
    }
  }

  logDb.commit();
  
  getLastMessageIdQuery->bindValue(":time", msg.timeStamp.toTime_t());
  getLastMessageIdQuery->bindValue(":bufferid", msg.buffer.uid());
  getLastMessageIdQuery->bindValue(":type", msg.type);
  getLastMessageIdQuery->bindValue(":sender", msg.sender);
  getLastMessageIdQuery->exec();
  
  if(getLastMessageIdQuery->first()) {
    return getLastMessageIdQuery->value(0).toUInt();
  } else { // somethin went wrong... :(
    return 0;
  }
}

QList<Message> SqliteStorage::requestMsgs(BufferId buffer, int lastmsgs, int offset) {
  QList<Message> messagelist;
  // we have to determine the real offset first
  requestMsgsOffsetQuery->bindValue(":bufferid", buffer.uid());
  requestMsgsOffsetQuery->bindValue(":messageid", offset);
  requestMsgsOffsetQuery->exec();
  requestMsgsOffsetQuery->first();
  offset = requestMsgsOffsetQuery->value(0).toUInt();
  
  // now let's select the messages
  requestMsgsQuery->bindValue(":bufferid", buffer.uid());
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
    msg.msgId = requestMsgsQuery->value(0).toUInt();
    messagelist << msg;
  }
  
  return messagelist;
}


QList<Message> SqliteStorage::requestMsgs(BufferId buffer, QDateTime since, int offset) {
  QList<Message> messagelist;
  // we have to determine the real offset first
  requestMsgsSinceOffsetQuery->bindValue(":bufferid", buffer.uid());
  requestMsgsSinceOffsetQuery->bindValue(":since", since.toTime_t());
  requestMsgsSinceOffsetQuery->exec();
  requestMsgsSinceOffsetQuery->first();
  offset = requestMsgsSinceOffsetQuery->value(0).toUInt();  
  
  // now let's select the messages
  requestMsgsSinceQuery->bindValue(":bufferid", buffer.uid());
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
    msg.msgId = requestMsgsSinceQuery->value(0).toUInt();
    messagelist << msg;
  }
  
    
  return messagelist;
}


QList<Message> SqliteStorage::requestMsgRange(BufferId buffer, int first, int last) {
  QList<Message> messagelist;
  requestMsgRangeQuery->bindValue(":bufferid", buffer.uid());
  requestMsgRangeQuery->bindValue(":firstmsg", first);
  requestMsgRangeQuery->bindValue(":lastmsg", last);
  
  while(requestMsgRangeQuery->next()) {
    Message msg(QDateTime::fromTime_t(requestMsgRangeQuery->value(1).toInt()),
                buffer,
                (Message::Type)requestMsgRangeQuery->value(2).toUInt(),
                requestMsgRangeQuery->value(5).toString(),
                requestMsgRangeQuery->value(4).toString(),
                requestMsgRangeQuery->value(3).toUInt());
    msg.msgId = requestMsgRangeQuery->value(0).toUInt();
    messagelist << msg;
  }
  
  return messagelist;
}

void SqliteStorage::importOldBacklog() {
  QSqlQuery query(logDb);
  int user;
  query.prepare("SELECT MIN(userid) FROM quasseluser");
  query.exec();
  if(!query.first()) {
    qDebug() << "create a user first!";
  } else {
    user = query.value(0).toUInt();
  }
  query.prepare("DELETE FROM backlog WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM buffer WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM buffergroup WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  query.prepare("DELETE FROM network WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();
  logDb.commit();
  qDebug() << "All userdata has been deleted";
  qDebug() << "importing old backlog files...";
  initBackLogOld(user);
  logDb.commit();
  return;
}




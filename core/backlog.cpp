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

#include "backlog.h"
#include "util.h"

#define DBVERSION 1

Backlog::Backlog() {


}


Backlog::~Backlog() {
  logDb.close();

  // FIXME Old stuff
  foreach(QDataStream *s, logStreams) {
    delete s;
  }
  foreach(QFile *f, logFiles) {
    if(f->isOpen()) f->close();
    delete f;
  }
}


void Backlog::init(QString _user) {
  user = _user;
  QDir backlogDir = QDir(Global::quasselDir);
  if(!backlogDir.exists()) {
    qWarning(QString("Creating backlog directory \"%1\"...").arg(backlogDir.absolutePath()).toAscii());
    if(!backlogDir.mkpath(backlogDir.absolutePath())) {
      qWarning(QString("Could not create backlog directory! Disabling logging...").toAscii());
      backlogEnabled = false;
      return;
    }
  }
  QString backlogFile = Global::quasselDir + "/quassel-backlog.sqlite";
  logDb = QSqlDatabase::addDatabase("QSQLITE", user);
  logDb.setDatabaseName(backlogFile);
  bool ok = logDb.open();
  if(!ok) {
    qWarning(tr("Could not open backlog database: %1").arg(logDb.lastError().text()).toAscii());
    qWarning(tr("Disabling logging...").toAscii());
    backlogEnabled = false; return;
  }

  if(!logDb.transaction()) qWarning(tr("Database driver does not support transactions. This might lead to a corrupt database!").toAscii());
  QString tname = QString("'Backlog$%1$'").arg(user);
  QSqlQuery query(logDb);
  /* DEBUG */
  //query.exec(QString("DROP TABLE %1").arg(tname)); // DEBUG
  //query.exec(QString("DROP TABLE 'Senders$%1$'").arg(user));
  //query.exec(QString("DROP TABLE 'Buffers$%1$'").arg(user));
  /* END DEBUG */
  query.exec(QString("CREATE TABLE IF NOT EXISTS %1 ("
      "MsgId INTEGER PRIMARY KEY AUTOINCREMENT,"
      "Time INTEGER,"
      "BufferId INTEGER,"
      "Type INTEGER,"
      "Flags INTEGER,"
      "SenderId INTEGER,"
      "Text BLOB"
      ")").arg(tname));
  query.exec(QString("INSERT OR REPLACE INTO %1 (MsgId, SenderId, Text) VALUES (0, '$VERSION$', %2)").arg(tname).arg(DBVERSION));
  query.exec(QString("CREATE TABLE IF NOT EXISTS 'Senders$%1$' (SenderId INTEGER PRIMARY KEY AUTOINCREMENT, Sender BLOB)").arg(user));
  query.exec(QString("CREATE TABLE IF NOT EXISTS 'Buffers$%1$' (BufferId INTEGER PRIMARY KEY AUTOINCREMENT, GroupId INTEGER, Network BLOB, Buffer BLOB)").arg(user));
  if(query.lastError().isValid()) {
    qWarning(tr("Could not create backlog table: %1").arg(query.lastError().text()).toAscii());
    qWarning(tr("Disabling logging...").toAscii());
    logDb.rollback();
    backlogEnabled = false; return;
  }
  // Find the next free uid numbers
  query.exec(QString("SELECT MsgId FROM %1 ORDER BY MsgId DESC LIMIT 1").arg(tname));
  query.first();
  if(query.value(0).isValid()) nextMsgId = query.value(0).toUInt() + 1;
  else {
    qWarning(tr("Something is wrong with the backlog database! %1").arg(query.lastError().text()).toAscii());
    nextMsgId = 1;
  }
  query.exec(QString("SELECT BufferId FROM 'Buffers$%1$' ORDER BY BufferId DESC LIMIT 1").arg(user));
  if(query.first()) {
    if(query.value(0).isValid()) nextBufferId = query.value(0).toUInt() + 1;
    else {
      qWarning(tr("Something is wrong with the backlog database! %1").arg(query.lastError().text()).toAscii());
      nextBufferId = 0;
    }
  } else nextBufferId = 0;
  query.exec(QString("SELECT SenderId FROM 'Senders$%1$' ORDER BY SenderId DESC LIMIT 1").arg(user));
  if(query.first()) {
    if(query.value(0).isValid()) nextSenderId = query.value(0).toUInt() + 1;
    else {
      qWarning(tr("Something is wrong with the backlog database! %1").arg(query.lastError().text()).toAscii());
      nextSenderId = 0;
    }
  } else nextSenderId = 0;
  logDb.commit();
  backlogEnabled = true;
}

uint Backlog::logMessage(Message msg) {
  if(!backlogEnabled) return 0;
  bool ok;
  logDb.transaction();
  QSqlQuery query(logDb);
  QString s = msg.sender; s.replace('\'', "''"); QByteArray bs = s.toUtf8().toHex();
  QString t = msg.text; t.replace('\'', "''");
  // Let's do some space-saving optimizations...
  query.exec(QString("SELECT SenderId FROM 'Senders$%1$' WHERE Sender == X'%2'").arg(user).arg(bs.constData()));
  int suid;
  if(!query.first()) {
    query.exec(QString("INSERT INTO 'Senders$%1$' (SenderId, Sender) VALUES (%2, X'%3')").arg(user).arg(nextSenderId).arg(bs.constData()));
    suid = nextSenderId;
  } else suid = query.value(0).toInt();
  query.exec(QString("INSERT INTO 'Backlog$%1$' (MsgId, Time, BufferId, Type, Flags, SenderId, Text) VALUES (%2, %3, %4, %5, %6, %7, X'%8')").arg(user)
      .arg(nextMsgId).arg(msg.timeStamp.toTime_t()).arg(msg.buffer.uid()).arg(msg.type).arg(msg.flags).arg(suid).arg(t.toUtf8().toHex().constData()));

  if(query.lastError().isValid()) {
    qWarning(tr("Database error while logging: %1").arg(query.lastError().text()).toAscii());
    logDb.rollback();
    return 0;
  }

  nextMsgId++;
  if(suid == nextSenderId) nextSenderId++;
  logDb.commit();
  return nextMsgId - 1;
}

// TODO: optimize by keeping free IDs in memory? What about deleted IDs? Nickchanges for queries?
BufferId Backlog::getBufferId(QString net, QString buf) {
  if(!backlogEnabled) {
    return BufferId(0, net, buf);
  }
  QByteArray n = net.toUtf8().toHex();
  QByteArray b = buf.toUtf8().toHex();
  logDb.transaction();
  QSqlQuery query(logDb);
  int uid = -1;
  query.exec(QString("SELECT BufferId FROM 'Buffers$%1$' WHERE Network == X'%2' AND Buffer == X'%3'").arg(user).arg(n.constData()).arg(b.constData()));
  if(!query.first()) {
    // TODO: joined buffers/queries
    query.exec(QString("INSERT INTO 'Buffers$%1$' (BufferId, GroupId, Network, Buffer) VALUES (%2, %2, X'%3', X'%4')").arg(user).arg(nextBufferId).arg(n.constData()).arg(b.constData()));
    uid = nextBufferId++;
  } else uid = query.value(0).toInt();
  logDb.commit();
  return BufferId(uid, net, buf, uid);  // FIXME (joined buffers)
}

QList<BufferId> Backlog::requestBuffers(QDateTime since) {
  QList<BufferId> result;
  QSqlQuery query(logDb);
  if(!since.isValid()) {
    query.exec(QString("SELECT BufferId, GroupId, Network, Buffer FROM 'Buffers$%1$'").arg(user));
  } else {
    query.exec(QString("SELECT DISTINCT 'Buffers$%1$'.BufferId, GroupId, Network, Buffer FROM 'Buffers$%1$' NATURAL JOIN 'Backlog$%1$' "
                       "WHERE Time >= %2").arg(user).arg(since.toTime_t()));
  }
  while(query.next()) {
    result.append(BufferId(query.value(0).toUInt(), QString::fromUtf8(query.value(2).toByteArray()), QString::fromUtf8(query.value(3).toByteArray()), query.value(1).toUInt()));
  }
  return result;
}

QList<Message> Backlog::requestMsgs(BufferId id, int lastlines, int offset) {
  QList<Message> result;
  QSqlQuery query(logDb);
  QString limit;
  if(lastlines > 0) limit = QString("LIMIT %1").arg(lastlines);
  query.exec(QString("SELECT MsgId, Time, Type, Flags, Sender, Text FROM 'Senders$%1$' NATURAL JOIN 'Backlog$%1$' "
                     "WHERE BufferId IN (SELECT BufferId FROM 'Buffers$%1$' WHERE GroupId == %2) ORDER BY MsgId DESC %3").arg(user).arg(id.groupId()).arg(limit));
  while(query.next()) {
    if(offset >= 0 && query.value(0).toInt() >= offset) continue;
    Message msg(QDateTime::fromTime_t(query.value(1).toInt()), id, (Message::Type)query.value(2).toUInt(), QString::fromUtf8(query.value(5).toByteArray()),
                QString::fromUtf8(query.value(4).toByteArray()), query.value(3).toUInt());
    msg.msgId = query.value(0).toUInt();
    result.append(msg);
  }
  return result;
}


// OBSOLETE
// This is kept here for importing the old file-based backlog.

void Backlog::importOldBacklog() {
  qDebug() << "Deleting backlog database...";
  logDb.exec(QString("DELETE FROM 'Backlog$%1$' WHERE SenderId != '$VERSION$'").arg(user));
  logDb.exec(QString("DELETE FROM 'Senders$%1$'").arg(user));
  logDb.exec(QString("DELETE FROM 'Buffers$%1$'").arg(user));
  nextMsgId = 1; nextBufferId = 1; nextSenderId = 1;
  qDebug() << "Importing old backlog files...";
  initBackLogOld();
  if(!backLogEnabledOld) return;
  logDb.exec("VACUUM");
  qDebug() << "Backlog successfully imported, you have to restart Quassel now!";
  exit(0);

}

// file name scheme: quassel-backlog-2006-29-10.bin
void Backlog::initBackLogOld() {
  backLogDir = QDir(Global::quasselDir + "/backlog");
  if(!backLogDir.exists()) {
    qWarning(QString("Creating backlog directory \"%1\"...").arg(backLogDir.absolutePath()).toAscii());
    if(!backLogDir.mkpath(backLogDir.absolutePath())) {
      qWarning(QString("Could not create backlog directory! Disabling logging...").toAscii());
      backLogEnabledOld = false;
      return;
    }
  }
  backLogDir.refresh();
  //if(!backLogDir.isReadable()) {
  //  qWarning(QString("Cannot read directory \"%1\". Disabling logging...").arg(backLogDir.absolutePath()).toAscii());
  //  backLogEnabled = false;
  //  return;
  //}
  QStringList networks = backLogDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable, QDir::Name);
  foreach(QString net, networks) {
    QDir dir(backLogDir.absolutePath() + "/" + net);
    if(!dir.exists()) {
      qWarning(QString("Could not change to directory \"%1\"!").arg(dir.absolutePath()).toAscii());
      continue;
    }
    QStringList logs = dir.entryList(QStringList("quassel-backlog-*.bin"), QDir::Files|QDir::Readable, QDir::Name);
    foreach(QString name, logs) {
      QFile f(dir.absolutePath() + "/" + name);
      if(!f.open(QIODevice::ReadOnly)) {
        qWarning(QString("Could not open \"%1\" for reading!").arg(f.fileName()).toAscii());
        continue;
      }
      QDataStream in(&f);
      in.setVersion(QDataStream::Qt_4_2);
      QByteArray verstring; quint8 vernum; in >> verstring >> vernum;
      if(verstring != BACKLOG_STRING) {
        qWarning(QString("\"%1\" is not a Quassel backlog file!").arg(f.fileName()).toAscii());
        f.close(); continue;
      }
      if(vernum != BACKLOG_FORMAT) {
        qWarning(QString("\"%1\": Version mismatch!").arg(f.fileName()).toAscii());
        f.close(); continue;
      }
      qDebug() << "Reading backlog from" << f.fileName();
      logFileDates[net] = QDate::fromString(f.fileName(),
                                            QString("'%1/quassel-backlog-'yyyy-MM-dd'.bin'").arg(dir.absolutePath()));
      if(!logFileDates[net].isValid()) {
        qWarning(QString("\"%1\" has an invalid file name!").arg(f.fileName()).toAscii());
      }
      while(!in.atEnd()) {
        quint8 t, f;
        quint32 ts;
        QByteArray s, m, targ;
        in >> ts >> t >> f >> targ >> s >> m;
        QString target = QString::fromUtf8(targ);
        QString sender = QString::fromUtf8(s);
        QString text = QString::fromUtf8(m);
        BufferId id;
        if((f & Message::PrivMsg) && !(f & Message::Self)) {
          id = getBufferId(net, sender);
        } else {
          id = getBufferId(net, target);
        }
        Message msg(QDateTime::fromTime_t(ts), id, (Message::Type)t, text, sender, f);
        //backLog[net].append(m);
        logMessage(msg);
      }
      f.close();
    }
  }
  backLogEnabledOld = true;
}


/** Log a core message (emitted via a displayMsg() signal) to the backlog file.
 * If a file for the current day does not exist, one will be created. Otherwise, messages will be appended.
 * The file header is the string defined by BACKLOG_STRING, followed by a quint8 specifying the format
 * version (BACKLOG_FORMAT). The rest is simply serialized Message objects.
 */
void Backlog::logMessageOld(QString net, Message msg) {
  backLog[net].append(msg);
  if(!logFileDirs.contains(net)) {
    QDir dir(backLogDir.absolutePath() + "/" + net);
    if(!dir.exists()) {
      qWarning(QString("Creating backlog directory \"%1\"...").arg(dir.absolutePath()).toAscii());
      if(!dir.mkpath(dir.absolutePath())) {
        qWarning(QString("Could not create backlog directory!").toAscii());
        return;
      }
    }
    logFileDirs[net] = dir;
    Q_ASSERT(!logFiles.contains(net) && !logStreams.contains(net));
    if(!logFiles.contains(net)) logFiles[net] = new QFile();
    if(!logStreams.contains(net)) logStreams[net] = new QDataStream();
  }
  if(!logFileDates[net].isValid() || logFileDates[net] < QDate::currentDate()) {
    if(logFiles[net]->isOpen()) logFiles[net]->close();
    logFileDates[net] = QDate::currentDate();
  }
  if(!logFiles[net]->isOpen()) {
    logFiles[net]->setFileName(QString("%1/%2").arg(logFileDirs[net].absolutePath())
        .arg(logFileDates[net].toString("'quassel-backlog-'yyyy-MM-dd'.bin'")));
    if(!logFiles[net]->open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Unbuffered)) {
      qWarning(QString("Could not open \"%1\" for writing: %2")
          .arg(logFiles[net]->fileName()).arg(logFiles[net]->errorString()).toAscii());
      return;
    }
    logStreams[net]->setDevice(logFiles[net]); logStreams[net]->setVersion(QDataStream::Qt_4_2);
    if(!logFiles[net]->size()) *logStreams[net] << BACKLOG_STRING << (quint8)BACKLOG_FORMAT;
  }
  *logStreams[net] << msg;
}



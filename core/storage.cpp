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

#include "storage.h"


// OBSOLETE
// This is kept here for importing the old file-based backlog.

/* This is a sample!

void Storage::importOldBacklog() {
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
*/

// file name scheme: quassel-backlog-2006-29-10.bin
void Storage::initBackLogOld(UserId uid) {
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
          id = getBufferId(uid, net, sender);
        } else {
          id = getBufferId(uid, net, target);
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



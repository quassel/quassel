/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#include "core.h"
#include "server.h"
#include "global.h"
#include "util.h"
#include "coreproxy.h"

#include <QtSql>
#include <QSettings>

Core::Core() {
  if(core) qFatal("Trying to instantiate more than one Core object!");

  connect(coreProxy, SIGNAL(requestServerStates()), this, SIGNAL(serverStateRequested()));
  connect(coreProxy, SIGNAL(gsRequestConnect(QStringList)), this, SLOT(connectToIrc(QStringList)));
  connect(coreProxy, SIGNAL(gsUserInput(QString, QString, QString)), this, SIGNAL(msgFromGUI(QString, QString, QString)));
  connect(this, SIGNAL(displayMsg(QString, Message)), coreProxy, SLOT(csDisplayMsg(QString, Message)));
  connect(this, SIGNAL(displayStatusMsg(QString, QString)), coreProxy, SLOT(csDisplayStatusMsg(QString, QString)));

  // Read global settings from config file
  QSettings s;
  s.beginGroup("Global");
  QString key;
  foreach(key, s.childKeys()) {
    global->updateData(key, s.value(key));
  }
  initBackLogOld();
  global->updateData("CoreReady", true);
  // Now that we are in sync, we can connect signals to automatically store further updates.
  // I don't think we care if global data changed locally or if it was updated by a client. 
  connect(global, SIGNAL(dataUpdatedRemotely(QString)), SLOT(globalDataUpdated(QString)));
  connect(global, SIGNAL(dataPutLocally(QString)), SLOT(globalDataUpdated(QString)));

}

Core::~Core() {
  //foreach(Server *s, servers) {
  //  delete s;
  //}
  foreach(QDataStream *s, logStreams) {
    delete s;
  }
  foreach(QFile *f, logFiles) {
    if(f->isOpen()) f->close();
    delete f;
  }
  logDb.close();
}

void Core::globalDataUpdated(QString key) {
  QVariant data = global->getData(key);
  QSettings s;
  s.setValue(QString("Global/")+key, data);
}

void Core::connectToIrc(QStringList networks) {
  foreach(QString net, networks) {
    if(servers.contains(net)) {

    } else {
      Server *server = new Server(net);
      connect(this, SIGNAL(serverStateRequested()), server, SLOT(sendState()));
      connect(this, SIGNAL(connectToIrc(QString)), server, SLOT(connectToIrc(QString)));
      connect(this, SIGNAL(disconnectFromIrc(QString)), server, SLOT(disconnectFromIrc(QString)));
      connect(this, SIGNAL(msgFromGUI(QString, QString, QString)), server, SLOT(userInput(QString, QString, QString)));
      connect(server, SIGNAL(serverState(QString, VarMap)), coreProxy, SLOT(csServerState(QString, VarMap)));
      connect(server, SIGNAL(displayMsg(Message)), this, SLOT(recvMessageFromServer(Message)));
      connect(server, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));
      connect(server, SIGNAL(modeSet(QString, QString, QString)), coreProxy, SLOT(csModeSet(QString, QString, QString)));
      connect(server, SIGNAL(topicSet(QString, QString, QString)), coreProxy, SLOT(csTopicSet(QString, QString, QString)));
      connect(server, SIGNAL(setNicks(QString, QString, QStringList)), coreProxy, SLOT(csSetNicks(QString, QString, QStringList)));
      connect(server, SIGNAL(nickAdded(QString, QString, VarMap)), coreProxy, SLOT(csNickAdded(QString, QString, VarMap)));
      connect(server, SIGNAL(nickRenamed(QString, QString, QString)), coreProxy, SLOT(csNickRenamed(QString, QString, QString)));
      connect(server, SIGNAL(nickRemoved(QString, QString)), coreProxy, SLOT(csNickRemoved(QString, QString)));
      connect(server, SIGNAL(nickUpdated(QString, QString, VarMap)), coreProxy, SLOT(csNickUpdated(QString, QString, VarMap)));
      connect(server, SIGNAL(ownNickSet(QString, QString)), coreProxy, SLOT(csOwnNickSet(QString, QString)));
      connect(server, SIGNAL(queryRequested(QString, QString)), coreProxy, SLOT(csQueryRequested(QString, QString)));
      // add error handling
      connect(server, SIGNAL(connected(QString)), coreProxy, SLOT(csServerConnected(QString)));
      connect(server, SIGNAL(disconnected(QString)), this, SLOT(serverDisconnected(QString)));

      server->start();
      servers[net] = server;
    }
    emit connectToIrc(net);
  }
}

void Core::serverDisconnected(QString net) {
  delete servers[net];
  servers.remove(net);
  coreProxy->csServerDisconnected(net);
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void Core::recvMessageFromServer(Message msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  logMessageOld(s->getNetwork(), msg);
  emit displayMsg(s->getNetwork(), msg);
}

void Core::recvStatusMsgFromServer(QString msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  emit displayStatusMsg(s->getNetwork(), msg);
}

void Core::initBackLog() {
  QDir backLogDir = QDir(Global::quasselDir);
  if(!backLogDir.exists()) {
    qWarning(QString("Creating backlog directory \"%1\"...").arg(backLogDir.absolutePath()).toAscii());
    if(!backLogDir.mkpath(backLogDir.absolutePath())) {
      qWarning(QString("Could not create backlog directory! Disabling logging...").toAscii());
      backLogEnabled = false;
      return;
    }
  }
  QString backLogFile = Global::quasselDir + "/quassel-backlog.sqlite";
  logDb = QSqlDatabase::addDatabase("QSQLITE", "backlog");
  logDb.setDatabaseName(backLogFile);
  bool ok = logDb.open();
  if(!ok) {
    qWarning(tr("Could not open backlog database: %1").arg(logDb.lastError().text()).toAscii());
    qWarning(tr("Disabling logging...").toAscii());
  }
  // TODO store database version
  QSqlQuery query = logDb.exec("CREATE TABLE IF NOT EXISTS backlog ("
                         "Time INTEGER, User TEXT, Network TEXT, Buffer TEXT, Message BLOB"
                         ");");
  if(query.lastError().isValid()) {
    qWarning(tr("Could not create backlog table: %1").arg(query.lastError().text()).toAscii());
    qWarning(tr("Disabling logging...").toAscii());
    backLogEnabled = false;
    return;
  }

  backLogEnabled = true;
}

// file name scheme: quassel-backlog-2006-29-10.bin
void Core::initBackLogOld() {
  backLogDir = QDir(Global::quasselDir + "/backlog");
  if(!backLogDir.exists()) {
    qWarning(QString("Creating backlog directory \"%1\"...").arg(backLogDir.absolutePath()).toAscii());
    if(!backLogDir.mkpath(backLogDir.absolutePath())) {
      qWarning(QString("Could not create backlog directory! Disabling logging...").toAscii());
      backLogEnabled = false;
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
      //qDebug() << "Reading backlog from" << f.fileName();
      logFileDates[net] = QDate::fromString(f.fileName(),
          QString("'%1/quassel-backlog-'yyyy-MM-dd'.bin'").arg(dir.absolutePath()));
      if(!logFileDates[net].isValid()) {
        qWarning(QString("\"%1\" has an invalid file name!").arg(f.fileName()).toAscii());
      }
      while(!in.atEnd()) {
        Message m;
        in >> m;
        backLog[net].append(m);
      }
      f.close();
    }
  }
  backLogEnabled = true;
}

void Core::logMessage(QString net, Message msg) {
  if(!backLogEnabled) return;
  QString buf;
  if(msg.flags & Message::PrivMsg) {
    // query
    if(msg.flags & Message::Self) buf = msg.target;
    else buf = nickFromMask(msg.sender);
  } else {
    buf = msg.target;
  }
  QSqlQuery query = logDb.exec(QString("INSERT INTO backlog Time, User, Network, Buffer, Message "
                               "VALUES %1, %2, %3, %4, %5;")
      .arg(msg.timeStamp.toTime_t()).arg("Default").arg(net).arg(buf).arg(msg.text));
  if(query.lastError().isValid()) {
    qWarning(tr("Error while logging to database: %1").arg(query.lastError().text()).toAscii());
  }

}


/** Log a core message (emitted via a displayMsg() signal) to the backlog file.
 * If a file for the current day does not exist, one will be created. Otherwise, messages will be appended.
 * The file header is the string defined by BACKLOG_STRING, followed by a quint8 specifying the format
 * version (BACKLOG_FORMAT). The rest is simply serialized Message objects.
 */
void Core::logMessageOld(QString net, Message msg) {
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

Core *core = 0;

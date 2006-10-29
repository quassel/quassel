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
#include "coreproxy.h"

#include <QSettings>

Core::Core() {
  if(core) qFatal("Trying to instantiate more than one Core object!");

  connect(coreProxy, SIGNAL(gsRequestConnect(QStringList)), this, SLOT(connectToIrc(QStringList)));
  connect(coreProxy, SIGNAL(gsUserInput(QString, QString, QString)), this, SIGNAL(msgFromGUI(QString, QString, QString)));
  connect(this, SIGNAL(displayMsg(QString, QString, Message)), coreProxy, SLOT(csDisplayMsg(QString, QString, Message)));
  connect(this, SIGNAL(displayStatusMsg(QString, QString)), coreProxy, SLOT(csDisplayStatusMsg(QString, QString)));

  // Read global settings from config file
  QSettings s;
  s.beginGroup("Global");
  QString key;
  foreach(key, s.childKeys()) {
    global->updateData(key, s.value(key));
  }
  initBackLog();
  global->updateData("CoreReady", true);
  // Now that we are in sync, we can connect signals to automatically store further updates.
  // I don't think we care if global data changed locally or if it was updated by a client. 
  connect(global, SIGNAL(dataUpdatedRemotely(QString)), SLOT(globalDataUpdated(QString)));
  connect(global, SIGNAL(dataPutLocally(QString)), SLOT(globalDataUpdated(QString)));

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
      connect(this, SIGNAL(connectToIrc(QString)), server, SLOT(connectToIrc(QString)));
      connect(this, SIGNAL(disconnectFromIrc(QString)), server, SLOT(disconnectFromIrc(QString)));
      connect(this, SIGNAL(msgFromGUI(QString, QString, QString)), server, SLOT(userInput(QString, QString, QString)));
      connect(server, SIGNAL(displayMsg(QString, Message)), this, SLOT(recvMessageFromServer(QString, Message)));
      connect(server, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));
      connect(server, SIGNAL(modeSet(QString, QString, QString)), coreProxy, SLOT(csModeSet(QString, QString, QString)));
      connect(server, SIGNAL(topicSet(QString, QString, QString)), coreProxy, SLOT(csTopicSet(QString, QString, QString)));
      connect(server, SIGNAL(setNicks(QString, QString, QStringList)), coreProxy, SLOT(csSetNicks(QString, QString, QStringList)));
      connect(server, SIGNAL(nickAdded(QString, QString, VarMap)), coreProxy, SLOT(csNickAdded(QString, QString, VarMap)));
      connect(server, SIGNAL(nickRenamed(QString, QString, QString)), coreProxy, SLOT(csNickRenamed(QString, QString, QString)));
      connect(server, SIGNAL(nickRemoved(QString, QString)), coreProxy, SLOT(csNickRemoved(QString, QString)));
      connect(server, SIGNAL(nickUpdated(QString, QString, VarMap)), coreProxy, SLOT(csNickUpdated(QString, QString, VarMap)));
      connect(server, SIGNAL(ownNickSet(QString, QString)), coreProxy, SLOT(csOwnNickSet(QString, QString)));
      // add error handling

      server->start();
      servers[net] = server;
    }
    emit connectToIrc(net);
  }
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void Core::recvMessageFromServer(QString buf, Message msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  logMessage(msg);
  emit displayMsg(s->getNetwork(), buf, msg);
}

void Core::recvStatusMsgFromServer(QString msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  emit displayStatusMsg(s->getNetwork(), msg);
}

// file name scheme: quassel-backlog-2006-29-10.bin
void Core::initBackLog() {
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
  QStringList logs = backLogDir.entryList(QStringList("quassel-backlog-*.bin"), QDir::Files|QDir::Readable, QDir::Name);
  foreach(QString name, logs) {
    QFile f(backLogDir.absolutePath() + "/" + name);
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
    currentLogFileDate = QDate::fromString(f.fileName(), QString("'%1/quassel-backlog-'yyyy-MM-dd'.bin'").arg(backLogDir.absolutePath()));
    if(!currentLogFileDate.isValid()) {
      qWarning(QString("\"%1\" has an invalid file name!").arg(f.fileName()).toAscii());
    }
    while(!in.atEnd()) {
      Message m;
      in >> m;
      backLog.append(m);
    }
    f.close();
  }
  backLogEnabled = true;
}

/** Log a core message (emitted via a displayMsg() signal) to the backlog file.
 * If a file for the current day does not exist, one will be created. Otherwise, messages will be appended.
 * The file header is the string defined by BACKLOG_STRING, followed by a quint8 specifying the format
 * version (BACKLOG_FORMAT). The rest is simply serialized Message objects.
 */
void Core::logMessage(Message msg) {
  backLog.append(msg);
  if(!currentLogFileDate.isValid() || currentLogFileDate < QDate::currentDate()) {
    if(currentLogFile.isOpen()) currentLogFile.close();
    currentLogFileDate = QDate::currentDate();
  }
  if(!currentLogFile.isOpen()) {
    currentLogFile.setFileName(backLogDir.absolutePath() + "/" + currentLogFileDate.toString("'quassel-backlog-'yyyy-MM-dd'.bin'"));
    if(!currentLogFile.open(QIODevice::WriteOnly|QIODevice::Append)) {
      qWarning(QString("Could not open \"%1\" for writing: %2").arg(currentLogFile.fileName()).arg(currentLogFile.errorString()).toAscii());
      return;
    }
    logStream.setDevice(&currentLogFile); logStream.setVersion(QDataStream::Qt_4_2);
    if(!currentLogFile.size()) logStream << BACKLOG_STRING << (quint8)BACKLOG_FORMAT;
  }
  logStream << msg;
}

Core *core = 0;

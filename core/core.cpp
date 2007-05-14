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
  connect(coreProxy, SIGNAL(gsUserInput(BufferId, QString)), this, SLOT(msgFromGUI(BufferId, QString)));
  connect(coreProxy, SIGNAL(gsImportBacklog()), &backlog, SLOT(importOldBacklog()));
  connect(coreProxy, SIGNAL(gsRequestBacklog(BufferId, QVariant, QVariant)), this, SLOT(sendBacklog(BufferId, QVariant, QVariant)));
  connect(this, SIGNAL(displayMsg(Message)), coreProxy, SLOT(csDisplayMsg(Message)));
  connect(this, SIGNAL(displayStatusMsg(QString, QString)), coreProxy, SLOT(csDisplayStatusMsg(QString, QString)));
  connect(this, SIGNAL(backlogData(BufferId, QList<QVariant>, bool)), coreProxy, SLOT(csBacklogData(BufferId, QList<QVariant>, bool)));

  // Read global settings from config file
  QSettings s;
  s.beginGroup("Global");
  QString key;
  foreach(key, s.childKeys()) {
    global->updateData(key, s.value(key));
  }
  backlog.init("Default"); // FIXME
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
      //connect(server, SIGNAL(displayMsg(Message)), this, SLOT(recvMessageFromServer(Message)));
      connect(server, SIGNAL(displayMsg(Message::Type, QString, QString, QString, quint8)), this, SLOT(recvMessageFromServer(Message::Type, QString, QString, QString, quint8)));
      connect(server, SIGNAL(displayStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));
      connect(server, SIGNAL(modeSet(QString, QString, QString)), coreProxy, SLOT(csModeSet(QString, QString, QString)));
      connect(server, SIGNAL(topicSet(QString, QString, QString)), coreProxy, SLOT(csTopicSet(QString, QString, QString)));
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

void Core::msgFromGUI(BufferId bufid, QString msg) {
  emit msgFromGUI(bufid.network(), bufid.buffer(), msg);
}

// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.

void Core::recvMessageFromServer(Message::Type type, QString target, QString text, QString sender, quint8 flags) {
  Server *s = qobject_cast<Server*>(this->sender());
  Q_ASSERT(s);
  BufferId buf;
  if((flags & Message::PrivMsg) && !(flags & Message::Self)) {
    buf = backlog.getBufferId(s->getNetwork(), nickFromMask(sender));
  } else {
    buf = backlog.getBufferId(s->getNetwork(), target);
  }
  Message msg(buf, type, text, sender, flags);
  msg.msgId = backlog.logMessage(msg);
  emit displayMsg(msg);
}
/*
void Core::recvMessageFromServer(Message msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  logMessage(s->getNetwork(), msg);
  emit displayMsg(s->getNetwork(), msg);
}
*/

void Core::recvStatusMsgFromServer(QString msg) {
  Server *s = qobject_cast<Server*>(sender());
  Q_ASSERT(s);
  emit displayStatusMsg(s->getNetwork(), msg);
}

QList<BufferId> Core::getBuffers() {
  return backlog.requestBuffers();
}

void Core::sendBacklog(BufferId id, QVariant v1, QVariant v2) {
  QList<QVariant> log;
  QList<Message> msglist;
  if(v1.type() == QVariant::DateTime) {


  } else {
    msglist = backlog.requestMsgs(id, v1.toInt(), v2.toInt());
  }

  // Send messages out in smaller packages - we don't want to make the signal data too large!
  for(int i = 0; i < msglist.count(); i++) {
    log.append(QVariant::fromValue(msglist[i]));
    if(log.count() >= 5) {
      emit backlogData(id, log, i >= msglist.count() - 1);
      log.clear();
    }
  }
  if(log.count() > 0) emit backlogData(id, log, true);
}


Core *core = 0;

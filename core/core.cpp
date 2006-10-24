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
  connect(this, SIGNAL(sendMessage(QString, QString, Message)), coreProxy, SLOT(csSendMessage(QString, QString, Message)));
  connect(this, SIGNAL(sendStatusMsg(QString, QString)), coreProxy, SLOT(csSendStatusMsg(QString, QString)));

  // Read global settings from config file
  QSettings s;
  s.beginGroup("Global");
  QString key;
  foreach(key, s.childKeys()) {
    global->updateData(key, s.value(key));
  }
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
      connect(server, SIGNAL(sendMessage(QString, Message)), this, SLOT(recvMessageFromServer(QString, Message)));
      connect(server, SIGNAL(sendStatusMsg(QString)), this, SLOT(recvStatusMsgFromServer(QString)));
      connect(server, SIGNAL(setTopic(QString, QString, QString)), coreProxy, SLOT(csSetTopic(QString, QString, QString)));
      connect(server, SIGNAL(setNicks(QString, QString, QStringList)), coreProxy, SLOT(csSetNicks(QString, QString, QStringList)));
      // add error handling

      server->start();
      servers[net] = server;
    }
    emit connectToIrc(net);
  }
}

void Core::recvMessageFromServer(QString buf, Message msg) {
  Q_ASSERT(sender());
  QString net = qobject_cast<Server*>(sender())->getNetwork();
  emit sendMessage(net, buf, msg);
}

void Core::recvStatusMsgFromServer(QString msg) {
  Q_ASSERT(sender());
  QString net = qobject_cast<Server*>(sender())->getNetwork();
  emit sendStatusMsg(net, msg);
}


Core *core = 0;

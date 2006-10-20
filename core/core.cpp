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
#include "quassel.h"
#include "coreproxy.h"

#include <QSettings>

Core::Core() {
  if(core) qFatal("Trying to instantiate more than one Core object!");

  connect(coreProxy, SIGNAL(gsRequestConnect(QString, quint16)), this, SLOT(connectToIrc(QString, quint16)));
  connect(coreProxy, SIGNAL(gsUserInput(QString)), this, SLOT(inputLine(QString)));

  connect(&server, SIGNAL(recvLine(QString)), coreProxy, SLOT(csCoreMessage(QString)));

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

  server.start();
}

void Core::connectToIrc(const QString &h, quint16 port) {
  if(server.isConnected()) return;
  qDebug() << "Core: Connecting to " << h << ":" << port;
  server.connectToIrc(h, port);
}

void Core::inputLine(QString s) {
  server.putRawLine(s);

}

void Core::globalDataUpdated(QString key) {
  QVariant data = global->getData(key);
  QSettings s;
  s.setValue(QString("Global/")+key, data);
}

Core *core = 0;

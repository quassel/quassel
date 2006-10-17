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

#include <QSettings>

void Core::init() {
  Server::init();

}

void Core::run() {

  connect(&server, SIGNAL(recvLine(const QString &)), this, SIGNAL(outputLine(const QString &)));
  //connect(
  server.start();
  exec();
}

void Core::connectToIrc( const QString &h, quint16 port) {
  server.connectToIrc(h, port);
}

void Core::inputLine(const QString &s) {
  server.putRawLine( s);

}

VarMap Core::loadIdentities() {
  QSettings s;
  return s.value("Network/Identities").toMap();
}

void Core::storeIdentities(VarMap identities) {
  QSettings s;
  s.setValue("Network/Identities", identities);
}

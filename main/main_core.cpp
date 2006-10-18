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
#include <iostream>

#include <QCoreApplication>

#include "quassel.h"
#include "core.h"
#include "coreproxy.h"

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("The Quassel Team");

  Quassel::runMode = Quassel::CoreOnly;
  quassel = new Quassel();
  coreProxy = new CoreProxy();
  core = new Core();

  //Logger *logger = new Logger();
  //Quassel::setLogger(logger);

  core->init();

  int exitCode = app.exec();
  delete core;
  delete coreProxy;
  delete quassel;
  return exitCode;
}

Core *core = 0;

//GUIProxy::send(uint func, QVariant arg) {
  /*
  switch(func) {
    case LOAD_IDENTITIES: return (QVariant) CoreProxy::loadIdentities();
    case STORE_IDENTITIES: CoreProxy::storeIdentities(arg.toMap()); return 0;

  }
  */

//}

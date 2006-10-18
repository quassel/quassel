/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
 *   devel@quassel-irc.org                                                 *
 *                                                                          *
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

#include <QApplication>

#include "core.h"
#include "quassel.h"
#include "guiproxy.h"
#include "coreproxy.h"

#include "mainwin.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QApplication::setOrganizationDomain("quassel-irc.org");
  QApplication::setApplicationName("Quassel IRC");
  QApplication::setOrganizationName("The Quassel Team");

  Quassel::runMode = Quassel::Monolithic;
  quassel = new Quassel();
  guiProxy = new GUIProxy();
  coreProxy = new CoreProxy();
  core = new Core();

  core->init();

  MainWin mainWin;
  mainWin.show();
  int exitCode = app.exec();
  delete core;
  delete guiProxy;
  delete coreProxy;
  delete quassel;
  return exitCode;
}

void GUIProxy::send(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  coreProxy->recv(sig, arg1, arg2, arg3);
}

void CoreProxy::recv(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  switch(sig) {
    case GS_USER_INPUT: emit gsUserInput(arg1.toString()); break;
    case GS_REQUEST_CONNECT: emit gsRequestConnect(arg1.toString(), arg2.toUInt()); break;
    default: qWarning() << "Unknown signal in CoreProxy::recv: " << sig;
  }
}

void CoreProxy::send(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  guiProxy->recv(sig, arg1, arg2, arg3);
}

void GUIProxy::recv(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  switch(sig) {
    case CS_CORE_MESSAGE: emit csCoreMessage(arg1.toString()); break;
    default: qWarning() << "Unknown signal in GUIProxy::recv: " << sig;
  }
}

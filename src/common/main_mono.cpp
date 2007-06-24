/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#include "global.h"
#include "settings.h"

#if defined BUILD_CORE
#include <QCoreApplication>
#include "core.h"

#elif defined BUILD_QTGUI
#include <QApplication>
#include "style.h"
#include "client.h"
#include "clientproxy.h"
#include "mainwin.h"

#elif defined BUILD_MONO
#include <QApplication>
#include "core.h"
#include "coreproxy.h"
#include "style.h"
#include "client.h"
#include "clientproxy.h"
#include "mainwin.h"

#else
#error "Something is wrong - you need to #define a build mode!"
#endif

int main(int argc, char **argv) {
#if defined BUILD_CORE
  Global::runMode = Global::CoreOnly;
  QCoreApplication app(argc, argv);
#elif defined BUILD_QTGUI
  Global::runMode = Global::ClientOnly;
  QApplication app(argc, argv);
#else
  Global::runMode = Global::Monolithic;
  QApplication app(argc, argv);
#endif

  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel IRC Development Team");

  Global::quasselDir = QDir::homePath() + "/.quassel";

#ifdef BUILD_MONO
  QObject::connect(Core::localSession(), SIGNAL(proxySignal(CoreSignal, QVariant, QVariant, QVariant)), ClientProxy::instance(), SLOT(recv(CoreSignal, QVariant, QVariant, QVariant)));
  QObject::connect(ClientProxy::instance(), SIGNAL(send(ClientSignal, QVariant, QVariant, QVariant)), Core::localSession(), SLOT(processSignal(ClientSignal, QVariant, QVariant, QVariant)));
#endif

  Settings::init();
  Style::init();

  MainWin *mainWin = new MainWin();
  Client::init(mainWin);
  mainWin->init();
  int exitCode = app.exec();
  // the mainWin has to be deleted before the Core
  // if not Quassel will crash on exit under certain conditions since the gui
  // still wants to access clientdata
  delete mainWin;
  Client::destroy();
  Core::destroy();
  return exitCode;
}

void Client::syncToCore() {
  //Q_ASSERT(Global::data("CoreReady").toBool());
  coreBuffers = Core::localSession()->buffers();
  // NOTE: We don't need to request server states, because in the monolithic version there can't be
  //       any servers connected at this stage...
}


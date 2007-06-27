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
#include "quasselui.h"

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
  //AbstractUi *foo = new AbstractUi();
  //foo->init();
  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel IRC Development Team");

  Global::quasselDir = QDir::homePath() + "/.quassel";
#ifndef BUILD_QTGUI
  Core::instance();  // create and init the core
#endif

  Settings::init();

#ifndef BUILD_CORE
  Style::init();
  QtGui *gui = new QtGui();
  Client::init(gui);
  gui->init();
//#else
//  Core::instance(); // create and init the core object
#endif

  int exitCode = app.exec();

#ifndef BUILD_CORE
  // the mainWin has to be deleted before the Core
  // if not Quassel will crash on exit under certain conditions since the gui
  // still wants to access clientdata
  delete gui;
  Client::destroy();
#endif
#ifndef BUILD_QTGUI
  Core::destroy();
#endif

  return exitCode;
}

#ifdef BUILD_QTGUI
QVariant Client::connectToLocalCore(QString, QString) { return QVariant(); }
void Client::disconnectFromLocalCore() {}
#elif defined BUILD_MONO

QVariant Client::connectToLocalCore(QString user, QString passwd) {
  // TODO catch exceptions
  QVariant reply = Core::connectLocalClient(user, passwd);
  QObject::connect(Core::localSession(), SIGNAL(proxySignal(CoreSignal, QVariant, QVariant, QVariant)), ClientProxy::instance(), SLOT(recv(CoreSignal, QVariant, QVariant, QVariant)));
  QObject::connect(ClientProxy::instance(), SIGNAL(send(ClientSignal, QVariant, QVariant, QVariant)), Core::localSession(), SLOT(processSignal(ClientSignal, QVariant, QVariant, QVariant)));
  return reply;
}

void Client::disconnectFromLocalCore() {
  disconnect(Core::localSession(), 0, ClientProxy::instance(), 0);
  disconnect(ClientProxy::instance(), 0, Core::localSession(), 0);
  Core::disconnectLocalClient();
}

#endif

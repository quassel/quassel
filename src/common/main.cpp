/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
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

#include "global.h"
#include "settings.h"
#include <QString>
#include <QTranslator>

#if defined BUILD_CORE
#include <QCoreApplication>
#include <QDir>
#include "core.h"
#include "message.h"

#elif defined BUILD_QTUI
#include <QApplication>
#include "client.h"
#include "qtui.h"

#elif defined BUILD_MONO
#include <QApplication>
#include "client.h"
#include "core.h"
#include "coresession.h"
#include "qtui.h"

#else
#error "Something is wrong - you need to #define a build mode!"
#endif

#include <signal.h>

//! Signal handler for graceful shutdown.
void handle_signal(int sig) {
  qWarning(QString("Caught signal %1 - exiting.").arg(sig).toAscii());
  QCoreApplication::quit();
}

int main(int argc, char **argv) {
  // We catch SIGTERM and SIGINT (caused by Ctrl+C) to graceful shutdown Quassel.
  signal(SIGTERM, handle_signal);
  signal(SIGINT, handle_signal);

  qRegisterMetaType<QVariant>("QVariant");
  qRegisterMetaType<Message>("Message");
  qRegisterMetaType<BufferInfo>("BufferInfo");
  qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
  qRegisterMetaTypeStreamOperators<Message>("Message");
  qRegisterMetaTypeStreamOperators<BufferInfo>("BufferInfo");

#if defined BUILD_CORE
  Global::runMode = Global::CoreOnly;
  QCoreApplication app(argc, argv);
#elif defined BUILD_QTUI
  Global::runMode = Global::ClientOnly;
  QApplication app(argc, argv);
#else
  Global::runMode = Global::Monolithic;
  QApplication app(argc, argv);
#endif

  // Set up i18n support
  QLocale locale = QLocale::system();
  QTranslator translator;
  translator.load(QString(":i18n/quassel_%1").arg(locale.name()));
  app.installTranslator(&translator);

  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel IRC Development Team");  // FIXME

#ifndef BUILD_QTUI
  Core::instance();  // create and init the core
#endif

  //Settings::init();

#ifndef BUILD_CORE
  QtUi *gui = new QtUi();
  Client::init(gui);
  gui->init();
#endif

#ifndef BUILD_QTUI
  if(!QCoreApplication::arguments().contains("--norestore")) {
    Core::restoreState();
  }
#endif

  int exitCode = app.exec();

#ifndef BUILD_QTUI
  Core::saveState();
#endif

#ifndef BUILD_CORE
  // the mainWin has to be deleted before the Core
  // if not Quassel will crash on exit under certain conditions since the gui
  // still wants to access clientdata
  delete gui;
  Client::destroy();
#endif
#ifndef BUILD_QTUI
  Core::destroy();
#endif

  return exitCode;
}

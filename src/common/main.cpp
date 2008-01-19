/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include <QDateTime>
#include <QString>
#include <QTimer>
#include <QTranslator>

#include "global.h"
#include "settings.h"

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

  Global::registerMetaTypes();

#include "../../version.inc"

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

  qsrand(QDateTime::currentDateTime().toTime_t());

  // Set up i18n support
  QLocale locale = QLocale::system();

  QTranslator qtTranslator;
  qtTranslator.load(QString(":i18n/qt_%1").arg(locale.name()));
  app.installTranslator(&qtTranslator);

  QTranslator quasselTranslator;
  quasselTranslator.load(QString(":i18n/quassel_%1").arg(locale.name()));
  app.installTranslator(&quasselTranslator);

  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel Project");

  // Check if a non-standard core port is requested
  QStringList args = QCoreApplication::arguments();  // TODO Build a CLI parser

  Global::defaultPort = 4242;
  int idx;
  if((idx = args.indexOf("-p")) > 0 && idx < args.count() - 1) {
    int port = args[idx+1].toInt();
    if(port >= 1024 && port < 65536) Global::defaultPort = port;
  }

#ifndef BUILD_QTUI
  Core::instance();  // create and init the core
#endif

  //Settings::init();

#ifndef BUILD_CORE
  QtUi *gui = new QtUi();
  Client::init(gui);
  // init gui only after the event loop has started
  QTimer::singleShot(0, gui, SLOT(init()));
  //gui->init();
#endif

#ifndef BUILD_QTUI
  if(!args.contains("--norestore")) {
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

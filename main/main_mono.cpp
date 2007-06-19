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

#include <QApplication>

#include "core.h"
#include "style.h"
#include "global.h"
#include "guiproxy.h"
#include "coreproxy.h"
#include "settings.h"
#include "chatwidget.h"

#include "mainwin.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QCoreApplication::setOrganizationDomain("quassel-irc.org");
  QCoreApplication::setApplicationName("Quassel IRC");
  QCoreApplication::setOrganizationName("Quassel IRC Development Team");

  Global::runMode = Global::Monolithic;
  Global::quasselDir = QDir::homePath() + "/.quassel";

  //settings = new Settings();
  //global = new Global();
  guiProxy = new GUIProxy();
  //coreProxy = new CoreProxy();
  QObject::connect(Core::guiSession(), SIGNAL(proxySignal(CoreSignal, QVariant, QVariant, QVariant)), guiProxy, SLOT(recv(CoreSignal, QVariant, QVariant, QVariant)));
  QObject::connect(guiProxy, SIGNAL(send(GUISignal, QVariant, QVariant, QVariant)), Core::guiSession(), SLOT(processSignal(GUISignal, QVariant, QVariant, QVariant)));

  Settings::init();
  Style::init();

  MainWin *mainWin = new MainWin();
  mainWin->show();
  mainWin->init();
  int exitCode = app.exec();
  //delete core;
  Core::destroy();
  delete guiProxy;
  //delete coreProxy;
  //delete global;
  delete mainWin;
  //delete settings;
  return exitCode;
}

void MainWin::syncToCore() {
  //Q_ASSERT(Global::data("CoreReady").toBool());
  coreBuffers = Core::guiSession()->buffers();
  // NOTE: We don't need to request server states, because in the monolithic version there can't be
  //       any servers connected at this stage...
}

/*
void CoreProxy::sendToGUI(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  guiProxy->recv(sig, arg1, arg2, arg3);
}
*/

GUIProxy::GUIProxy() {
  if(guiProxy) qFatal("Trying to instantiate more than one GUIProxy object!");
}

/*
void GUIProxy::send(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  coreProxy->recv(sig, arg1, arg2, arg3);
}
*/

// Dummy function definitions
// These are not needed, since we don't have a network connection to the core.
void GUIProxy::serverHasData() {}
void GUIProxy::connectToCore(QString, quint16) {}
void GUIProxy::disconnectFromCore() {}
void GUIProxy::updateCoreData(QString) {}
void GUIProxy::serverError(QAbstractSocket::SocketError) {}


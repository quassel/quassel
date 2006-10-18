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

#include "quassel.h"
#include "guiproxy.h"

#include "mainwin.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QApplication::setOrganizationDomain("quassel-irc.org");
  QApplication::setApplicationName("Quassel IRC");
  QApplication::setOrganizationName("The Quassel Team");

  Quassel::runMode = Quassel::GUIOnly;
  quassel = Quassel::init();
  guiProxy = GUIProxy::init();

  MainWin mainWin;
  mainWin.show();
  int exitCode = app.exec();
  delete guiProxy;
  delete quassel;
}

void GUIProxy::send(GUISignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {



}

void GUIProxy::recv(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {



}

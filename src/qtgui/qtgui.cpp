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

#include "qtgui.h"

#include "mainwin.h"

QtGui::QtGui() : AbstractUi() {
  mainWin = new MainWin(this);
  connect(mainWin, SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
  connect(mainWin, SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));

}

QtGui::~QtGui() {
  delete mainWin;
}

void QtGui::init() {
  mainWin->init();
}

AbstractUiMsg *QtGui::layoutMsg(const Message &msg) {
  return mainWin->layoutMsg(msg);
}

void QtGui::connectedToCore() {
  mainWin->connectedToCore();
}

void QtGui::disconnectedFromCore() {
  mainWin->disconnectedFromCore();
}

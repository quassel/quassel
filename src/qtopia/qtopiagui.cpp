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

#include "qtopiagui.h"
#include "qtopiamainwin.h"

QtopiaGui::QtopiaGui(QtopiaMainWin *mw) : AbstractUi(), mainWin(mw) {
  connect(mainWin, SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
  connect(mainWin, SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));


}

QtopiaGui::~QtopiaGui() {
  delete mainWin;

}

void QtopiaGui::init() {

}

AbstractUiMsg *QtopiaGui::layoutMsg(const Message &msg) {
  return mainWin->layoutMsg(msg);
}

void QtopiaGui::connectedToCore() {
  mainWin->connectedToCore();
}

void QtopiaGui::disconnectedFromCore() {
  mainWin->disconnectedFromCore();
}

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

#include "qtopiaui.h"
#include "qtopiamainwin.h"
#include "qtopiauistyle.h"

QtopiaUiStyle *QtopiaUi::_style;

QtopiaUi::QtopiaUi(QtopiaMainWin *mw) : AbstractUi(), mainWin(mw) {
  _style = new QtopiaUiStyle();
  connect(mainWin, SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
  connect(mainWin, SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));


}

QtopiaUi::~QtopiaUi() {
  delete _style;
  delete mainWin;

}

void QtopiaUi::init() {

}

QtopiaUiStyle *QtopiaUi::style() {
  return _style;
}

AbstractUiMsg *QtopiaUi::layoutMsg(const Message &msg) {
  return mainWin->layoutMsg(msg);
}

void QtopiaUi::connectedToCore() {
  mainWin->connectedToCore();
}

void QtopiaUi::disconnectedFromCore() {
  mainWin->disconnectedFromCore();
}

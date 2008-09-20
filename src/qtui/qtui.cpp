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

#include "qtui.h"

#include <QDebug>

#include "actioncollection.h"
#include "chatlinemodel.h"
#include "mainwin.h"
#include "qtuimessageprocessor.h"
#include "qtuistyle.h"
#include "uisettings.h"
#include "util.h"

ActionCollection *QtUi::_actionCollection = 0;
QtUiStyle *QtUi::_style = 0;

QtUi::QtUi() : AbstractUi()
{
  if(_style != 0) {
    qWarning() << "QtUi has been instantiated again!";
    return;
  }
  _actionCollection = new ActionCollection(this);

  UiSettings uiSettings;
  loadTranslation(uiSettings.value("Locale", QLocale::system()).value<QLocale>());

  mainWin = new MainWin();
  _style = new QtUiStyle;

  connect(mainWin, SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
  connect(mainWin, SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));
}

QtUi::~QtUi() {
  delete _style;
  delete mainWin;
}

void QtUi::init() {
  mainWin->init();
}

MessageModel *QtUi::createMessageModel(QObject *parent) {
  return new ChatLineModel(parent);
}

AbstractMessageProcessor *QtUi::createMessageProcessor(QObject *parent) {
  return new QtUiMessageProcessor(parent);
}

void QtUi::connectedToCore() {
  mainWin->connectedToCore();
}

void QtUi::disconnectedFromCore() {
  mainWin->disconnectedFromCore();
}

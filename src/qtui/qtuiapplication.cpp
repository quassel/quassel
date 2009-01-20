/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "qtuiapplication.h"

#include <QStringList>

#include "client.h"
#include "cliparser.h"
#include "qtui.h"
#include "sessionsettings.h"

QtUiApplication::QtUiApplication(int &argc, char **argv)
#ifdef HAVE_KDE
  : KApplication(),
#else
  : QApplication(argc, argv),
#endif
    Quassel(),
    _aboutToQuit(false)
{
#ifdef HAVE_KDE
  Q_UNUSED(argc)
  Q_UNUSED(argv)
#endif

  setRunMode(Quassel::ClientOnly);

  qInstallMsgHandler(Client::logMessage);
}

bool QtUiApplication::init() {
  if(Quassel::init()) {
    // session resume
    QtUi *gui = new QtUi();
    Client::init(gui);
    // init gui only after the event loop has started
    // QTimer::singleShot(0, gui, SLOT(init()));
    gui->init();
    resumeSessionIfPossible();
    return true;
  }
  return false;
}

QtUiApplication::~QtUiApplication() {
  Client::destroy();
}

void QtUiApplication::commitData(QSessionManager &manager) {
  _aboutToQuit = true;
}

void QtUiApplication::saveState(QSessionManager & manager) {
  //qDebug() << QString("saving session state to id %1").arg(manager.sessionId());
  AccountId activeCore = Client::currentCoreAccount();
  SessionSettings s(manager.sessionId());
  s.setSessionAge(0);
  emit saveStateToSession(manager.sessionId());
  emit saveStateToSessionSettings(s);
}

void QtUiApplication::resumeSessionIfPossible() {
  // load all sessions
  if(isSessionRestored()) {
    qDebug() << QString("restoring from session %1").arg(sessionId());
    SessionSettings s(sessionId());
    s.sessionAging();
    s.setSessionAge(0);
    emit resumeFromSession(sessionId());
    emit resumeFromSessionSettings(s);
    s.cleanup();
  } else {
    SessionSettings s(QString("1"));
    s.sessionAging();
    s.cleanup();
  }
}



/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include <QStringList>

#include "qtuiapplication.h"
#include "sessionsettings.h"
#include "client.h"

QtUiApplication::QtUiApplication(int &argc, char **argv) : QApplication(argc, argv) {

}

void QtUiApplication::saveState(QSessionManager & manager) {
  //qDebug() << QString("saving session state to id %1").arg(manager.sessionId());
  AccountId activeCore = Client::currentCoreAccount();
  SessionSettings s(manager.sessionId());
  s.setSessionAge(0);
  emit saveStateToSession(manager.sessionId());
  emit saveStateToSessionSettings(s);
}

QtUiApplication::~ QtUiApplication() {
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


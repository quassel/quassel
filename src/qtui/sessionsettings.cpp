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
#include "sessionsettings.h"

#include <QStringList>

void SessionSettings::setValue(const QString &key, const QVariant &data) {
  setLocalValue(QString("%1/%2").arg(_sessionId, key), data);
}

QVariant SessionSettings::value(const QString &key, const QVariant &def) {
  return localValue(QString("%1/%2").arg(_sessionId, key), def);
}

void SessionSettings::removeKey(const QString &key) {
  removeLocalKey(QString("%1/%2").arg(_sessionId, key));
}

void SessionSettings::cleanup() {
  QStringList sessions = localChildGroups();
  QString str;
  SessionSettings s(sessionId());
  foreach(str, sessions) {
    // load session and check age
    s.setSessionId(str);
    if(s.sessionAge() > 3) {
      s.removeSession();
    }
  }
}

int SessionSettings::sessionAge() {
  QVariant val = localValue(QString("%1/_sessionAge").arg(_sessionId), 0);
  bool b = false;
  int i = val.toInt(&b);
  if(b) {
    return i;
  } else {
    // no int saved, delete session
    //qDebug() << QString("deleting invalid session %1 (invalid session age found)").arg(_sessionId);
    removeSession();
  }
  return 10;
}

void SessionSettings::removeSession() {
  QStringList keys = localChildKeys(sessionId());
  foreach(QString k, keys) {
    removeKey(k);
  }
}


SessionSettings::SessionSettings(const QString & sessionId, const QString & group)  : ClientSettings(group), _sessionId(sessionId) {
}

void SessionSettings::setSessionAge(int age) {
  setValue(QString("_sessionAge"),age);
}

void SessionSettings::sessionAging() {
  QStringList sessions = localChildGroups();
  QString str;
  SessionSettings s(sessionId());
  foreach(str, sessions) {
    // load session and check age
    s.setSessionId(str);
    s.setSessionAge(s.sessionAge()+1);
  }
}


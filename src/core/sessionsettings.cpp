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

#include "sessionsettings.h"

#include <QStringList>
#include <QDebug>

SessionSettings::SessionSettings(UserId uid) : CoreSettings("SessionData"), user(uid) {

}

QVariantMap SessionSettings::sessionData() {
  QVariantMap res;
  foreach(QString key, localChildKeys(QString("%1").arg(user))) {
    res[key] = localValue(QString("%1/%2").arg(user).arg(key));
  }
  return res;
}

void SessionSettings::setSessionValue(const QString &key, const QVariant &data) {
  setLocalValue(QString("%1/%2").arg(user).arg(key), data);
}

QVariant SessionSettings::sessionValue(const QString &key, const QVariant &def) {
  return localValue(QString("%1/%2").arg(user).arg(key), def);
}


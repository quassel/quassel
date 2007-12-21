/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include "coresettings.h"

#include <QStringList>

CoreSettings::CoreSettings() : Settings("Core") {
}

CoreSettings::~CoreSettings() {
}

void CoreSettings::setDatabaseSettings(const QVariant &data) {
  setLocalValue("DatabaseSettings", data);
}

QVariant CoreSettings::databaseSettings(const QVariant &def) {
  return localValue("DatabaseSettings", def);
}

void CoreSettings::setPort(const uint &port) {
  setLocalValue("Port", port);
}

uint CoreSettings::port(const uint &def) {
  return localValue("Port", def).toUInt();
}

void CoreSettings::setCoreState(const QVariant &data) {
  setLocalValue("CoreState", data);
}

QVariant CoreSettings::coreState(const QVariant &def) {
  return localValue("CoreState", def);
}

QStringList CoreSettings::sessionKeys() {
  Q_ASSERT(false);
  return QStringList();
}

void CoreSettings::setSessionValue(const QString &key, const QVariant &data) {
  Q_ASSERT(false);
}

QVariant CoreSettings::sessionValue(const QString &key, const QVariant &def) {
  Q_ASSERT(false);
  return QVariant();
}

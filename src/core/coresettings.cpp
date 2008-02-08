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

#include "coresettings.h"

#include <QStringList>

CoreSettings::CoreSettings(const QString group) : Settings(group, Global::coreApplicationName) {
}

CoreSettings::~CoreSettings() {
}

void CoreSettings::setStorageSettings(const QVariant &data) {
  setLocalValue("StorageSettings", data);
}

QVariant CoreSettings::storageSettings(const QVariant &def) {
  return localValue("StorageSettings", def);
}

// FIXME remove
QVariant CoreSettings::oldDbSettings() {
  return localValue("DatabaseSettings");
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

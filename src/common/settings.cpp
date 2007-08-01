/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
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

#include <QSettings>
#include <QStringList>
#include <QDebug>

#include "settings.h"

Settings::Settings(QString g) : QObject(), group(g) {


}

Settings::~Settings() {

}

void Settings::setGroup(QString g) {
  group = g;

}

QStringList Settings::allLocalKeys() {
  QSettings s;
  s.beginGroup(group);
  return s.allKeys();
}

QStringList Settings::localChildKeys() {
  QSettings s;
  s.beginGroup(group);
  return s.childKeys();
}

QStringList Settings::localChildGroups() {
  QSettings s;
  s.beginGroup(group);
  return s.childGroups();
}

void Settings::setLocalValue(const QString &key, const QVariant &data) {
  QSettings s;
  s.beginGroup(group);
  s.setValue(key, data);
}

QVariant Settings::localValue(const QString &key, const QVariant &def) {
  QSettings s;
  s.beginGroup(group);
  return s.value(key, def);
}

void Settings::removeLocalKey(const QString &key) {
  QSettings s;
  s.beginGroup(group);
  s.remove(key);
}


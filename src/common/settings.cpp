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

#include <QSettings>
#include <QStringList>
#include <QDebug>

#ifdef Q_WS_QWS
#include <Qtopia>
#include <QCoreApplication>
#endif

#include "settings.h"

Settings::Settings(QString g) : group(g) {

#ifndef Q_WS_QWS
  QSettings();
#else
  // FIXME sandboxDir() is not currently working correctly...
  //if(Qtopia::sandboxDir().isEmpty()) QSettings();
  //else QSettings(Qtopia::sandboxDir() + "/etc/QuasselIRC.conf", QSettings::NativeFormat);
  // ...so we have to use a workaround:
  QString appPath = QCoreApplication::applicationFilePath();
  if(appPath.startsWith(Qtopia::packagePath())) {
    QString sandboxPath = appPath.left(Qtopia::packagePath().length() + 32);
    QSettings(sandboxPath + "/etc/QuasselIRC.conf", QSettings::IniFormat);
    qDebug() << sandboxPath + "/etc/QuasselIRC.conf";
  } else {
    QSettings();
  }
#endif
}

Settings::~Settings() {

}

void Settings::setGroup(QString g) {
  group = g;

}

QStringList Settings::allLocalKeys() {
  beginGroup(group);
  QStringList res = allKeys();
  endGroup();
  return res;
}

QStringList Settings::localChildKeys() {
  beginGroup(group);
  QStringList res = childKeys();
  endGroup();
  return res;
}

QStringList Settings::localChildGroups() {
  beginGroup(group);
  QStringList res = childGroups();
  endGroup();
  return res;
}

void Settings::setLocalValue(const QString &key, const QVariant &data) {
  beginGroup(group);
  setValue(key, data);
  endGroup();
}

QVariant Settings::localValue(const QString &key, const QVariant &def) {
  beginGroup(group);
  QVariant res = value(key, def);
  endGroup();
  return res;
}

void Settings::removeLocalKey(const QString &key) {
  beginGroup(group);
  remove(key);
  endGroup();
}

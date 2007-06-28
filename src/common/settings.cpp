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

#include "settings.h"

Settings *settings;

void Settings::init() {
  curProfile = QObject::tr("Default");
}
/*
Settings::~Settings() {
  qDebug() << "destructing";

}
*/

void Settings::setProfile(const QString &profile) {
  curProfile = profile;
}

void Settings::setGuiValue(const QString &key, const QVariant &value) {
  QSettings s;
  //s.setValue("GUI/Default/BufferStates/QuakeNet/#quassel/voicedExpanded", true);
  //QString k = QString("GUI/%1/%2").arg(curProfile).arg(key);
  s.setValue(QString("GUI/%1/%2").arg(curProfile).arg(key), value);
}

QVariant Settings::guiValue(const QString &key, const QVariant &defaultValue) {
  QSettings s;
  return s.value(QString("GUI/%1/%2").arg(curProfile).arg(key), defaultValue);
}

void Settings::setCoreValue(const QString &user, const QString &key, const QVariant &value) {
  QSettings s;
  s.setValue(QString("Core/%1/%2").arg(user).arg(key), value);
}

QVariant Settings::coreValue(const QString &user, const QString &key, const QVariant &defaultValue) {
  QSettings s;
  return s.value(QString("Core/%1/%2").arg(user).arg(key), defaultValue);
}

QString Settings::curProfile;

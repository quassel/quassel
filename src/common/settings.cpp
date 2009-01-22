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

#include <QStringList>

#include "settings.h"

const int VERSION = 1;

QHash<QString, QHash<QString, QVariant> > Settings::settingsCache;
QHash<QString, QHash<QString, SettingsChangeNotifier *> > Settings::settingsChangeNotifier;

#ifdef Q_WS_MAC
#  define create_qsettings QSettings s(QCoreApplication::organizationDomain(), appName)
#else
#  define create_qsettings QSettings s(fileName(), format())
#endif

// Settings::Settings(QString group_, QString appName_)
//   : group(group_),
//     appName(appName_)
// {

// /* we need to call the constructor immediately in order to set the path...
// #ifndef Q_WS_QWS
//   QSettings(QCoreApplication::organizationName(), applicationName);
// #else
//   // FIXME sandboxDir() is not currently working correctly...
//   //if(Qtopia::sandboxDir().isEmpty()) QSettings();
//   //else QSettings(Qtopia::sandboxDir() + "/etc/QuasselIRC.conf", QSettings::NativeFormat);
//   // ...so we have to use a workaround:
//   QString appPath = QCoreApplication::applicationFilePath();
//   if(appPath.startsWith(Qtopia::packagePath())) {
//     QString sandboxPath = appPath.left(Qtopia::packagePath().length() + 32);
//     QSettings(sandboxPath + "/etc/QuasselIRC.conf", QSettings::IniFormat);
//     qDebug() << sandboxPath + "/etc/QuasselIRC.conf";
//   } else {
//     QSettings(QCoreApplication::organizationName(), applicationName);
//   }
// #endif
// */
// }

void Settings::notify(const QString &key, QObject *receiver, const char *slot) {
  QObject::connect(notifier(group, key), SIGNAL(valueChanged(const QVariant &)),
		   receiver, slot);
}

uint Settings::version() {
  // we don't cache this value, and we ignore the group
  create_qsettings;
  uint ver = s.value("Config/Version", 0).toUInt();
  if(!ver) {
    // No version, so create one
    s.setValue("Config/Version", VERSION);
    return VERSION;
  }
  return ver;
}

QStringList Settings::allLocalKeys() {
  create_qsettings;
  s.beginGroup(group);
  QStringList res = s.allKeys();
  s.endGroup();
  return res;
}

QStringList Settings::localChildKeys(const QString &rootkey) {
  QString g;
  if(rootkey.isEmpty())
    g = group;
  else
    g = QString("%1/%2").arg(group, rootkey);

  create_qsettings;
  s.beginGroup(g);
  QStringList res = s.childKeys();
  s.endGroup();
  return res;
}

QStringList Settings::localChildGroups(const QString &rootkey) {
  QString g;
  if(rootkey.isEmpty())
    g = group;
  else
    g = QString("%1/%2").arg(group, rootkey);

  create_qsettings;
  s.beginGroup(g);
  QStringList res = s.childGroups();
  s.endGroup();
  return res;
}

void Settings::setLocalValue(const QString &key, const QVariant &data) {
  create_qsettings;
  s.beginGroup(group);
  s.setValue(key, data);
  s.endGroup();
  setCacheValue(group, key, data);
  if(hasNotifier(group, key)) {
    emit notifier(group, key)->valueChanged(data);
  }
}

const QVariant &Settings::localValue(const QString &key, const QVariant &def) {
  if(!isCached(group, key)) {
    create_qsettings;
    s.beginGroup(group);
    setCacheValue(group, key, s.value(key, def));
    s.endGroup();
  }
  return cacheValue(group, key);
}

void Settings::removeLocalKey(const QString &key) {
  create_qsettings;
  s.beginGroup(group);
  s.remove(key);
  s.endGroup();
  if(isCached(group, key))
    settingsCache[group].remove(key);
}

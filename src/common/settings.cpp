/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QStringList>

#include "settings.h"

const int VERSION = 1;

QHash<QString, QVariant> Settings::settingsCache;
QHash<QString, SettingsChangeNotifier *> Settings::settingsChangeNotifier;

#ifdef Q_OS_MAC
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

void Settings::notify(const QString &key, QObject *receiver, const char *slot)
{
    QObject::connect(notifier(normalizedKey(group, key)), SIGNAL(valueChanged(const QVariant &)),
        receiver, slot);
}


void Settings::initAndNotify(const QString &key, QObject *receiver, const char *slot, const QVariant &defaultValue)
{
    notify(key, receiver, slot);
    emit notifier(normalizedKey(group, key))->valueChanged(localValue(key, defaultValue));
}


uint Settings::version()
{
    // we don't cache this value, and we ignore the group
    create_qsettings;
    uint ver = s.value("Config/Version", 0).toUInt();
    if (!ver) {
        // No version, so create one
        s.setValue("Config/Version", VERSION);
        return VERSION;
    }
    return ver;
}


QStringList Settings::allLocalKeys()
{
    create_qsettings;
    s.beginGroup(group);
    QStringList res = s.allKeys();
    s.endGroup();
    return res;
}


QStringList Settings::localChildKeys(const QString &rootkey)
{
    QString g;
    if (rootkey.isEmpty())
        g = group;
    else
        g = QString("%1/%2").arg(group, rootkey);

    create_qsettings;
    s.beginGroup(g);
    QStringList res = s.childKeys();
    s.endGroup();
    return res;
}


QStringList Settings::localChildGroups(const QString &rootkey)
{
    QString g;
    if (rootkey.isEmpty())
        g = group;
    else
        g = QString("%1/%2").arg(group, rootkey);

    create_qsettings;
    s.beginGroup(g);
    QStringList res = s.childGroups();
    s.endGroup();
    return res;
}


void Settings::setLocalValue(const QString &key, const QVariant &data)
{
    QString normKey = normalizedKey(group, key);
    create_qsettings;
    s.setValue(normKey, data);
    setCacheValue(normKey, data);
    if (hasNotifier(normKey)) {
        emit notifier(normKey)->valueChanged(data);
    }
}


const QVariant &Settings::localValue(const QString &key, const QVariant &def)
{
    QString normKey = normalizedKey(group, key);
    if (!isCached(normKey)) {
        create_qsettings;
        setCacheValue(normKey, s.value(normKey, def));
    }
    return cacheValue(normKey);
}


void Settings::removeLocalKey(const QString &key)
{
    create_qsettings;
    s.beginGroup(group);
    s.remove(key);
    s.endGroup();
    QString normKey = normalizedKey(group, key);
    if (isCached(normKey))
        settingsCache.remove(normKey);
}

/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

const int VERSION = 1;              /// Settings version for backwords/forwards incompatible changes

// This is used if no VersionMinor key exists, e.g. upgrading from a Quassel version before this
// change.  This shouldn't be increased from 1; instead, change the logic in Core::Core() and
// QtUiApplication::init() to handle upgrading and downgrading.
const int VERSION_MINOR_INITIAL = 1; /// Initial settings version for compatible changes

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


uint Settings::versionMinor()
{
    // Don't cache this value; ignore the group
    create_qsettings;
    // '0' means new configuration, anything else indicates an existing configuration.  Application
    // initialization should check this value and manage upgrades/downgrades, e.g. in Core::Core()
    // and QtUiApplication::init().
    uint verMinor = s.value("Config/VersionMinor", 0).toUInt();

    // As previous Quassel versions didn't implement this, we need to check if any settings other
    // than Config/Version exist.  If so, assume it's version 1.
    if (verMinor == 0 && s.allKeys().count() > 1) {
        // More than 1 key exists, but version's never been set.  Assume and set version 1.
        setVersionMinor(VERSION_MINOR_INITIAL);
        return VERSION_MINOR_INITIAL;
    } else {
        return verMinor;
    }
}


void Settings::setVersionMinor(const uint versionMinor)
{
    // Don't cache this value; ignore the group
    create_qsettings;
    // Set the value directly.
    s.setValue("Config/VersionMinor", versionMinor);
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

bool Settings::localKeyExists(const QString &key)
{
    QString normKey = normalizedKey(group, key);
    if (isCached(normKey))
        return true;

    create_qsettings;
    return s.contains(normKey);
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

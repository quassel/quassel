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

const int VERSION = 1;              /// Settings version for backwords/forwards incompatible changes

// This is used if no VersionMinor key exists, e.g. upgrading from a Quassel version before this
// change.  This shouldn't be increased from 1; instead, change the logic in Core::Core() and
// QtUiApplication::init() to handle upgrading and downgrading.
const int VERSION_MINOR_INITIAL = 1; /// Initial settings version for compatible changes

QHash<QString, QVariant> Settings::_settingsCache;
QHash<QString, bool> Settings::_settingsKeyPersistedCache;
QHash<QString, std::shared_ptr<SettingsChangeNotifier>> Settings::_settingsChangeNotifier;

#ifdef Q_OS_MAC
#  define create_qsettings QSettings s(QCoreApplication::organizationDomain(), _appName)
#else
#  define create_qsettings QSettings s(fileName(), format())
#endif

Settings::Settings(QString group, QString appName)
    : _group(std::move(group)), _appName(std::move(appName))
{}


void Settings::setGroup(QString group) {
    _group = std::move(group);
}


void Settings::notify(const QString &key, QObject *receiver, const char *slot) const
{
    QObject::connect(notifier(normalizedKey(_group, key)), SIGNAL(valueChanged(const QVariant &)),
        receiver, slot);
}


void Settings::initAndNotify(const QString &key, QObject *receiver, const char *slot, const QVariant &defaultValue) const
{
    notify(key, receiver, slot);
    emit notifier(normalizedKey(_group, key))->valueChanged(localValue(key, defaultValue));
}


uint Settings::version() const
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


uint Settings::versionMinor() const
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
        // const_cast is ok, because setVersionMinor() doesn't actually change this instance
        const_cast<Settings*>(this)->setVersionMinor(VERSION_MINOR_INITIAL);
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


QSettings::Format Settings::format() const
{
#ifdef Q_OS_WIN
    return QSettings::IniFormat;
#else
    return QSettings::NativeFormat;
#endif
}


bool Settings::sync()
{
    create_qsettings;
    s.sync();
    switch (s.status()) {
        case QSettings::NoError:
            return true;
        default:
            return false;
    }
}


bool Settings::isWritable() const
{
    create_qsettings;
    return s.isWritable();
}


QStringList Settings::allLocalKeys() const
{
    create_qsettings;
    s.beginGroup(_group);
    QStringList res = s.allKeys();
    s.endGroup();
    return res;
}


QStringList Settings::localChildKeys(const QString &rootkey) const
{
    QString g;
    if (rootkey.isEmpty())
        g = _group;
    else
        g = QString("%1/%2").arg(_group, rootkey);

    create_qsettings;
    s.beginGroup(g);
    QStringList res = s.childKeys();
    s.endGroup();
    return res;
}


QStringList Settings::localChildGroups(const QString &rootkey) const
{
    QString g;
    if (rootkey.isEmpty())
        g = _group;
    else
        g = QString("%1/%2").arg(_group, rootkey);

    create_qsettings;
    s.beginGroup(g);
    QStringList res = s.childGroups();
    s.endGroup();
    return res;
}


void Settings::setLocalValue(const QString &key, const QVariant &data)
{
    QString normKey = normalizedKey(_group, key);
    create_qsettings;
    s.setValue(normKey, data);
    setCacheKeyPersisted(normKey, true);
    setCacheValue(normKey, data);
    if (hasNotifier(normKey)) {
        emit notifier(normKey)->valueChanged(data);
    }
}


QVariant Settings::localValue(const QString &key, const QVariant &def) const
{
    QString normKey = normalizedKey(_group, key);
    if (!isCached(normKey)) {
        create_qsettings;
        // Since we're loading from settings anyways, cache whether or not the key exists on disk
        setCacheKeyPersisted(normKey, s.contains(normKey));
        // Cache key value
        setCacheValue(normKey, s.value(normKey, def));
    }
    if (cacheKeyPersisted(normKey)) {
        return cacheValue(normKey);
    }
    // Don't return possibly wrong cached values
    // A key gets cached with the first default value requested and never changes afterwards
    return def;
}


bool Settings::localKeyExists(const QString &key) const
{
    QString normKey = normalizedKey(_group, key);
    if (!isKeyPersistedCached(normKey)) {
        create_qsettings;
        // Cache whether or not key exists on disk
        // We can't cache key value as we don't know the default
        setCacheKeyPersisted(normKey, s.contains(normKey));
    }

    return cacheKeyPersisted(normKey);
}


void Settings::removeLocalKey(const QString &key)
{
    create_qsettings;
    s.beginGroup(_group);
    s.remove(key);
    s.endGroup();
    QString normKey = normalizedKey(_group, key);
    if (isCached(normKey)) {
        _settingsCache.remove(normKey);
    }
    if (isKeyPersistedCached(normKey)) {
        _settingsKeyPersistedCache.remove(normKey);
    }
    if (hasNotifier(normKey)) {
        emit notifier(normKey)->valueChanged({});
    }
}


QString Settings::fileName() const
{
    return Quassel::configDirPath() + _appName
           + ((format() == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
}


QString Settings::normalizedKey(const QString &group, const QString &key) const
{
    if (group.isEmpty())
        return key;
    return group + '/' + key;
}


void Settings::setCacheKeyPersisted(const QString &normKey, bool exists) const
{
    _settingsKeyPersistedCache[normKey] = exists;
}


bool Settings::cacheKeyPersisted(const QString &normKey) const
{
    return _settingsKeyPersistedCache[normKey];
}


bool Settings::isKeyPersistedCached(const QString &normKey) const
{
    return _settingsKeyPersistedCache.contains(normKey);
}


void Settings::setCacheValue(const QString &normKey, const QVariant &data) const
{
    _settingsCache[normKey] = data;
}


QVariant Settings::cacheValue(const QString &normKey) const
{
    return _settingsCache[normKey];
}


bool Settings::isCached(const QString &normKey) const
{
    return _settingsCache.contains(normKey);
}


SettingsChangeNotifier *Settings::notifier(const QString &normKey) const
{
    if (!hasNotifier(normKey))
        _settingsChangeNotifier[normKey] = std::make_shared<SettingsChangeNotifier>();
    return _settingsChangeNotifier[normKey].get();
}


bool Settings::hasNotifier(const QString &normKey) const
{
    return _settingsChangeNotifier.contains(normKey);
}

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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCoreApplication>
#include <QHash>
#include <QSettings>
#include <QString>
#include <QVariant>

#include "quassel.h"

class SettingsChangeNotifier : public QObject
{
    Q_OBJECT

signals:
    void valueChanged(const QVariant &newValue);

private:
    friend class Settings;
};


class Settings
{
public:
    enum Mode { Default, Custom };

public:
    //! Call the given slot on change of the given key
    virtual void notify(const QString &key, QObject *receiver, const char *slot);

    //! Sets up notification and calls the given slot to set the initial value
    void initAndNotify(const QString &key, QObject *receiver, const char *slot, const QVariant &defaultValue = QVariant());

    /**
     * Get the major configuration version
     *
     * This indicates the backwards/forwards incompatible version of configuration.
     *
     * @return Major configuration version (the X in XX.YY)
     */
    virtual uint version();

    /**
     * Get the minor configuration version
     *
     * This indicates the backwards/forwards compatible version of configuration.
     *
     * @see Settings::setVersionMinor()
     * @return Minor configuration version (the Y in XX.YY)
     */
    virtual uint versionMinor();

    /**
     * Set the minor configuration version
     *
     * When making backwards/forwards compatible changes, call this with the new version number.
     * This does not implement any upgrade logic; implement that when checking Settings::version(),
     * e.g. in Core::Core() and QtUiApplication::init().
     *
     * @param[in] versionMinor New minor version number
     */
    virtual void setVersionMinor(const uint versionMinor);

    /**
     * Persist unsaved changes to permanent storage
     *
     * @return true if succeeded, false otherwise
     */
    bool sync();

    /**
     * Check if the configuration storage is writable.
     *
     * @return true if writable, false otherwise
     */
    bool isWritable();

protected:
    inline Settings(QString group_, QString appName_) : group(group_), appName(appName_) {}
    inline virtual ~Settings() {}

    inline void setGroup(const QString &group_) { group = group_; }

    virtual QStringList allLocalKeys();
    virtual QStringList localChildKeys(const QString &rootkey = QString());
    virtual QStringList localChildGroups(const QString &rootkey = QString());

    virtual void setLocalValue(const QString &key, const QVariant &data);
    virtual const QVariant &localValue(const QString &key, const QVariant &def = QVariant());

    /**
     * Gets if a key exists in settings
     *
     * @param[in] key ID of local settings key
     * @returns True if key exists in settings, otherwise false
     */
    virtual bool localKeyExists(const QString &key);

    virtual void removeLocalKey(const QString &key);

    QString group;
    QString appName;

private:
    inline QSettings::Format format()
    {
#ifdef Q_OS_WIN
        return QSettings::IniFormat;
#else
        return QSettings::NativeFormat;
#endif
    }


    inline QString fileName()
    {
        return Quassel::configDirPath() + appName
               + ((format() == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
    }


    static QHash<QString, QVariant> settingsCache;         ///< Cached settings values
    static QHash<QString, bool> settingsKeyPersistedCache; ///< Cached settings key exists on disk
    static QHash<QString, SettingsChangeNotifier *> settingsChangeNotifier;

    inline QString normalizedKey(const QString &group, const QString &key)
    {
        if (group.isEmpty())
            return key;
        return group + '/' + key;
    }


    /**
     * Update the cache of whether or not a given settings key persists on disk
     *
     * @param normKey Normalized settings key ID
     * @param exists  True if key exists, otherwise false
     */
    inline void setCacheKeyPersisted(const QString &normKey, bool exists)
    {
        settingsKeyPersistedCache[normKey] = exists;
    }


    /**
     * Check if the given settings key ID persists on disk (rather than being a default value)
     *
     * @see Settings::localKeyExists()
     *
     * @param normKey Normalized settings key ID
     * @return True if key exists and persistence has been cached, otherwise false
     */
    inline const bool &cacheKeyPersisted(const QString &normKey)
    {
        return settingsKeyPersistedCache[normKey];
    }


    /**
     * Check if the persistence of the given settings key ID has been cached
     *
     * @param normKey Normalized settings key ID
     * @return True if key persistence has been cached, otherwise false
     */
    inline bool isKeyPersistedCached(const QString &normKey)
    {
        return settingsKeyPersistedCache.contains(normKey);
    }


    inline void setCacheValue(const QString &normKey, const QVariant &data)
    {
        settingsCache[normKey] = data;
    }


    inline const QVariant &cacheValue(const QString &normKey)
    {
        return settingsCache[normKey];
    }


    inline bool isCached(const QString &normKey)
    {
        return settingsCache.contains(normKey);
    }


    inline SettingsChangeNotifier *notifier(const QString &normKey)
    {
        if (!hasNotifier(normKey))
            settingsChangeNotifier[normKey] = new SettingsChangeNotifier();
        return settingsChangeNotifier[normKey];
    }


    inline bool hasNotifier(const QString &normKey)
    {
        return settingsChangeNotifier.contains(normKey);
    }
};


#endif

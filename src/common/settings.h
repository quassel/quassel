/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#pragma once

#include "common-export.h"

#include <memory>
#include <type_traits>
#include <utility>

#include <QCoreApplication>
#include <QHash>
#include <QSettings>
#include <QString>
#include <QVariant>

#include "quassel.h"

class COMMON_EXPORT SettingsChangeNotifier : public QObject
{
    Q_OBJECT

signals:
    void valueChanged(const QVariant& newValue);

private:
    friend class Settings;
};

class COMMON_EXPORT Settings
{
public:
    enum Mode
    {
        Default,
        Custom
    };

public:
    //! Calls the given slot on change of the given key
    template<typename Receiver, typename Slot>
    void notify(const QString& key, const Receiver* receiver, Slot slot) const
    {
        static_assert(!std::is_same<Slot, const char*>::value, "Old-style slots not supported");
        QObject::connect(notifier(normalizedKey(_group, keyForNotify(key))), &SettingsChangeNotifier::valueChanged, receiver, slot);
    }

    //! Sets up notification and calls the given slot to set the initial value
    template<typename Receiver, typename Slot>
    void initAndNotify(const QString& key, const Receiver* receiver, Slot slot, const QVariant& defaultValue = {}) const
    {
        notify(key, receiver, std::move(slot));
        auto notifyKey = keyForNotify(key);
        emit notifier(normalizedKey(_group, notifyKey))->valueChanged(localValue(notifyKey, defaultValue));
    }

    /**
     * Get the major configuration version
     *
     * This indicates the backwards/forwards incompatible version of configuration.
     *
     * @return Major configuration version (the X in XX.YY)
     */
    virtual uint version() const;

    /**
     * Get the minor configuration version
     *
     * This indicates the backwards/forwards compatible version of configuration.
     *
     * @see Settings::setVersionMinor()
     * @return Minor configuration version (the Y in XX.YY)
     */
    virtual uint versionMinor() const;

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
    bool isWritable() const;

protected:
    Settings(QString group, QString appName);
    virtual ~Settings() = default;

    void setGroup(QString group);

    /**
     * Allows subclasses to transform the key given to notify().
     *
     * Default implementation just returns the given key.
     *
     * @param key Key given to notify()
     * @returns Key that should be used for notifcation
     */
    virtual QString keyForNotify(const QString& key) const;

    virtual QStringList allLocalKeys() const;
    virtual QStringList localChildKeys(const QString& rootkey = QString()) const;
    virtual QStringList localChildGroups(const QString& rootkey = QString()) const;

    virtual void setLocalValue(const QString& key, const QVariant& data);
    virtual QVariant localValue(const QString& key, const QVariant& def = QVariant()) const;

    /**
     * Gets if a key exists in settings
     *
     * @param[in] key ID of local settings key
     * @returns True if key exists in settings, otherwise false
     */
    virtual bool localKeyExists(const QString& key) const;

    virtual void removeLocalKey(const QString& key);

    QString _group;
    QString _appName;

private:
    QSettings::Format format() const;

    QString fileName() const;

    QString normalizedKey(const QString& group, const QString& key) const;

    /**
     * Update the cache of whether or not a given settings key persists on disk
     *
     * @param normKey Normalized settings key ID
     * @param exists  True if key exists, otherwise false
     */
    void setCacheKeyPersisted(const QString& normKey, bool exists) const;

    /**
     * Check if the given settings key ID persists on disk (rather than being a default value)
     *
     * @see Settings::localKeyExists()
     *
     * @param normKey Normalized settings key ID
     * @return True if key exists and persistence has been cached, otherwise false
     */
    bool cacheKeyPersisted(const QString& normKey) const;

    /**
     * Check if the persistence of the given settings key ID has been cached
     *
     * @param normKey Normalized settings key ID
     * @return True if key persistence has been cached, otherwise false
     */
    bool isKeyPersistedCached(const QString& normKey) const;

    void setCacheValue(const QString& normKey, const QVariant& data) const;

    QVariant cacheValue(const QString& normKey) const;

    bool isCached(const QString& normKey) const;

    SettingsChangeNotifier* notifier(const QString& normKey) const;

    bool hasNotifier(const QString& normKey) const;

private:
    static QHash<QString, QVariant> _settingsCache;          ///< Cached settings values
    static QHash<QString, bool> _settingsKeyPersistedCache;  ///< Cached settings key exists on disk
    static QHash<QString, std::shared_ptr<SettingsChangeNotifier>> _settingsChangeNotifier;
};

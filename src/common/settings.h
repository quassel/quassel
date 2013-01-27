/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

    virtual uint version();

protected:
    inline Settings(QString group_, QString appName_) : group(group_), appName(appName_) {}
    inline virtual ~Settings() {}

    inline void setGroup(const QString &group_) { group = group_; }

    virtual QStringList allLocalKeys();
    virtual QStringList localChildKeys(const QString &rootkey = QString());
    virtual QStringList localChildGroups(const QString &rootkey = QString());

    virtual void setLocalValue(const QString &key, const QVariant &data);
    virtual const QVariant &localValue(const QString &key, const QVariant &def = QVariant());

    virtual void removeLocalKey(const QString &key);

    QString group;
    QString appName;

private:
    inline QSettings::Format format()
    {
#ifdef Q_WS_WIN
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


    static QHash<QString, QVariant> settingsCache;
    static QHash<QString, SettingsChangeNotifier *> settingsChangeNotifier;

    inline QString normalizedKey(const QString &group, const QString &key)
    {
        if (group.isEmpty())
            return key;
        return group + '/' + key;
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

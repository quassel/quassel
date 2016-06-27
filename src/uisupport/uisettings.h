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

#ifndef UISETTINGS_H
#define UISETTINGS_H

#include "clientsettings.h"
#include "uistyle.h"

class UiSettings : public ClientSettings
{
public:
    UiSettings(const QString &group = "Ui");

    virtual inline void setValue(const QString &key, const QVariant &data) { setLocalValue(key, data); }
    virtual inline QVariant value(const QString &key, const QVariant &def = QVariant()) { return localValue(key, def); }

    /**
     * Gets if a value exists in settings
     *
     * @param[in] key ID of local settings key
     * @returns True if key exists in settings, otherwise false
     */
    virtual inline bool valueExists(const QString &key) { return localKeyExists(key); }

    inline void remove(const QString &key) { removeLocalKey(key); }
};


class UiStyleSettings : public UiSettings
{
public:
    UiStyleSettings();
    UiStyleSettings(const QString &subGroup);

    void setCustomFormat(UiStyle::FormatType, QTextCharFormat);
    QTextCharFormat customFormat(UiStyle::FormatType);

    void removeCustomFormat(UiStyle::FormatType);
    QList<UiStyle::FormatType> availableFormats();
};


class SessionSettings : public UiSettings
{
public:
    SessionSettings(const QString &sessionId, const QString &group = "Session");

    virtual void setValue(const QString &key, const QVariant &data);
    virtual QVariant value(const QString &key, const QVariant &def = QVariant());

    void removeKey(const QString &key);
    void removeSession();

    void cleanup();
    void sessionAging();

    int sessionAge();
    void setSessionAge(int age);
    inline const QString sessionId() { return _sessionId; };
    inline void setSessionId(const QString &sessionId) { _sessionId = sessionId; }

private:
    QString _sessionId;
};


class ShortcutSettings : public UiSettings
{
public:
    ShortcutSettings();

    //! Remove all stored shortcuts
    void clear();

    QStringList savedShortcuts();

    void saveShortcut(const QString &name, const QKeySequence &shortcut);
    QKeySequence loadShortcut(const QString &name);
};


#endif

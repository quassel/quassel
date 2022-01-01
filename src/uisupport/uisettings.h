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

#include "uisupport-export.h"

#include "clientsettings.h"
#include "uistyle.h"

class UISUPPORT_EXPORT UiSettings : public ClientSettings
{
public:
    UiSettings(QString group = "Ui");

    virtual void setValue(const QString& key, const QVariant& data);
    virtual QVariant value(const QString& key, const QVariant& def = {}) const;

    /**
     * Gets if a value exists in settings
     *
     * @param[in] key ID of local settings key
     * @returns True if key exists in settings, otherwise false
     */
    bool valueExists(const QString& key) const;

    void remove(const QString& key);
};

class UISUPPORT_EXPORT UiStyleSettings : public UiSettings
{
public:
    UiStyleSettings();
    UiStyleSettings(const QString& subGroup);

    void setCustomFormat(UiStyle::FormatType, const QTextCharFormat& format);
    QTextCharFormat customFormat(UiStyle::FormatType) const;

    void removeCustomFormat(UiStyle::FormatType);
    QList<UiStyle::FormatType> availableFormats() const;
};

class UISUPPORT_EXPORT SessionSettings : public UiSettings
{
public:
    SessionSettings(QString sessionId, QString group = "Session");

    void setValue(const QString& key, const QVariant& data) override;
    QVariant value(const QString& key, const QVariant& def = {}) const override;

    void removeKey(const QString& key);
    void removeSession();

    void cleanup();
    void sessionAging();

    int sessionAge();
    void setSessionAge(int age);
    QString sessionId() const;
    void setSessionId(QString sessionId);

private:
    QString _sessionId;
};

class UISUPPORT_EXPORT ShortcutSettings : public UiSettings
{
public:
    ShortcutSettings();

    //! Remove all stored shortcuts
    void clear();

    QStringList savedShortcuts() const;

    void saveShortcut(const QString& name, const QKeySequence& shortcut);
    QKeySequence loadShortcut(const QString& name) const;
};

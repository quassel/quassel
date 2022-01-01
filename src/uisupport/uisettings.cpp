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

#include "uisettings.h"

#include <utility>

#include "action.h"
#include "actioncollection.h"

UiSettings::UiSettings(QString group)
    : ClientSettings(std::move(group))
{}

void UiSettings::setValue(const QString& key, const QVariant& data)
{
    setLocalValue(key, data);
}

QVariant UiSettings::value(const QString& key, const QVariant& def) const
{
    return localValue(key, def);
}

bool UiSettings::valueExists(const QString& key) const
{
    return localKeyExists(key);
}

void UiSettings::remove(const QString& key)
{
    removeLocalKey(key);
}

/**************************************************************************/

UiStyleSettings::UiStyleSettings()
    : UiSettings("UiStyle")
{}

UiStyleSettings::UiStyleSettings(const QString& subGroup)
    : UiSettings(QString("UiStyle/%1").arg(subGroup))
{}

void UiStyleSettings::setCustomFormat(UiStyle::FormatType ftype, const QTextCharFormat& format)
{
    setLocalValue(QString("Format/%1").arg(static_cast<quint32>(ftype)), format);
}

QTextCharFormat UiStyleSettings::customFormat(UiStyle::FormatType ftype) const
{
    return localValue(QString("Format/%1").arg(static_cast<quint32>(ftype)), QTextFormat()).value<QTextFormat>().toCharFormat();
}

void UiStyleSettings::removeCustomFormat(UiStyle::FormatType ftype)
{
    removeLocalKey(QString("Format/%1").arg(static_cast<quint32>(ftype)));
}

QList<UiStyle::FormatType> UiStyleSettings::availableFormats() const
{
    QList<UiStyle::FormatType> formats;
    QStringList list = localChildKeys("Format");
    foreach (QString type, list) {
        formats << (UiStyle::FormatType)type.toInt();
    }
    return formats;
}

/**************************************************************************
 * SessionSettings
 **************************************************************************/

SessionSettings::SessionSettings(QString sessionId, QString group)
    : UiSettings(std::move(group))
    , _sessionId(std::move(sessionId))
{}

void SessionSettings::setValue(const QString& key, const QVariant& data)
{
    setLocalValue(QString("%1/%2").arg(_sessionId, key), data);
}

QVariant SessionSettings::value(const QString& key, const QVariant& def) const
{
    return localValue(QString("%1/%2").arg(_sessionId, key), def);
}

void SessionSettings::removeKey(const QString& key)
{
    removeLocalKey(QString("%1/%2").arg(_sessionId, key));
}

void SessionSettings::cleanup()
{
    QStringList sessions = localChildGroups();
    QString str;
    SessionSettings s(sessionId());
    foreach (str, sessions) {
        // load session and check age
        s.setSessionId(str);
        if (s.sessionAge() > 3) {
            s.removeSession();
        }
    }
}

QString SessionSettings::sessionId() const
{
    return _sessionId;
}

void SessionSettings::setSessionId(QString sessionId)
{
    _sessionId = std::move(sessionId);
}

int SessionSettings::sessionAge()
{
    QVariant val = localValue(QString("%1/_sessionAge").arg(_sessionId), 0);
    bool b = false;
    int i = val.toInt(&b);
    if (b) {
        return i;
    }
    else {
        // no int saved, delete session
        // qDebug() << QString("deleting invalid session %1 (invalid session age found)").arg(_sessionId);
        removeSession();
    }
    return 10;
}

void SessionSettings::removeSession()
{
    QStringList keys = localChildKeys(sessionId());
    foreach (QString k, keys) {
        removeKey(k);
    }
}

void SessionSettings::setSessionAge(int age)
{
    setValue(QString("_sessionAge"), age);
}

void SessionSettings::sessionAging()
{
    QStringList sessions = localChildGroups();
    QString str;
    SessionSettings s(sessionId());
    foreach (str, sessions) {
        // load session and check age
        s.setSessionId(str);
        s.setSessionAge(s.sessionAge() + 1);
    }
}

/**************************************************************************
 * ShortcutSettings
 **************************************************************************/

ShortcutSettings::ShortcutSettings()
    : UiSettings("Shortcuts")
{}

void ShortcutSettings::clear()
{
    for (auto&& key : allLocalKeys()) {
        removeLocalKey(key);
    }
}

QStringList ShortcutSettings::savedShortcuts() const
{
    return localChildKeys();
}

QKeySequence ShortcutSettings::loadShortcut(const QString& name) const
{
    return localValue(name, QKeySequence()).value<QKeySequence>();
}

void ShortcutSettings::saveShortcut(const QString& name, const QKeySequence& seq)
{
    setLocalValue(name, seq);
}

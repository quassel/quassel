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

#include "uisettings.h"

#include "action.h"
#include "actioncollection.h"

UiSettings::UiSettings(const QString &group)
    : ClientSettings(group)
{
}


/**************************************************************************/

UiStyleSettings::UiStyleSettings() : UiSettings("UiStyle") {}
UiStyleSettings::UiStyleSettings(const QString &subGroup) : UiSettings(QString("UiStyle/%1").arg(subGroup))
{
}


void UiStyleSettings::setCustomFormat(UiStyle::FormatType ftype, QTextCharFormat format)
{
    setLocalValue(QString("Format/%1").arg(ftype), format);
}


QTextCharFormat UiStyleSettings::customFormat(UiStyle::FormatType ftype)
{
    return localValue(QString("Format/%1").arg(ftype), QTextFormat()).value<QTextFormat>().toCharFormat();
}


void UiStyleSettings::removeCustomFormat(UiStyle::FormatType ftype)
{
    removeLocalKey(QString("Format/%1").arg(ftype));
}


QList<UiStyle::FormatType> UiStyleSettings::availableFormats()
{
    QList<UiStyle::FormatType> formats;
    QStringList list = localChildKeys("Format");
    foreach(QString type, list) {
        formats << (UiStyle::FormatType)type.toInt();
    }
    return formats;
}


/**************************************************************************
 * SessionSettings
 **************************************************************************/

SessionSettings::SessionSettings(const QString &sessionId, const QString &group)
    : UiSettings(group), _sessionId(sessionId)
{
}


void SessionSettings::setValue(const QString &key, const QVariant &data)
{
    setLocalValue(QString("%1/%2").arg(_sessionId, key), data);
}


QVariant SessionSettings::value(const QString &key, const QVariant &def)
{
    return localValue(QString("%1/%2").arg(_sessionId, key), def);
}


void SessionSettings::removeKey(const QString &key)
{
    removeLocalKey(QString("%1/%2").arg(_sessionId, key));
}


void SessionSettings::cleanup()
{
    QStringList sessions = localChildGroups();
    QString str;
    SessionSettings s(sessionId());
    foreach(str, sessions) {
        // load session and check age
        s.setSessionId(str);
        if (s.sessionAge() > 3) {
            s.removeSession();
        }
    }
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
        //qDebug() << QString("deleting invalid session %1 (invalid session age found)").arg(_sessionId);
        removeSession();
    }
    return 10;
}


void SessionSettings::removeSession()
{
    QStringList keys = localChildKeys(sessionId());
    foreach(QString k, keys) {
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
    foreach(str, sessions) {
        // load session and check age
        s.setSessionId(str);
        s.setSessionAge(s.sessionAge()+1);
    }
}


/**************************************************************************
 * ShortcutSettings
 **************************************************************************/

ShortcutSettings::ShortcutSettings() : UiSettings("Shortcuts")
{
}


void ShortcutSettings::clear()
{
    foreach(const QString &key, allLocalKeys())
    removeLocalKey(key);
}


QStringList ShortcutSettings::savedShortcuts()
{
    return localChildKeys();
}


QKeySequence ShortcutSettings::loadShortcut(const QString &name)
{
    return localValue(name, QKeySequence()).value<QKeySequence>();
}


void ShortcutSettings::saveShortcut(const QString &name, const QKeySequence &seq)
{
    setLocalValue(name, seq);
}

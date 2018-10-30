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

#include "identity.h"

#include <QMetaProperty>
#include <QString>
#include <QVariantMap>

#ifdef Q_OS_MAC
#    include <CoreServices/CoreServices.h>

#    include "mac_utils.h"
#endif

#ifdef Q_OS_UNIX
#    include <pwd.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

#ifdef Q_OS_WIN
#    include <windows.h>
#    include <Winbase.h>
#    define SECURITY_WIN32
#    include <Security.h>
#endif

Identity::Identity(IdentityId id, QObject* parent)
    : SyncableObject(parent)
    , _identityId(id)
{
    init();
    setToDefaults();
}

Identity::Identity(const Identity& other, QObject* parent)
    : SyncableObject(parent)
    , _identityId(other.id())
    , _identityName(other.identityName())
    , _realName(other.realName())
    , _nicks(other.nicks())
    , _awayNick(other.awayNick())
    , _awayNickEnabled(other.awayNickEnabled())
    , _awayReason(other.awayReason())
    , _awayReasonEnabled(other.awayReasonEnabled())
    , _autoAwayEnabled(other.autoAwayEnabled())
    , _autoAwayTime(other.autoAwayTime())
    , _autoAwayReason(other.autoAwayReason())
    , _autoAwayReasonEnabled(other.autoAwayReasonEnabled())
    , _detachAwayEnabled(other.detachAwayEnabled())
    , _detachAwayReason(other.detachAwayReason())
    , _detachAwayReasonEnabled(other.detachAwayReasonEnabled())
    , _ident(other.ident())
    , _kickReason(other.kickReason())
    , _partReason(other.partReason())
    , _quitReason(other.quitReason())
{
    init();
}

#ifdef Q_OS_WIN
#    ifdef UNICODE
QString tcharToQString(TCHAR* tchar)
{
    return QString::fromUtf16(reinterpret_cast<ushort*>(tchar));
}

#    else
QString tcharToQString(TCHAR* tchar)
{
    return QString::fromLocal8Bit(tchar);
}

#    endif

#endif
void Identity::init()
{
    setObjectName(QString::number(id().toInt()));
    setAllowClientUpdates(true);
}

QString Identity::defaultNick()
{
    QString nick = QString("quassel%1").arg(qrand() & 0xff);  // FIXME provide more sensible default nicks

#ifdef Q_OS_MAC
    QString shortUserName = CFStringToQString(CSCopyUserName(true));
    if (!shortUserName.isEmpty())
        nick = shortUserName;

#elif defined(Q_OS_UNIX)
    QString userName;
    struct passwd* pwd = getpwuid(getuid());
    if (pwd)
        userName = pwd->pw_name;
    if (!userName.isEmpty())
        nick = userName;

#elif defined(Q_OS_WIN)
    TCHAR infoBuf[128];
    DWORD bufCharCount = 128;
    // if(GetUserNameEx(/* NameSamCompatible */ 1, infoBuf, &bufCharCount))
    if (GetUserNameEx(NameSamCompatible, infoBuf, &bufCharCount)) {
        QString nickName(tcharToQString(infoBuf));
        int lastBs = nickName.lastIndexOf('\\');
        if (lastBs != -1) {
            nickName = nickName.mid(lastBs + 1);
        }
        if (!nickName.isEmpty())
            nick = nickName;
    }
#endif

    // cleaning forbidden characters from nick
    QRegExp rx(QString("(^[\\d-]+|[^A-Za-z0-9\x5b-\x60\x7b-\x7d])"));  // NOLINT(modernize-raw-string-literal)
    nick.remove(rx);
    return nick;
}

QString Identity::defaultRealName()
{
    QString generalDefault = tr("Quassel IRC User");

#ifdef Q_OS_MAC
    return CFStringToQString(CSCopyUserName(false));

#elif defined(Q_OS_UNIX)
    QString realName;
    struct passwd* pwd = getpwuid(getuid());
    if (pwd)
        realName = QString::fromUtf8(pwd->pw_gecos);
    if (!realName.isEmpty())
        return realName;
    else
        return generalDefault;

#elif defined(Q_OS_WIN)
    TCHAR infoBuf[128];
    DWORD bufCharCount = 128;
    if (GetUserName(infoBuf, &bufCharCount))
        return tcharToQString(infoBuf);
    else
        return generalDefault;
#else
    return generalDefault;
#endif
}

void Identity::setToDefaults()
{
    setIdentityName(tr("<empty>"));
    setRealName(defaultRealName());
    QStringList n = QStringList() << defaultNick();
    setNicks(n);
    setAwayNick("");
    setAwayNickEnabled(false);
    setAwayReason(tr("Gone fishing."));
    setAwayReasonEnabled(true);
    setAutoAwayEnabled(false);
    setAutoAwayTime(10);
    setAutoAwayReason(tr("Not here. No, really. not here!"));
    setAutoAwayReasonEnabled(false);
    setDetachAwayEnabled(false);
    setDetachAwayReason(tr("All Quassel clients vanished from the face of the earth..."));
    setDetachAwayReasonEnabled(false);
    setIdent("quassel");
    setKickReason(tr("Kindergarten is elsewhere!"));
    setPartReason(tr("https://quassel-irc.org - Chat comfortably. Anywhere."));
    setQuitReason(tr("https://quassel-irc.org - Chat comfortably. Anywhere."));
}

/*** setters ***/

void Identity::setId(IdentityId id)
{
    if (_identityId != id) {
        _identityId = id;
        setObjectName(QString::number(id.toInt()));
        emit idSet(id);
    }
}

void Identity::setIdentityName(const QString& identityName)
{
    if (_identityName != identityName) {
        _identityName = identityName;
        emit identityNameSet(identityName);
    }
}

void Identity::setRealName(const QString& realName)
{
    if (_realName != realName) {
        _realName = realName;
        emit realNameSet(realName);
    }
}

void Identity::setNicks(const QStringList& nicks)
{
    if (_nicks != nicks) {
        _nicks = nicks;
        emit nicksSet(nicks);
    }
}

void Identity::setAwayNick(const QString& nick)
{
    if (_awayNick != nick) {
        _awayNick = nick;
        emit awayNickSet(nick);
    }
}

void Identity::setAwayReason(const QString& reason)
{
    if (_awayReason != reason) {
        _awayReason = reason;
        emit awayReasonSet(reason);
    }
}

void Identity::setAwayNickEnabled(bool enabled)
{
    if (_awayNickEnabled != enabled) {
        _awayNickEnabled = enabled;
        emit awayNickEnabledSet(enabled);
    }
}

void Identity::setAwayReasonEnabled(bool enabled)
{
    if (_awayReasonEnabled != enabled) {
        _awayReasonEnabled = enabled;
        emit awayReasonEnabledSet(enabled);
    }
}

void Identity::setAutoAwayEnabled(bool enabled)
{
    if (_autoAwayEnabled != enabled) {
        _autoAwayEnabled = enabled;
        emit autoAwayEnabledSet(enabled);
    }
}

void Identity::setAutoAwayTime(int time)
{
    if (_autoAwayTime != time) {
        _autoAwayTime = time;
        emit autoAwayTimeSet(time);
    }
}

void Identity::setAutoAwayReason(const QString& reason)
{
    if (_autoAwayReason != reason) {
        _autoAwayReason = reason;
        emit autoAwayReasonSet(reason);
    }
}

void Identity::setAutoAwayReasonEnabled(bool enabled)
{
    if (_autoAwayReasonEnabled != enabled) {
        _autoAwayReasonEnabled = enabled;
        emit autoAwayReasonEnabledSet(enabled);
    }
}

void Identity::setDetachAwayEnabled(bool enabled)
{
    if (_detachAwayEnabled != enabled) {
        _detachAwayEnabled = enabled;
        emit detachAwayEnabledSet(enabled);
    }
}

void Identity::setDetachAwayReason(const QString& reason)
{
    if (_detachAwayReason != reason) {
        _detachAwayReason = reason;
        emit detachAwayReasonSet(reason);
    }
}

void Identity::setDetachAwayReasonEnabled(bool enabled)
{
    if (_detachAwayReasonEnabled != enabled) {
        _detachAwayReasonEnabled = enabled;
        emit detachAwayReasonEnabledSet(enabled);
    }
}

void Identity::setIdent(const QString& ident)
{
    if (_ident != ident) {
        _ident = ident;
        emit identSet(ident);
    }
}

void Identity::setKickReason(const QString& reason)
{
    if (_kickReason != reason) {
        _kickReason = reason;
        emit kickReasonSet(reason);
    }
}

void Identity::setPartReason(const QString& reason)
{
    if (_partReason != reason) {
        _partReason = reason;
        emit partReasonSet(reason);
    }
}

void Identity::setQuitReason(const QString& reason)
{
    if (_quitReason != reason) {
        _quitReason = reason;
        emit quitReasonSet(reason);
    }
}

/***  ***/

void Identity::copyFrom(const Identity& other)
{
    for (int idx = staticMetaObject.propertyOffset(); idx < staticMetaObject.propertyCount(); idx++) {
        QMetaProperty metaProp = staticMetaObject.property(idx);
        Q_ASSERT(metaProp.isValid());
        if (this->property(metaProp.name()) != other.property(metaProp.name())) {
            setProperty(metaProp.name(), other.property(metaProp.name()));
        }
    }
}

bool Identity::operator==(const Identity& other) const
{
    for (int idx = staticMetaObject.propertyOffset(); idx < staticMetaObject.propertyCount(); idx++) {
        QMetaProperty metaProp = staticMetaObject.property(idx);
        Q_ASSERT(metaProp.isValid());
        QVariant v1 = this->property(metaProp.name());
        QVariant v2 = other.property(metaProp.name());  // qDebug() << v1 << v2;
        // QVariant cannot compare custom types, so we need to check for this special case
        if (QString(v1.typeName()) == "IdentityId") {
            if (v1.value<IdentityId>() != v2.value<IdentityId>())
                return false;
        }
        else {
            if (v1 != v2)
                return false;
        }
    }
    return true;
}

bool Identity::operator!=(const Identity& other) const
{
    return !(*this == other);
}

///////////////////////////////

QDataStream& operator<<(QDataStream& out, Identity id)
{
    out << id.toVariantMap();
    return out;
}

QDataStream& operator>>(QDataStream& in, Identity& id)
{
    QVariantMap i;
    in >> i;
    id.fromVariantMap(i);
    return in;
}

/***************************************************************************
 *   Copyright (C) 2005-2025 by the Quassel Project                        *
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

#include "ircchannel.h"

#include <QTimeZone>

#include "ircuser.h"
#include "network.h"
#include "signalproxy.h"

IrcChannel::IrcChannel(const QString& channelname, Network* network)
    : SyncableObject(static_cast<QObject*>(network))
    , _name(channelname.toLower())
    , _encrypted(false)
    , _network(network)
{
    static_cast<QObject*>(this)->setObjectName(QString::number(network->networkId().toInt()) + "/" + channelname);
}

void IrcChannel::setTopic(const QString& topic)
{
    if (_topic != topic) {
        _topic = topic;
        SYNC(ARG(topic))
    }
}

void IrcChannel::setPassword(const QString& password)
{
    if (_password != password) {
        _password = password;
        SYNC(ARG(password))
    }
}

void IrcChannel::setEncrypted(bool encrypted)
{
    if (_encrypted != encrypted) {
        _encrypted = encrypted;
        SYNC(ARG(encrypted))
    }
}

void IrcChannel::setCodecForEncoding(const QString& name)
{
    _codecForEncoding = QStringConverter::encodingForName(name.toLatin1());
}

void IrcChannel::setCodecForDecoding(const QString& name)
{
    _codecForDecoding = QStringConverter::encodingForName(name.toLatin1());
}

QString IrcChannel::decodeString(const QByteArray& text) const
{
    if (_codecForDecoding) {
        QStringDecoder decoder(*_codecForDecoding, QStringConverter::Flag::Default);
        return decoder.decode(text);
    }
    return network()->decodeString(text);
}

QByteArray IrcChannel::encodeString(const QString& string) const
{
    if (_codecForEncoding) {
        QStringEncoder encoder(*_codecForEncoding, QStringConverter::Flag::Default);
        return encoder.encode(string);
    }
    return network()->encodeString(string);
}

bool IrcChannel::hasUser(IrcUser* user) const
{
    return _userModes.contains(user);
}

QString IrcChannel::userModes(IrcUser* user) const
{
    return _userModes.value(user, QString());
}

QStringList IrcChannel::userList() const
{
    QStringList users;
    for (auto it = _userModes.constBegin(); it != _userModes.constEnd(); ++it) {
        if (it.key()) {
            users << it.key()->nick();
        }
    }
    return users;
}

void IrcChannel::joinIrcUsers(const QStringList& nicks, const QStringList& modes)
{
    if (nicks.isEmpty())
        return;

    for (int i = 0; i < nicks.size(); ++i) {
        IrcUser* user = network()->ircUser(nicks[i]);
        if (!user) {
            qWarning() << Q_FUNC_INFO << "Unknown user:" << nicks[i];
            continue;
        }
        if (!_userModes.contains(user)) {
            _userModes[user] = i < modes.size() ? modes[i] : QString();
            connect(user, &QObject::destroyed, this, &IrcChannel::userDestroyed);
            user->joinChannel(this, true);  // Skip reciprocal join to avoid infinite loop
        }
    }
    SYNC_OTHER(joinIrcUsers, ARG(nicks), ARG(modes))
}

void IrcChannel::joinIrcUser(IrcUser* user)
{
    if (!_userModes.contains(user)) {
        _userModes[user] = QString();
        connect(user, &QObject::destroyed, this, &IrcChannel::userDestroyed);
        user->joinChannel(this, true);  // Skip reciprocal join to avoid infinite loop
        QStringList nicks = QStringList() << user->nick();
        QStringList modes = QStringList() << QString();
        SYNC_OTHER(joinIrcUsers, ARG(nicks), ARG(modes))
    }
}

void IrcChannel::part(IrcUser* user)
{
    if (_userModes.contains(user)) {
        _userModes.remove(user);
        disconnect(user, nullptr, this, nullptr);
        QString nick = user->nick();
        SYNC_OTHER(part, ARG(nick))
    }
}

void IrcChannel::partChannel()
{
    for (auto user : _userModes.keys()) {
        part(user);
    }
    network()->removeIrcChannel(this);
    SYNC_OTHER(partChannel, NO_ARG)
}

void IrcChannel::setUserModes(IrcUser* user, const QString& modes)
{
    if (_userModes.contains(user) && _userModes[user] != modes) {
        _userModes[user] = modes;
        QString nick = user->nick();
        SYNC_OTHER(setUserModes, ARG(nick), ARG(modes))
    }
}

void IrcChannel::addUserMode(IrcUser* user, const QString& mode)
{
    if (_userModes.contains(user) && !mode.isEmpty()) {
        QString currentModes = _userModes[user];
        if (!currentModes.contains(mode)) {
            _userModes[user] = currentModes + mode;
            QString nick = user->nick();
            SYNC_OTHER(addUserMode, ARG(nick), ARG(mode))
        }
    }
}

void IrcChannel::removeUserMode(IrcUser* user, const QString& mode)
{
    if (_userModes.contains(user) && !mode.isEmpty()) {
        QString currentModes = _userModes[user];
        if (currentModes.contains(mode)) {
            currentModes.remove(mode);
            _userModes[user] = currentModes;
            QString nick = user->nick();
            SYNC_OTHER(removeUserMode, ARG(nick), ARG(mode))
        }
    }
}

void IrcChannel::addChannelMode(const QString& mode, const QString& param)
{
    // Placeholder: implement if channel modes are stored
    Q_UNUSED(mode)
    Q_UNUSED(param)
    SYNC_OTHER(addChannelMode, ARG(mode), ARG(param))
}

void IrcChannel::removeChannelMode(const QString& mode, const QString& param)
{
    // Placeholder: implement if channel modes are stored
    Q_UNUSED(mode)
    Q_UNUSED(param)
    SYNC_OTHER(removeChannelMode, ARG(mode), ARG(param))
}

void IrcChannel::ircUserNickSet(const QString& newnick)
{
    IrcUser* user = qobject_cast<IrcUser*>(sender());
    if (user && _userModes.contains(user)) {
        QString modes = _userModes[user];
        _userModes.remove(user);
        _userModes[network()->ircUser(newnick)] = modes;
        QStringList nicks = QStringList() << newnick;
        QStringList modeList = QStringList() << modes;
        SYNC_OTHER(joinIrcUsers, ARG(nicks), ARG(modeList))
    }
}

void IrcChannel::userDestroyed()
{
    IrcUser* user = qobject_cast<IrcUser*>(sender());
    if (user) {
        part(user);
    }
}

QVariantMap IrcChannel::toVariantMap()
{
    QVariantMap map;
    map["name"] = _name;
    map["topic"] = _topic;
    map["password"] = _password;
    map["encrypted"] = _encrypted;
    QVariantMap users;
    for (auto it = _userModes.constBegin(); it != _userModes.constEnd(); ++it) {
        if (it.key()) {
            users[it.key()->nick()] = it.value();
        }
    }
    map["userModes"] = users;
    return map;
}

void IrcChannel::fromVariantMap(const QVariantMap& map)
{
    if (map.contains("name")) {
        _name = map["name"].toString().toLower();
        // Replace updateObjectName() with setObjectName
        static_cast<QObject*>(this)->setObjectName(QString::number(network()->networkId().toInt()) + "/" + _name);
    }
    if (map.contains("topic")) {
        setTopic(map["topic"].toString());
    }
    if (map.contains("password")) {
        setPassword(map["password"].toString());
    }
    if (map.contains("encrypted")) {
        setEncrypted(map["encrypted"].toBool());
    }
    if (map.contains("userModes")) {
        QVariantMap users = map["userModes"].toMap();
        for (auto it = users.constBegin(); it != users.constEnd(); ++it) {
            IrcUser* user = network()->ircUser(it.key());
            if (user) {
                setUserModes(user, it.value().toString());
            }
        }
    }
}

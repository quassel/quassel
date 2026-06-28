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
    // ObjectName will be set by Network::addIrcChannel()
}

QString IrcChannel::modes() const
{
    QString modeString;
    
    // Add D modes (no parameters)
    for (QChar mode : _D_channelModes) {
        modeString += mode;
    }
    
    // Add A modes (list parameters)
    for (auto it = _A_channelModes.constBegin(); it != _A_channelModes.constEnd(); ++it) {
        modeString += it.key();
        if (!it.value().isEmpty()) {
            modeString += " " + it.value().join(" ");
        }
    }
    
    // Add B modes (parameter always)
    for (auto it = _B_channelModes.constBegin(); it != _B_channelModes.constEnd(); ++it) {
        modeString += it.key();
        if (!it.value().isEmpty()) {
            modeString += " " + it.value();
        }
    }
    
    // Add C modes (parameter only when set)
    for (auto it = _C_channelModes.constBegin(); it != _C_channelModes.constEnd(); ++it) {
        modeString += it.key();
        if (!it.value().isEmpty()) {
            modeString += " " + it.value();
        }
    }
    
    return modeString;
}

void IrcChannel::setTopic(const QString& topic)
{
    if (_topic != topic) {
        _topic = topic;
        SYNC(ARG(topic))
        emit topicChanged(topic);
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
        emit encryptedChanged(encrypted);
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

QList<IrcUser*> IrcChannel::ircUsers() const
{
    return _userModes.keys();
}

void IrcChannel::joinIrcUsers(const QStringList& nicks, const QStringList& modes)
{
    // Optimized: Processing user join for channel
    
    if (nicks.isEmpty())
        return;

    QStringList actuallyJoinedNicks;
    QStringList actuallyJoinedModes;

    for (int i = 0; i < nicks.size(); ++i) {
        IrcUser* user = network()->ircUser(nicks[i]);
        if (!user) {
            // Auto-create missing user - this handles sync timing issues between Qt5 core and Qt6 client
            user = network()->addIrcUser(nicks[i]);
        }
        if (!user) {
            qWarning() << Q_FUNC_INFO << "Failed to create user:" << nicks[i];
            continue;
        }
        if (!_userModes.contains(user)) {
            _userModes[user] = i < modes.size() ? modes[i] : QString();
            connect(user, &QObject::destroyed, this, &IrcChannel::userDestroyed);
            user->joinChannel(this, true);
            actuallyJoinedNicks << nicks[i];
            actuallyJoinedModes << (i < modes.size() ? modes[i] : QString());
            // User added to channel
        } else {
            // User already in channel (skipped)
        }
    }
    
    // Only sync and emit signals if we actually added new users
    if (!actuallyJoinedNicks.isEmpty()) {
        SYNC_OTHER(joinIrcUsers, ARG(actuallyJoinedNicks), ARG(actuallyJoinedModes))
        emit usersJoined(actuallyJoinedNicks, actuallyJoinedModes);
        // Emitted usersJoined signal for new users
    } else {
        // No new users added, signal not emitted (optimization)
    }
}

void IrcChannel::joinIrcUser(IrcUser* user)
{
    if (!_userModes.contains(user)) {
        _userModes[user] = QString();
        connect(user, &QObject::destroyed, this, &IrcChannel::userDestroyed);
        user->joinChannel(this, true);
        QStringList nicks = QStringList() << user->nick();
        QStringList modes = QStringList() << QString();
        SYNC_OTHER(joinIrcUsers, ARG(nicks), ARG(modes))
        emit usersJoined(nicks, modes);
    }
}

void IrcChannel::part(IrcUser* user)
{
    if (_userModes.contains(user)) {
        _userModes.remove(user);
        disconnect(user, nullptr, this, nullptr);
        QString nick = user->nick();
        SYNC_OTHER(part, ARG(nick))
        emit userParted(user);
    }
}

void IrcChannel::part(const QString& nick)
{
    IrcUser* user = network()->ircUser(nick);
    if (user) {
        part(user);
    } else {
        qWarning() << Q_FUNC_INFO << "Unknown user:" << nick << "on channel" << name();
    }
}

void IrcChannel::partChannel()
{
    for (auto user : _userModes.keys()) {
        part(user);
    }
    // Removing channel from network and updating UI
    network()->removeIrcChannel(this);
    SYNC_OTHER(partChannel, NO_ARG)
    emit parted();
    // Channel part completed successfully
}

void IrcChannel::setUserModes(IrcUser* user, const QString& modes)
{
    if (_userModes.contains(user) && _userModes[user] != modes) {
        _userModes[user] = modes;
        QString nick = user->nick();
        SYNC_OTHER(setUserModes, ARG(nick), ARG(modes))
        emit userModesChanged(user, modes);
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
            emit userModeAdded(user, mode);
        }
    }
}

// Sync-compatible overload that takes nick string instead of IrcUser pointer
void IrcChannel::addUserMode(const QString& nick, const QString& mode)
{
    IrcUser* user = network()->ircUser(nick);
    if (!user) {
        user = network()->addIrcUser(nick);
    }
    if (user) {
        addUserMode(user, mode);
    } else {
        qWarning() << Q_FUNC_INFO << "Failed to find or create user:" << nick;
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
            emit userModeRemoved(user, mode);
        }
    }
}

void IrcChannel::addChannelMode(const QChar& mode, const QString& value)
{
    Network::ChannelModeType modeType = network()->channelModeType(mode);

    switch (modeType) {
    case Network::NOT_A_CHANMODE:
        return;
    case Network::A_CHANMODE:
        if (!_A_channelModes.contains(mode))
            _A_channelModes[mode] = QStringList(value);
        else if (!_A_channelModes[mode].contains(value))
            _A_channelModes[mode] << value;
        break;

    case Network::B_CHANMODE:
        _B_channelModes[mode] = value;
        break;

    case Network::C_CHANMODE:
        _C_channelModes[mode] = value;
        break;

    case Network::D_CHANMODE:
        _D_channelModes << mode;
        break;
    }
    SYNC_OTHER(addChannelMode, ARG(mode), ARG(value))
}

void IrcChannel::removeChannelMode(const QChar& mode, const QString& value)
{
    Network::ChannelModeType modeType = network()->channelModeType(mode);

    switch (modeType) {
    case Network::NOT_A_CHANMODE:
        return;
    case Network::A_CHANMODE:
        if (_A_channelModes.contains(mode))
            _A_channelModes[mode].removeAll(value);
        break;

    case Network::B_CHANMODE:
        _B_channelModes.remove(mode);
        break;

    case Network::C_CHANMODE:
        _C_channelModes.remove(mode);
        break;

    case Network::D_CHANMODE:
        _D_channelModes.remove(mode);
        break;
    }
    SYNC_OTHER(removeChannelMode, ARG(mode), ARG(value))
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

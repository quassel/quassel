/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <QDebug>
#include <QHashIterator>
#include <QMapIterator>
#include <QTextCodec>

#include "ircuser.h"
#include "network.h"
#include "util.h"

IrcChannel::IrcChannel(const QString& channelname, Network* network)
    : SyncableObject(network)
    , _initialized(false)
    , _name(channelname)
    , _topic(QString())
    , _encrypted(false)
    , _network(network)
    , _codecForEncoding(nullptr)
    , _codecForDecoding(nullptr)
{
    setObjectName(QString::number(network->networkId().toInt()) + "/" + channelname);
}

// ====================
//  PUBLIC:
// ====================
bool IrcChannel::isKnownUser(IrcUser* ircuser) const
{
    if (ircuser == nullptr) {
        qWarning() << "Channel" << name() << "received IrcUser Nullpointer!";
        return false;
    }

    if (!_userModes.contains(ircuser)) {
        // This can happen e.g. when disconnecting from a network, so don't log a warning
        return false;
    }

    return true;
}

bool IrcChannel::isValidChannelUserMode(const QString& mode) const
{
    bool isvalid = true;
    if (mode.size() > 1) {
        qWarning() << "Channel" << name() << "received Channel User Mode which is longer than 1 Char:" << mode;
        isvalid = false;
    }
    return isvalid;
}

QString IrcChannel::userModes(IrcUser* ircuser) const
{
    if (_userModes.contains(ircuser))
        return _userModes[ircuser];
    else
        return QString();
}

QString IrcChannel::userModes(const QString& nick) const
{
    return userModes(network()->ircUser(nick));
}

void IrcChannel::setCodecForEncoding(const QString& name)
{
    setCodecForEncoding(QTextCodec::codecForName(name.toLatin1()));
}

void IrcChannel::setCodecForEncoding(QTextCodec* codec)
{
    _codecForEncoding = codec;
}

void IrcChannel::setCodecForDecoding(const QString& name)
{
    setCodecForDecoding(QTextCodec::codecForName(name.toLatin1()));
}

void IrcChannel::setCodecForDecoding(QTextCodec* codec)
{
    _codecForDecoding = codec;
}

QString IrcChannel::decodeString(const QByteArray& text) const
{
    if (!codecForDecoding())
        return network()->decodeString(text);
    return ::decodeString(text, _codecForDecoding);
}

QByteArray IrcChannel::encodeString(const QString& string) const
{
    if (codecForEncoding()) {
        return _codecForEncoding->fromUnicode(string);
    }
    return network()->encodeString(string);
}

// ====================
//  PUBLIC SLOTS:
// ====================
void IrcChannel::setTopic(const QString& topic)
{
    _topic = topic;
    SYNC(ARG(topic))
    emit topicSet(topic);
}

void IrcChannel::setPassword(const QString& password)
{
    _password = password;
    SYNC(ARG(password))
}

void IrcChannel::setEncrypted(bool encrypted)
{
    _encrypted = encrypted;
    SYNC(ARG(encrypted))
    emit encryptedSet(encrypted);
}

void IrcChannel::joinIrcUsers(const QList<IrcUser*>& users, const QStringList& modes)
{
    if (users.isEmpty())
        return;

    if (users.count() != modes.count()) {
        qWarning() << "IrcChannel::addUsers(): number of nicks does not match number of modes!";
        return;
    }

    // Sort user modes first
    const QStringList sortedModes = network()->sortPrefixModes(modes);

    QStringList newNicks;
    QStringList newModes;
    QList<IrcUser*> newUsers;

    IrcUser* ircuser;
    for (int i = 0; i < users.count(); i++) {
        ircuser = users[i];
        if (!ircuser)
            continue;
        if (_userModes.contains(ircuser)) {
            if (sortedModes[i].count() > 1) {
                // Multiple modes received, do it one at a time
                // TODO Better way of syncing this without breaking protocol?
                for (int i_m = 0; i_m < sortedModes[i].count(); ++i_m) {
                    addUserMode(ircuser, sortedModes[i][i_m]);
                }
            }
            else {
                addUserMode(ircuser, sortedModes[i]);
            }
            continue;
        }

        _userModes[ircuser] = sortedModes[i];
        ircuser->joinChannel(this, true);
        connect(ircuser, &IrcUser::nickSet, this, selectOverload<QString>(&IrcChannel::ircUserNickSet));

        // connect(ircuser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
        // If you wonder why there is no counterpart to ircUserJoined:
        // the joins are propagated by the ircuser. The signal ircUserJoined is only for convenience

        newNicks << ircuser->nick();
        newModes << sortedModes[i];
        newUsers << ircuser;
    }

    if (newNicks.isEmpty())
        return;

    SYNC_OTHER(joinIrcUsers, ARG(newNicks), ARG(newModes));
    emit ircUsersJoined(newUsers);
}

void IrcChannel::joinIrcUsers(const QStringList& nicks, const QStringList& modes)
{
    QList<IrcUser*> users;
    foreach (QString nick, nicks)
        users << network()->newIrcUser(nick);
    joinIrcUsers(users, modes);
}

void IrcChannel::joinIrcUser(IrcUser* ircuser)
{
    QList<IrcUser*> users;
    users << ircuser;
    QStringList modes;
    modes << QString();
    joinIrcUsers(users, modes);
}

void IrcChannel::part(IrcUser* ircuser)
{
    if (isKnownUser(ircuser)) {
        _userModes.remove(ircuser);
        ircuser->partChannel(this);
        // If you wonder why there is no counterpart to ircUserParted:
        // the joins are propagted by the ircuser. The signal ircUserParted is only for convenience
        disconnect(ircuser, nullptr, this, nullptr);
        emit ircUserParted(ircuser);

        if (network()->isMe(ircuser) || _userModes.isEmpty()) {
            // in either case we're no longer in the channel
            //  -> clean up the channel and destroy it
            QList<IrcUser*> users = _userModes.keys();
            _userModes.clear();
            foreach (IrcUser* user, users) {
                disconnect(user, nullptr, this, nullptr);
                user->partChannelInternal(this, true);
            }
            emit parted();
            network()->removeIrcChannel(this);
        }
    }
}

void IrcChannel::part(const QString& nick)
{
    part(network()->ircUser(nick));
}

// SET USER MODE
void IrcChannel::setUserModes(IrcUser* ircuser, const QString& modes)
{
    if (isKnownUser(ircuser)) {
        // Keep user modes sorted
        _userModes[ircuser] = network()->sortPrefixModes(modes);
        QString nick = ircuser->nick();
        SYNC_OTHER(setUserModes, ARG(nick), ARG(modes))
        emit ircUserModesSet(ircuser, modes);
    }
}

void IrcChannel::setUserModes(const QString& nick, const QString& modes)
{
    setUserModes(network()->ircUser(nick), modes);
}

// ADD USER MODE
void IrcChannel::addUserMode(IrcUser* ircuser, const QString& mode)
{
    if (!isKnownUser(ircuser) || !isValidChannelUserMode(mode))
        return;

    if (!_userModes[ircuser].contains(mode)) {
        // Keep user modes sorted
        _userModes[ircuser] = network()->sortPrefixModes(_userModes[ircuser] + mode);
        QString nick = ircuser->nick();
        SYNC_OTHER(addUserMode, ARG(nick), ARG(mode))
        emit ircUserModeAdded(ircuser, mode);
    }
}

void IrcChannel::addUserMode(const QString& nick, const QString& mode)
{
    addUserMode(network()->ircUser(nick), mode);
}

// REMOVE USER MODE
void IrcChannel::removeUserMode(IrcUser* ircuser, const QString& mode)
{
    if (!isKnownUser(ircuser) || !isValidChannelUserMode(mode))
        return;

    if (_userModes[ircuser].contains(mode)) {
        // Removing modes shouldn't mess up ordering
        _userModes[ircuser].remove(mode);
        QString nick = ircuser->nick();
        SYNC_OTHER(removeUserMode, ARG(nick), ARG(mode));
        emit ircUserModeRemoved(ircuser, mode);
    }
}

void IrcChannel::removeUserMode(const QString& nick, const QString& mode)
{
    removeUserMode(network()->ircUser(nick), mode);
}

// INIT SET USER MODES
QVariantMap IrcChannel::initUserModes() const
{
    QVariantMap usermodes;
    QHash<IrcUser*, QString>::const_iterator iter = _userModes.constBegin();
    while (iter != _userModes.constEnd()) {
        usermodes[iter.key()->nick()] = iter.value();
        ++iter;
    }
    return usermodes;
}

void IrcChannel::initSetUserModes(const QVariantMap& usermodes)
{
    QList<IrcUser*> users;
    QStringList modes;
    QVariantMap::const_iterator iter = usermodes.constBegin();
    while (iter != usermodes.constEnd()) {
        users << network()->newIrcUser(iter.key());
        modes << iter.value().toString();
        ++iter;
    }
    // joinIrcUsers handles sorting modes
    joinIrcUsers(users, modes);
}

QVariantMap IrcChannel::initChanModes() const
{
    QVariantMap channelModes;

    QVariantMap A_modes;
    QHash<QChar, QStringList>::const_iterator A_iter = _A_channelModes.constBegin();
    while (A_iter != _A_channelModes.constEnd()) {
        A_modes[A_iter.key()] = A_iter.value();
        ++A_iter;
    }
    channelModes["A"] = A_modes;

    QVariantMap B_modes;
    QHash<QChar, QString>::const_iterator B_iter = _B_channelModes.constBegin();
    while (B_iter != _B_channelModes.constEnd()) {
        B_modes[B_iter.key()] = B_iter.value();
        ++B_iter;
    }
    channelModes["B"] = B_modes;

    QVariantMap C_modes;
    QHash<QChar, QString>::const_iterator C_iter = _C_channelModes.constBegin();
    while (C_iter != _C_channelModes.constEnd()) {
        C_modes[C_iter.key()] = C_iter.value();
        ++C_iter;
    }
    channelModes["C"] = C_modes;

    QString D_modes;
    QSet<QChar>::const_iterator D_iter = _D_channelModes.constBegin();
    while (D_iter != _D_channelModes.constEnd()) {
        D_modes += *D_iter;
        ++D_iter;
    }
    channelModes["D"] = D_modes;

    return channelModes;
}

void IrcChannel::initSetChanModes(const QVariantMap& channelModes)
{
    QVariantMap::const_iterator iter = channelModes["A"].toMap().constBegin();
    QVariantMap::const_iterator iterEnd = channelModes["A"].toMap().constEnd();
    while (iter != iterEnd) {
        _A_channelModes[iter.key()[0]] = iter.value().toStringList();
        ++iter;
    }

    iter = channelModes["B"].toMap().constBegin();
    iterEnd = channelModes["B"].toMap().constEnd();
    while (iter != iterEnd) {
        _B_channelModes[iter.key()[0]] = iter.value().toString();
        ++iter;
    }

    iter = channelModes["C"].toMap().constBegin();
    iterEnd = channelModes["C"].toMap().constEnd();
    while (iter != iterEnd) {
        _C_channelModes[iter.key()[0]] = iter.value().toString();
        ++iter;
    }

    QString D_modes = channelModes["D"].toString();
    for (int i = 0; i < D_modes.count(); i++) {
        _D_channelModes << D_modes[i];
    }
}

void IrcChannel::ircUserDestroyed()
{
    auto* ircUser = static_cast<IrcUser*>(sender());
    Q_ASSERT(ircUser);
    _userModes.remove(ircUser);
    // no further propagation.
    // this leads only to fuck ups.
}

void IrcChannel::ircUserNickSet(QString nick)
{
    auto* ircUser = qobject_cast<IrcUser*>(sender());
    Q_ASSERT(ircUser);
    emit ircUserNickSet(ircUser, nick);
}

/*******************************************************************************
 *
 * 3.3 CHANMODES
 *
 *    o  CHANMODES=A,B,C,D
 *
 *    The CHANMODES token specifies the modes that may be set on a channel.
 *    These modes are split into four categories, as follows:
 *
 *    o  Type A: Modes that add or remove an address to or from a list.
 *       These modes always take a parameter when sent by the server to a
 *       client; when sent by a client, they may be specified without a
 *       parameter, which requests the server to display the current
 *       contents of the corresponding list on the channel to the client.
 *    o  Type B: Modes that change a setting on the channel.  These modes
 *       always take a parameter.
 *    o  Type C: Modes that change a setting on the channel. These modes
 *       take a parameter only when set; the parameter is absent when the
 *       mode is removed both in the client's and server's MODE command.
 *    o  Type D: Modes that change a setting on the channel. These modes
 *       never take a parameter.
 *
 *    If the server sends any additional types after these 4, the client
 *    MUST ignore them; this is intended to allow future extension of this
 *    token.
 *
 *    The IRC server MUST NOT list modes in CHANMODES which are also
 *    present in the PREFIX parameter; however, for completeness, modes
 *    described in PREFIX may be treated as type B modes.
 *
 ******************************************************************************/

/*******************************************************************************
 * Short Version:
 * A --> add/remove from List
 * B --> set value or remove
 * C --> set value or remove
 * D --> on/off
 *
 * B and C behave very similar... we store the data in different datastructures
 * for future compatibility
 ******************************************************************************/

// NOTE: the behavior of addChannelMode and removeChannelMode depends on the type of mode
// see list above for chanmode types
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
    SYNC(ARG(mode), ARG(value))
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
    SYNC(ARG(mode), ARG(value))
}

bool IrcChannel::hasMode(const QChar& mode) const
{
    Network::ChannelModeType modeType = network()->channelModeType(mode);

    switch (modeType) {
    case Network::NOT_A_CHANMODE:
        return false;
    case Network::A_CHANMODE:
        return _A_channelModes.contains(mode);
    case Network::B_CHANMODE:
        return _B_channelModes.contains(mode);
    case Network::C_CHANMODE:
        return _C_channelModes.contains(mode);
    case Network::D_CHANMODE:
        return _D_channelModes.contains(mode);
    }
    return false;
}

QString IrcChannel::modeValue(const QChar& mode) const
{
    Network::ChannelModeType modeType = network()->channelModeType(mode);

    switch (modeType) {
    case Network::B_CHANMODE:
        if (_B_channelModes.contains(mode))
            return _B_channelModes[mode];
        else
            return QString();
    case Network::C_CHANMODE:
        if (_C_channelModes.contains(mode))
            return _C_channelModes[mode];
        else
            return QString();
    default:
        return QString();
    }
}

QStringList IrcChannel::modeValueList(const QChar& mode) const
{
    Network::ChannelModeType modeType = network()->channelModeType(mode);

    switch (modeType) {
    case Network::A_CHANMODE:
        if (_A_channelModes.contains(mode))
            return _A_channelModes[mode];
        break;
    default:
        ;
    }
    return {};
}

QString IrcChannel::channelModeString() const
{
    QStringList params;
    QString modeString;

    QSet<QChar>::const_iterator D_iter = _D_channelModes.constBegin();
    while (D_iter != _D_channelModes.constEnd()) {
        modeString += *D_iter;
        ++D_iter;
    }

    QHash<QChar, QString>::const_iterator BC_iter = _C_channelModes.constBegin();
    while (BC_iter != _C_channelModes.constEnd()) {
        modeString += BC_iter.key();
        params << BC_iter.value();
        ++BC_iter;
    }

    BC_iter = _B_channelModes.constBegin();
    while (BC_iter != _B_channelModes.constEnd()) {
        modeString += BC_iter.key();
        params << BC_iter.value();
        ++BC_iter;
    }
    if (modeString.isEmpty())
        return modeString;
    else
        return QString("+%1 %2").arg(modeString).arg(params.join(" "));
}

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

#include "ircuser.h"
#include "util.h"

#include "network.h"
#include "signalproxy.h"
#include "ircchannel.h"

#include <QTextCodec>
#include <QDebug>

INIT_SYNCABLE_OBJECT(IrcUser)
IrcUser::IrcUser(const QString &hostmask, Network *network) : SyncableObject(network),
    _initialized(false),
    _nick(nickFromMask(hostmask)),
    _user(userFromMask(hostmask)),
    _host(hostFromMask(hostmask)),
    _realName(),
    _awayMessage(),
    _away(false),
    _server(),
    // _idleTime(QDateTime::currentDateTime()),
    _ircOperator(),
    _lastAwayMessage(),
    _whoisServiceReply(),
    _encrypted(false),
    _network(network),
    _codecForEncoding(0),
    _codecForDecoding(0)
{
    updateObjectName();
    _lastAwayMessage.setTimeSpec(Qt::UTC);
    _lastAwayMessage.setMSecsSinceEpoch(0);
}


IrcUser::~IrcUser()
{
}


// ====================
//  PUBLIC:
// ====================

QString IrcUser::hostmask() const
{
    return QString("%1!%2@%3").arg(nick()).arg(user()).arg(host());
}


QDateTime IrcUser::idleTime()
{
    if ((QDateTime::currentDateTime().toMSecsSinceEpoch() - _idleTimeSet.toMSecsSinceEpoch())
            > 1200000) {
        // 20 * 60 * 1000 = 1200000
        // 20 minutes have elapsed, clear the known idle time as it's likely inaccurate by now
        _idleTime = QDateTime();
    }
    return _idleTime;
}


QStringList IrcUser::channels() const
{
    QStringList chanList;
    IrcChannel *channel;
    foreach(channel, _channels) {
        chanList << channel->name();
    }
    return chanList;
}


void IrcUser::setCodecForEncoding(const QString &name)
{
    setCodecForEncoding(QTextCodec::codecForName(name.toLatin1()));
}


void IrcUser::setCodecForEncoding(QTextCodec *codec)
{
    _codecForEncoding = codec;
}


void IrcUser::setCodecForDecoding(const QString &name)
{
    setCodecForDecoding(QTextCodec::codecForName(name.toLatin1()));
}


void IrcUser::setCodecForDecoding(QTextCodec *codec)
{
    _codecForDecoding = codec;
}


QString IrcUser::decodeString(const QByteArray &text) const
{
    if (!codecForDecoding()) return network()->decodeString(text);
    return ::decodeString(text, codecForDecoding());
}


QByteArray IrcUser::encodeString(const QString &string) const
{
    if (codecForEncoding()) {
        return codecForEncoding()->fromUnicode(string);
    }
    return network()->encodeString(string);
}


// ====================
//  PUBLIC SLOTS:
// ====================
void IrcUser::setUser(const QString &user)
{
    if (!user.isEmpty() && _user != user) {
        _user = user;
        SYNC(ARG(user));
    }
}


void IrcUser::setRealName(const QString &realName)
{
    if (!realName.isEmpty() && _realName != realName) {
        _realName = realName;
        SYNC(ARG(realName))
    }
}


void IrcUser::setAccount(const QString &account)
{
    if (_account != account) {
        _account = account;
        SYNC(ARG(account))
    }
}


void IrcUser::setAway(const bool &away)
{
    if (away != _away) {
        _away = away;
        SYNC(ARG(away))
        emit awaySet(away);
    }
}


void IrcUser::setAwayMessage(const QString &awayMessage)
{
    if (!awayMessage.isEmpty() && _awayMessage != awayMessage) {
        _awayMessage = awayMessage;
        SYNC(ARG(awayMessage))
    }
}


void IrcUser::setIdleTime(const QDateTime &idleTime)
{
    if (idleTime.isValid() && _idleTime != idleTime) {
        _idleTime = idleTime;
        _idleTimeSet = QDateTime::currentDateTime();
        SYNC(ARG(idleTime))
    }
}


void IrcUser::setLoginTime(const QDateTime &loginTime)
{
    if (loginTime.isValid() && _loginTime != loginTime) {
        _loginTime = loginTime;
        SYNC(ARG(loginTime))
    }
}


void IrcUser::setServer(const QString &server)
{
    if (!server.isEmpty() && _server != server) {
        _server = server;
        SYNC(ARG(server))
    }
}


void IrcUser::setIrcOperator(const QString &ircOperator)
{
    if (!ircOperator.isEmpty() && _ircOperator != ircOperator) {
        _ircOperator = ircOperator;
        SYNC(ARG(ircOperator))
    }
}


void IrcUser::setLastAwayMessage(const QDateTime &lastAwayMessage)
{
    if (lastAwayMessage > _lastAwayMessage) {
        _lastAwayMessage = lastAwayMessage;
        SYNC(ARG(lastAwayMessage))
    }
}


void IrcUser::setHost(const QString &host)
{
    if (!host.isEmpty() && _host != host) {
        _host = host;
        SYNC(ARG(host))
    }
}


void IrcUser::setNick(const QString &nick)
{
    if (!nick.isEmpty() && nick != _nick) {
        _nick = nick;
        updateObjectName();
        SYNC(ARG(nick))
        emit nickSet(nick);
    }
}


void IrcUser::setWhoisServiceReply(const QString &whoisServiceReply)
{
    if (!whoisServiceReply.isEmpty() && whoisServiceReply != _whoisServiceReply) {
        _whoisServiceReply = whoisServiceReply;
        SYNC(ARG(whoisServiceReply))
    }
}


void IrcUser::setSuserHost(const QString &suserHost)
{
    if (!suserHost.isEmpty() && suserHost != _suserHost) {
        _suserHost = suserHost;
        SYNC(ARG(suserHost))
    }
}


void IrcUser::setEncrypted(bool encrypted)
{
    _encrypted = encrypted;
    emit encryptedSet(encrypted);
    SYNC(ARG(encrypted))
}


void IrcUser::updateObjectName()
{
    renameObject(QString::number(network()->networkId().toInt()) + "/" + _nick);
}


void IrcUser::updateHostmask(const QString &mask)
{
    if (mask == hostmask())
        return;

    QString user = userFromMask(mask);
    QString host = hostFromMask(mask);
    setUser(user);
    setHost(host);
}


void IrcUser::joinChannel(IrcChannel *channel, bool skip_channel_join)
{
    Q_ASSERT(channel);
    if (!_channels.contains(channel)) {
        _channels.insert(channel);
        if (!skip_channel_join)
            channel->joinIrcUser(this);
    }
}


void IrcUser::joinChannel(const QString &channelname)
{
    joinChannel(network()->newIrcChannel(channelname));
}


void IrcUser::partChannel(IrcChannel *channel)
{
    if (_channels.contains(channel)) {
        _channels.remove(channel);
        disconnect(channel, 0, this, 0);
        channel->part(this);
        QString channelName = channel->name();
        SYNC_OTHER(partChannel, ARG(channelName))
        if (_channels.isEmpty() && !network()->isMe(this))
            quit();
    }
}


void IrcUser::partChannel(const QString &channelname)
{
    IrcChannel *channel = network()->ircChannel(channelname);
    if (channel == 0) {
        qWarning() << "IrcUser::partChannel(): received part for unknown Channel" << channelname;
    }
    else {
        partChannel(channel);
    }
}


void IrcUser::quit()
{
    QList<IrcChannel *> channels = _channels.toList();
    _channels.clear();
    foreach(IrcChannel *channel, channels) {
        disconnect(channel, 0, this, 0);
        channel->part(this);
    }
    network()->removeIrcUser(this);
    SYNC(NO_ARG)
    emit quited();
}


void IrcUser::channelDestroyed()
{
    // private slot!
    IrcChannel *channel = static_cast<IrcChannel *>(sender());
    if (_channels.contains(channel)) {
        _channels.remove(channel);
        if (_channels.isEmpty() && !network()->isMe(this))
            quit();
    }
}


void IrcUser::setUserModes(const QString &modes)
{
    if (_userModes != modes) {
        _userModes = modes;
        SYNC(ARG(modes))
        emit userModesSet(modes);
    }
}


void IrcUser::addUserModes(const QString &modes)
{
    if (modes.isEmpty())
        return;

    // Don't needlessly sync when no changes are made
    bool changesMade = false;
    for (int i = 0; i < modes.count(); i++) {
        if (!_userModes.contains(modes[i])) {
            _userModes += modes[i];
            changesMade = true;
        }
    }

    if (changesMade) {
        SYNC(ARG(modes))
        emit userModesAdded(modes);
    }
}


void IrcUser::removeUserModes(const QString &modes)
{
    if (modes.isEmpty())
        return;

    for (int i = 0; i < modes.count(); i++) {
        _userModes.remove(modes[i]);
    }
    SYNC(ARG(modes))
    emit userModesRemoved(modes);
}


void IrcUser::setLastChannelActivity(BufferId buffer, const QDateTime &time)
{
    _lastActivity[buffer] = time;
    emit lastChannelActivityUpdated(buffer, time);
}


void IrcUser::setLastSpokenTo(BufferId buffer, const QDateTime &time)
{
    _lastSpokenTo[buffer] = time;
    emit lastSpokenToUpdated(buffer, time);
}

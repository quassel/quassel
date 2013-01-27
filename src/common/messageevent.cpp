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

#include "messageevent.h"

Event *MessageEvent::create(EventManager::EventType type, QVariantMap &map, Network *network)
{
    if (type == EventManager::MessageEvent)
        return new MessageEvent(type, map, network);

    return 0;
}


MessageEvent::MessageEvent(Message::Type msgType, Network *net, const QString &msg, const QString &sender, const QString &target,
    Message::Flags flags, const QDateTime &timestamp)
    : NetworkEvent(EventManager::MessageEvent, net),
    _msgType(msgType),
    _text(msg),
    _sender(sender),
    _target(target),
    _msgFlags(flags)
{
    IrcChannel *channel = network()->ircChannel(_target);
    if (!channel) {
        if (!_target.isEmpty() && network()->prefixes().contains(_target.at(0)))
            _target = _target.mid(1);

        if (_target.startsWith('$') || _target.startsWith('#'))
            _target = nickFromMask(sender);
    }

    _bufferType = bufferTypeByTarget(_target);

    if (timestamp.isValid())
        setTimestamp(timestamp);
    else
        setTimestamp(QDateTime::currentDateTime());
}


MessageEvent::MessageEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : NetworkEvent(type, map, network)
{
    _msgType = static_cast<Message::Type>(map.take("messageType").toInt());
    _msgFlags = static_cast<Message::Flags>(map.take("messageFlags").toInt());
    _bufferType = static_cast<BufferInfo::Type>(map.take("bufferType").toInt());
    _text = map.take("text").toString();
    _sender = map.take("sender").toString();
    _target = map.take("target").toString();
}


void MessageEvent::toVariantMap(QVariantMap &map) const
{
    NetworkEvent::toVariantMap(map);
    map["messageType"] = msgType();
    map["messageFlags"] = (int)msgFlags();
    map["bufferType"] = bufferType();
    map["text"] = text();
    map["sender"] = sender();
    map["target"] = target();
}


BufferInfo::Type MessageEvent::bufferTypeByTarget(const QString &target) const
{
    if (target.isEmpty())
        return BufferInfo::StatusBuffer;

    if (network()->isChannelName(target))
        return BufferInfo::ChannelBuffer;

    return BufferInfo::QueryBuffer;
}

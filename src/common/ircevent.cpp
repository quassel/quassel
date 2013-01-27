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

#include "ircevent.h"

Event *IrcEvent::create(EventManager::EventType type, QVariantMap &map, Network *network)
{
    if ((type & EventManager::IrcEventNumericMask) == EventManager::IrcEventNumeric)
        return new IrcEventNumeric(type, map, network);

    if ((type & EventManager::EventGroupMask) != EventManager::IrcEvent)
        return 0;

    switch (type) {
    case EventManager::IrcEventRawPrivmsg:
    case EventManager::IrcEventRawNotice:
        return new IrcEventRawMessage(type, map, network);

    default:
        return new IrcEvent(type, map, network);
    }
}


IrcEvent::IrcEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : NetworkEvent(type, map, network)
{
    _prefix = map.take("prefix").toString();
    _params = map.take("params").toStringList();
}


void IrcEvent::toVariantMap(QVariantMap &map) const
{
    NetworkEvent::toVariantMap(map);
    map["prefix"] = prefix();
    map["params"] = params();
}


IrcEventNumeric::IrcEventNumeric(EventManager::EventType type, QVariantMap &map, Network *network)
    : IrcEvent(type, map, network)
{
    _number = map.take("number").toUInt();
    _target = map.take("target").toString();
}


void IrcEventNumeric::toVariantMap(QVariantMap &map) const
{
    IrcEvent::toVariantMap(map);
    map["number"] = number();
    map["target"] = target();
}


IrcEventRawMessage::IrcEventRawMessage(EventManager::EventType type, QVariantMap &map, Network *network)
    : IrcEvent(type, map, network)
{
    _rawMessage = map.take("rawMessage").toByteArray();
}


void IrcEventRawMessage::toVariantMap(QVariantMap &map) const
{
    IrcEvent::toVariantMap(map);
    map["rawMessage"] = rawMessage();
}

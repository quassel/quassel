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

#include "networkevent.h"

Event *NetworkEvent::create(EventManager::EventType type, QVariantMap &map, Network *network)
{
    switch (type) {
    case EventManager::NetworkIncoming:
        return new NetworkDataEvent(type, map, network);

    case EventManager::NetworkConnecting:
    case EventManager::NetworkInitializing:
    case EventManager::NetworkInitialized:
    case EventManager::NetworkReconnecting:
    case EventManager::NetworkDisconnecting:
    case EventManager::NetworkDisconnected:
        return new NetworkConnectionEvent(type, map, network);

    case EventManager::NetworkSplitJoin:
    case EventManager::NetworkSplitQuit:
        return new NetworkSplitEvent(type, map, network);

    default:
        return 0;
    }
}


NetworkEvent::NetworkEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : Event(type, map)
    , _network(network)
{
}


void NetworkEvent::toVariantMap(QVariantMap &map) const
{
    Event::toVariantMap(map);
    map["network"] = networkId().toInt();
}


NetworkDataEvent::NetworkDataEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : NetworkEvent(type, map, network)
{
    _data = map.take("data").toByteArray();
}


void NetworkDataEvent::toVariantMap(QVariantMap &map) const
{
    NetworkEvent::toVariantMap(map);
    map["data"] = data();
}


NetworkConnectionEvent::NetworkConnectionEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : NetworkEvent(type, map, network)
{
    _state = static_cast<Network::ConnectionState>(map.take("state").toInt()); // FIXME: check enum plausibility
}


void NetworkConnectionEvent::toVariantMap(QVariantMap &map) const
{
    NetworkEvent::toVariantMap(map);
    map["state"] = connectionState();
}


NetworkSplitEvent::NetworkSplitEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : NetworkEvent(type, map, network)
{
    _channel = map.take("channel").toString();
    _users = map.take("users").toStringList();
    _quitMsg = map.take("quitMessage").toString();
}


void NetworkSplitEvent::toVariantMap(QVariantMap &map) const
{
    NetworkEvent::toVariantMap(map);
    map["channel"] = channel();
    map["users"] = users();
    map["quitMessage"] = quitMessage();
}

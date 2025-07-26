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

#include "networkevent.h"

#include <QDebug>
#include <QVariantMap>

NetworkEvent::NetworkEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent)
    : Event(type, map, parent)
    , _network(network)
{
}

void NetworkEvent::toVariantMap(QVariantMap& map) const
{
    Event::toVariantMap(map);
    map["network"] = network() ? network()->networkId().toInt() : 0;
}

Event* NetworkEvent::create(EventManager::EventType type, QVariantMap& map, Network* network)
{
    switch (type) {
    case EventManager::NetworkConnecting:
    case EventManager::NetworkInitializing:
    case EventManager::NetworkInitialized:
    case EventManager::NetworkReconnecting:
    case EventManager::NetworkDisconnecting:
    case EventManager::NetworkDisconnected:
        return new NetworkConnectionEvent(type, map, network);
    case EventManager::NetworkIncoming:
        return new NetworkDataEvent(type, map, network);
    case EventManager::NetworkSplitJoin:
    case EventManager::NetworkSplitQuit:
        return new NetworkSplitEvent(type, map, network);
    default:
        return new NetworkEvent(type, map, network);
    }
}

NetworkDataEvent::NetworkDataEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent)
    : NetworkEvent(type, map, network, parent)
    , _data(map.value("data").toByteArray())
    , _networkPtr(network)
{
    if (!network) {
        qWarning() << Q_FUNC_INFO << "NetworkDataEvent created with null network pointer!";
    }
}

void NetworkDataEvent::toVariantMap(QVariantMap& map) const
{
    NetworkEvent::toVariantMap(map);
    map["data"] = _data;
}

NetworkConnectionEvent::NetworkConnectionEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent)
    : NetworkEvent(type, map, network, parent)
    , _state(static_cast<Network::ConnectionState>(map.value("connectionState").toInt()))
{
}

void NetworkConnectionEvent::toVariantMap(QVariantMap& map) const
{
    NetworkEvent::toVariantMap(map);
    map["connectionState"] = static_cast<int>(_state);
}

NetworkSplitEvent::NetworkSplitEvent(EventManager::EventType type, QVariantMap& map, Network* network, QObject* parent)
    : NetworkEvent(type, map, network, parent)
    , _channel(map.value("channel").toString())
    , _users(map.value("users").toStringList())
    , _quitMsg(map.value("quitMessage").toString())
{
}

void NetworkSplitEvent::toVariantMap(QVariantMap& map) const
{
    NetworkEvent::toVariantMap(map);
    map["channel"] = _channel;
    map["users"] = _users;
    map["quitMessage"] = _quitMsg;
}

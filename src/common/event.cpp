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

#include "ctcpevent.h"
#include "ircevent.h"
#include "networkevent.h"
#include "messageevent.h"

Event::Event(EventManager::EventType type)
    : _type(type)
    , _valid(true)
{
}


Event::Event(EventManager::EventType type, QVariantMap &map)
    : _type(type)
    , _valid(true)
{
    if (!map.contains("flags") || !map.contains("timestamp")) {
        qWarning() << "Received invalid serialized event:" << map;
        setValid(false);
        return;
    }

    setFlags(static_cast<EventManager::EventFlags>(map.take("flags").toInt())); // TODO sanity check?
    setTimestamp(QDateTime::fromTime_t(map.take("timestamp").toUInt()));
}


void Event::toVariantMap(QVariantMap &map) const
{
    map["type"] = static_cast<int>(type());
    map["flags"] = static_cast<int>(flags());
    map["timestamp"] = timestamp().toTime_t();
}


QVariantMap Event::toVariantMap() const
{
    QVariantMap map;
    toVariantMap(map);
    return map;
}


Event *Event::fromVariantMap(QVariantMap &map, Network *network)
{
    int inttype = map.take("type").toInt();
    // sanity check if we have a valid enum value
    if (EventManager::enumName(inttype).isEmpty()) {
        qWarning() << "Received a serialized event with unknown type" << inttype;
        return 0;
    }

    EventManager::EventType type = static_cast<EventManager::EventType>(inttype);
    if (type == EventManager::Invalid || type == EventManager::GenericEvent)
        return 0;

    EventManager::EventType group = static_cast<EventManager::EventType>(type & EventManager::EventGroupMask);

    Event *e = 0;

    // we use static create() functions to keep group-specific special cases in the files they belong
    // e.g. IrcEventRawMessage
    switch (group) {
    case EventManager::NetworkEvent:
        e = NetworkEvent::create(type, map, network);
        break;
    case EventManager::IrcServerEvent:
        // not in use!
        break;
    case EventManager::IrcEvent:
        e = IrcEvent::create(type, map, network);
        break;
    case EventManager::MessageEvent:
        e = MessageEvent::create(type, map, network);
        break;
    case EventManager::CtcpEvent:
        e = CtcpEvent::create(type, map, network);
        break;
    default:
        break;
    }

    if (!e) {
        qWarning() << "Can't create event of type" << type;
        return 0;
    }

    if (!map.isEmpty()) {
        qWarning() << "Event creation from map did not consume all data:" << map;
    }

    return e;
}


QDebug operator<<(QDebug dbg, Event *e)
{
    dbg.nospace() << qPrintable(e->className()) << "("
    << "type = 0x" << qPrintable(QString::number(e->type(), 16));
    e->debugInfo(dbg);
    //<< ", data = " << e->data(); // we don't use data anywhere yet
    dbg.nospace() << ", flags = 0x" << qPrintable(QString::number(e->flags(), 16))
    << ")";
    return dbg.space();
}

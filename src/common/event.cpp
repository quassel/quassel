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

#include "event.h"

#include <QDebug>
#include <QVariantMap>

#include "ctcpevent.h"
#include "ircevent.h"
#include "messageevent.h"
#include "networkevent.h"
#include "peer.h"
#include "signalproxy.h"

Event::Event(EventManager::EventType type, QObject* parent)
    : QObject(parent)
    , _type(type)
    , _flags(0)
    , _timestamp(QDateTime::currentDateTimeUtc())
    , _valid(true)
{
}

Event::Event(EventManager::EventType type, QVariantMap& map, QObject* parent)
    : QObject(parent)
    , _type(type)
    , _flags(static_cast<EventManager::EventFlags>(map.value("flags").toInt()))
    , _timestamp(QDateTime::fromMSecsSinceEpoch(map.value("timestamp").toLongLong()))
    , _valid(true)
{
    if (!map.contains("flags") || !map.contains("timestamp")) {
        qWarning() << "Received invalid serialized event:" << map;
        setValid(false);
        return;
    }

    Q_ASSERT(SignalProxy::current());
    Q_ASSERT(SignalProxy::current()->sourcePeer());

    if (!SignalProxy::current()->sourcePeer()->hasFeature(Quassel::Feature::LongTime)) {
        _timestamp = QDateTime::fromSecsSinceEpoch(map.value("timestamp").toUInt());
    }
}

void Event::toVariantMap(QVariantMap& map) const
{
    Q_ASSERT(SignalProxy::current());
    Q_ASSERT(SignalProxy::current()->targetPeer());

    map["type"] = static_cast<int>(type());
    map["flags"] = static_cast<int>(flags());
    if (SignalProxy::current()->targetPeer()->hasFeature(Quassel::Feature::LongTime)) {
        map["timestamp"] = timestamp().toMSecsSinceEpoch();
    }
    else {
        map["timestamp"] = timestamp().toSecsSinceEpoch();
    }
}

QVariantMap Event::toVariantMap() const
{
    QVariantMap map;
    toVariantMap(map);
    return map;
}

Event* Event::fromVariantMap(QVariantMap& map, Network* network)
{
    int inttype = map.take("type").toInt();
    if (EventManager::enumName(inttype).isEmpty()) {
        qWarning() << "Received a serialized event with unknown type" << inttype;
        return nullptr;
    }

    auto type = static_cast<EventManager::EventType>(inttype);
    if (type == EventManager::Invalid || type == EventManager::GenericEvent)
        return nullptr;

    auto group = static_cast<EventManager::EventType>(type & EventManager::EventGroupMask);

    Event* e = nullptr;

    switch (group) {
    case EventManager::NetworkEvent:
        e = NetworkEvent::create(type, map, network);
        break;
    case EventManager::IrcServerEvent:
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
        return nullptr;
    }

    if (!map.isEmpty()) {
        qWarning() << "Event creation from map did not consume all data:" << map;
    }

    return e;
}

QDebug operator<<(QDebug dbg, Event* e)
{
    if (!e)
        return dbg << "Event(nullptr)";
    dbg.nospace() << qPrintable(e->className()) << "(type = 0x" << qPrintable(QString::number(e->type(), 16));
    e->debugInfo(dbg);
    dbg.nospace() << ", flags = 0x" << qPrintable(QString::number(e->flags(), 16)) << ")";
    return dbg.space();
}

/***************************************************************************
 *   Copyright (C) 2013 by the Quassel Project                             *
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

#include "keyevent.h"

Event *KeyEvent::create(EventManager::EventType type, QVariantMap &map, Network *network)
{
    if (type == EventManager::KeyEvent)
        return new KeyEvent(type, map, network);

    return 0;
}


KeyEvent::KeyEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : IrcEvent(type, map, network)
{
    _exchangeType = static_cast<ExchangeType>(map.take("exchangeType").toInt());
    _target = map.take("target").toString();
    _key = map.take("key").toByteArray();
}


void KeyEvent::toVariantMap(QVariantMap &map) const
{
    IrcEvent::toVariantMap(map);
    map["exchangeType"] = exchangeType();
    map["target"] = target();
    map["key"] = key();
}

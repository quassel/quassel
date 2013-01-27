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

Event *CtcpEvent::create(EventManager::EventType type, QVariantMap &map, Network *network)
{
    if (type == EventManager::CtcpEvent || type == EventManager::CtcpEventFlush)
        return new CtcpEvent(type, map, network);

    return 0;
}


CtcpEvent::CtcpEvent(EventManager::EventType type, QVariantMap &map, Network *network)
    : IrcEvent(type, map, network)
{
    _ctcpType = static_cast<CtcpType>(map.take("ctcpType").toInt());
    _ctcpCmd = map.take("ctcpCmd").toString();
    _target = map.take("target").toString();
    _param = map.take("param").toString();
    _reply = map.take("repy").toString();
    _uuid = map.take("uuid").toString();
}


void CtcpEvent::toVariantMap(QVariantMap &map) const
{
    IrcEvent::toVariantMap(map);
    map["ctcpType"] = ctcpType();
    map["ctcpCmd"] = ctcpCmd();
    map["target"] = target();
    map["param"] = param();
    map["reply"] = reply();
    map["uuid"] = uuid().toString();
}

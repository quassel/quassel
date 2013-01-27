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

#include "buffersettings.h"

BufferSettings::BufferSettings(BufferId bufferId)
    : ClientSettings(QString("Buffer/%1").arg(bufferId.toInt()))
{
}


BufferSettings::BufferSettings(const QString &idString)
    : ClientSettings(QString("Buffer/%1").arg(idString))
{
}


void BufferSettings::filterMessage(Message::Type msgType, bool filter)
{
    if (!hasFilter())
        setLocalValue("hasMessageTypeFilter", true);
    if (filter)
        setLocalValue("MessageTypeFilter", localValue("MessageTypeFilter", 0).toInt() | msgType);
    else
        setLocalValue("MessageTypeFilter", localValue("MessageTypeFilter", 0).toInt() & ~msgType);
}


void BufferSettings::setMessageFilter(int filter)
{
    if (!hasFilter())
        setLocalValue("hasMessageTypeFilter", true);
    setLocalValue("MessageTypeFilter", filter);
}


void BufferSettings::removeFilter()
{
    setLocalValue("hasMessageTypeFilter", false);
    removeLocalKey("MessageTypeFilter");
}

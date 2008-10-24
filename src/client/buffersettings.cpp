/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
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

bool BufferSettings::hasFilter() {
  return localValue("hasMessageTypeFilter", false).toBool();
}

int BufferSettings::messageFilter() {
  return localValue("MessageTypeFilter", 0).toInt();
}

void BufferSettings::filterMessage(Message::Type msgType, bool filter) {
  if(!hasFilter())
    setLocalValue("hasMessageTypeFilter", true);
  if(filter)
    setLocalValue("MessageTypeFilter", localValue("MessageTypeFilter", 0).toInt() | msgType);
  else
    setLocalValue("MessageTypeFilter", localValue("MessageTypeFilter", 0).toInt() & ~msgType);
}

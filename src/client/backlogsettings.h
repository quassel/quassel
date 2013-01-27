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

#ifndef BACKLOGSETTINGS_H
#define BACKLOGSETTINGS_H

#include "clientsettings.h"

class BacklogSettings : public ClientSettings
{
public:
    BacklogSettings() : ClientSettings("Backlog") {}
    inline int requesterType() { return localValue("RequesterType", 1).toInt(); }
    inline void setRequesterType(int requesterType) { setLocalValue("RequesterType", requesterType); }

    inline int dynamicBacklogAmount() { return localValue("DynamicBacklogAmount", 200).toInt(); }
    inline void setDynamicBacklogAmount(int amount) { return setLocalValue("DynamicBacklogAmount", amount); }

    inline int fixedBacklogAmount() { return localValue("FixedBacklogAmount", 500).toInt(); }
    inline void setFixedBacklogAmount(int amount) { return setLocalValue("FixedBacklogAmount", amount); }

    inline int globalUnreadBacklogLimit() { return localValue("GlobalUnreadBacklogLimit", 5000).toInt(); }
    inline void setGlobalUnreadBacklogLimit(int limit) { return setLocalValue("GlobalUnreadBacklogLimit", limit); }
    inline int globalUnreadBacklogAdditional() { return localValue("GlobalUnreadBacklogAdditional", 100).toInt(); }
    inline void setGlobalUnreadBacklogAdditional(int Additional) { return setLocalValue("GlobalUnreadBacklogAdditional", Additional); }

    inline int perBufferUnreadBacklogLimit() { return localValue("PerBufferUnreadBacklogLimit", 200).toInt(); }
    inline void setPerBufferUnreadBacklogLimit(int limit) { return setLocalValue("PerBufferUnreadBacklogLimit", limit); }
    inline int perBufferUnreadBacklogAdditional() { return localValue("PerBufferUnreadBacklogAdditional", 50).toInt(); }
    inline void setPerBufferUnreadBacklogAdditional(int Additional) { return setLocalValue("PerBufferUnreadBacklogAdditional", Additional); }
};


#endif //BACKLOGSETTINGS_H

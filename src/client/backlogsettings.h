/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#pragma once

#include "client-export.h"

#include "backlogrequester.h"
#include "clientsettings.h"

class CLIENT_EXPORT BacklogSettings : public ClientSettings
{
public:
    BacklogSettings();
    int requesterType() const;
    // Default to PerBufferUnread to help work around performance problems on connect when there's
    // many buffers that don't have much activity.
    void setRequesterType(int requesterType);

    int dynamicBacklogAmount() const;
    void setDynamicBacklogAmount(int amount);

    /**
     * Gets if a buffer should fetch backlog upon show to provide a scrollable amount of backlog
     *
     * @return True if showing a buffer without scrollbar visible fetches backlog, otherwise false
     */
    bool ensureBacklogOnBufferShow() const;
    /**
     * Sets if a buffer should fetch backlog upon show to provide a scrollable amount of backlog
     *
     * @param enabled True if showing a buffer without scrollbar fetches backlog, otherwise false
     */
    void setEnsureBacklogOnBufferShow(bool enabled);

    int fixedBacklogAmount() const;
    void setFixedBacklogAmount(int amount);

    int globalUnreadBacklogLimit() const;
    void setGlobalUnreadBacklogLimit(int limit);
    int globalUnreadBacklogAdditional() const;
    void setGlobalUnreadBacklogAdditional(int additional);

    int perBufferUnreadBacklogLimit() const;
    void setPerBufferUnreadBacklogLimit(int limit);
    int perBufferUnreadBacklogAdditional() const;
    void setPerBufferUnreadBacklogAdditional(int additional);
};

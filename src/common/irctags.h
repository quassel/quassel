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

#include "irctag.h"

/**
 * This namespace contains commonly used message tags, similar to the IrcCaps
 * namespace used for IRCv3 capabilities.
 */
namespace IrcTags
{
    /**
     * Services account status with user messages
     *
     * https://ircv3.net/specs/extensions/account-tag-3.2
     */
    const IrcTagKey ACCOUNT = IrcTagKey{"", "account", false};

    /**
     * Server time for messages.
     *
     * https://ircv3.net/specs/extensions/server-time-3.2.html
     */
    const IrcTagKey SERVER_TIME = IrcTagKey{"", "time", false};

    /**
     * Message Batches.
     *
     * https://ircv3.net/specs/extensions/batch
     */
    const IrcTagKey BATCH = IrcTagKey{"", "batch", false};
}

/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "client-export.h"

#include "clientsettings.h"
#include "message.h"
#include "types.h"

class CLIENT_EXPORT BufferSettings : public ClientSettings
{
public:
    enum RedirectTarget
    {
        DefaultBuffer = 0x01,
        StatusBuffer = 0x02,
        CurrentBuffer = 0x04
    };

    BufferSettings(const QString& idString = "__default__");
    BufferSettings(BufferId bufferId);

    void setValue(const QString& key, const QVariant& data);
    QVariant value(const QString& key, const QVariant& def = {}) const;

    // Message Filter (default and per view)
    bool hasFilter() const;
    int messageFilter() const;
    void setMessageFilter(int filter);
    void filterMessage(Message::Type msgType, bool filter);
    void removeFilter();

    // user state icons for query buffers (default)
    bool showUserStateIcons() const;
    void enableUserStateIcons(bool enabled);

    // redirection settings (default)
    int userNoticesTarget() const;
    void setUserNoticesTarget(int target);
    int serverNoticesTarget() const;
    void setServerNoticesTarget(int target);
    int errorMsgsTarget() const;
    void setErrorMsgsTarget(int target);
};

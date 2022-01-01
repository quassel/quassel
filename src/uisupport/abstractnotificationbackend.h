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

#pragma once

#include "uisupport-export.h"

#include <utility>

#include <QObject>
#include <QString>

#include "bufferinfo.h"

class SettingsPage;

class UISUPPORT_EXPORT AbstractNotificationBackend : public QObject
{
    Q_OBJECT

public:
    enum NotificationType
    {
        Highlight = 0x01,
        PrivMsg = 0x02,
        HighlightFocused = 0x11,
        PrivMsgFocused = 0x12
    };

    struct Notification
    {
        uint notificationId;
        BufferId bufferId;
        NotificationType type;
        QString sender;
        QString message;

        Notification(uint id_, BufferId buf_, NotificationType type_, QString sender_, QString msg_)
            : notificationId(id_)
            , bufferId(buf_)
            , type(type_)
            , sender(std::move(sender_))
            , message(std::move(msg_))
        {}
    };

    using QObject::QObject;

    virtual void notify(const Notification&) = 0;
    virtual void close(uint notificationId) { Q_UNUSED(notificationId); }

    //! Factory to create a configuration widget suitable for a specific notification backend
    /**
     * AbstractNotification will not take ownership of that configWidget!
     * In case you need to communicate with the configWidget directly, make your connections here
     */
    virtual SettingsPage* createConfigWidget() const = 0;

signals:
    //! May be emitted by the notification to tell the MainWin to raise itself
    void activated(uint notificationId = 0);
};

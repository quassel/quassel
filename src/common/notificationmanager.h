/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "common-export.h"

#include <utility>

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "expressionmatch.h"
#include "message.h"
#include "nickhighlightmatcher.h"
#include "syncableobject.h"

class COMMON_EXPORT NotificationManager : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

    Q_PROPERTY(int notificationSettingQuery READ notificationSettingQuery WRITE setNotificationSettingQuery)
    Q_PROPERTY(int notificationSettingChannel READ notificationSettingChannel WRITE setNotificationSettingChannel)
    Q_PROPERTY(int notificationSettingStatus READ notificationSettingStatus WRITE setNotificationSettingStatus)

public:
    enum NotificationSetting : int {
        Default = 0x00,
        None = 0x01,
        Highlight = 0x02,
        All = 0x03
    };

    inline NotificationManager(QObject* parent = nullptr)
        : SyncableObject(parent)
    {
        setAllowClientUpdates(true);
    }

    inline int notificationSettingQuery() { return _notificationSettingQuery; }
    inline int notificationSettingChannel() { return _notificationSettingChannel; }
    inline int notificationSettingStatus() { return _notificationSettingStatus; }

public slots:

    virtual inline void requestSetNotificationSettingQuery(int notificationSetting) { REQUEST(ARG(notificationSetting)) }

    inline void setNotificationSettingQuery(int notificationSetting)
    {
        _notificationSettingQuery = static_cast<NotificationManager::NotificationSetting>(notificationSetting);
    }

    virtual inline void requestSetNotificationSettingChannel(int notificationSetting) { REQUEST(ARG(notificationSetting)) }

    inline void setNotificationSettingChannel(int notificationSetting)
    {
        _notificationSettingChannel = static_cast<NotificationManager::NotificationSetting>(notificationSetting);
    }

    virtual inline void requestSetNotificationSettingStatus(int notificationSetting) { REQUEST(ARG(notificationSetting)) }

    inline void setNotificationSettingStatus(int notificationSetting)
    {
        _notificationSettingStatus = static_cast<NotificationManager::NotificationSetting>(notificationSetting);
    }

private:
    NotificationSetting _notificationSettingQuery = NotificationSetting::All;
    NotificationSetting _notificationSettingChannel = NotificationSetting::Highlight;
    NotificationSetting _notificationSettingStatus = NotificationSetting::None;
};

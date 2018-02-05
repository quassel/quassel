/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QIcon>
#include <QHBoxLayout>

#include "systrayanimationnotificationbackend.h"

#include "client.h"
#include "clientsettings.h"
#include "mainwin.h"
#include "networkmodel.h"
#include "qtui.h"
#include "systemtray.h"

SystrayAnimationNotificationBackend::SystrayAnimationNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent)
{
    NotificationSettings notificationSettings;
    notificationSettings.initAndNotify("Systray/Animate", this, SLOT(animateChanged(QVariant)), true);
}


void SystrayAnimationNotificationBackend::notify(const Notification &n)
{
    if (n.type != Highlight && n.type != PrivMsg)
        return;

    if (_animate)
        QtUi::mainWindow()->systemTray()->setAlert(true);
}


void SystrayAnimationNotificationBackend::close(uint notificationId)
{
    QtUi::mainWindow()->systemTray()->setAlert(false);
}


void SystrayAnimationNotificationBackend::animateChanged(const QVariant &v)
{
    _animate = v.toBool();
}


SettingsPage *SystrayAnimationNotificationBackend::createConfigWidget() const
{
    return nullptr;
}
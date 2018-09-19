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

#include "systrayanimationnotificationbackend.h"

#include "clientsettings.h"
#include "icon.h"
#include "mainwin.h"
#include "qtui.h"
#include "systemtray.h"

SystrayAnimationNotificationBackend::SystrayAnimationNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent)
{
    NotificationSettings notificationSettings;
    notificationSettings.initAndNotify("Systray/Alert", this, &SystrayAnimationNotificationBackend::alertChanged, true);
}


void SystrayAnimationNotificationBackend::notify(const Notification &n)
{
    if (n.type != Highlight && n.type != PrivMsg)
        return;

    if (_alert)
        QtUi::mainWindow()->systemTray()->setAlert(true);
}


void SystrayAnimationNotificationBackend::close(uint notificationId)
{
    Q_UNUSED(notificationId)
    QtUi::mainWindow()->systemTray()->setAlert(false);
}


void SystrayAnimationNotificationBackend::alertChanged(const QVariant &v)
{
    _alert = v.toBool();
}


SettingsPage *SystrayAnimationNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


/***************************************************************************/

SystrayAnimationNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "SystrayAnimation", parent)
{
    ui.setupUi(this);
    ui.enableAlert->setIcon(icon::get("dialog-information"));

    ui.attentionBehavior->setEnabled(ui.enableAlert->isChecked());

    initAutoWidgets();
}


QString SystrayAnimationNotificationBackend::ConfigWidget::settingsKey() const
{
    return "Notification";
}


QVariant SystrayAnimationNotificationBackend::ConfigWidget::loadAutoWidgetValue(const QString &widgetName)
{
    if (widgetName == "attentionBehavior") {
        NotificationSettings s;
        if (s.value("Systray/Animate", false).toBool()) {
            return 2;
        }
        if (s.value("Systray/ChangeColor", true).toBool()) {
            return 1;
        }
        return 0;
    }

    return SettingsPage::loadAutoWidgetValue(widgetName);
}


void SystrayAnimationNotificationBackend::ConfigWidget::saveAutoWidgetValue(const QString &widgetName, const QVariant &value)
{
    if (widgetName == "attentionBehavior") {
        NotificationSettings s;
        s.setValue("Systray/ChangeColor", false);
        s.setValue("Systray/Animate", false);
        switch (value.toInt()) {
        case 1:
            s.setValue("Systray/ChangeColor", true);
            return;
        case 2:
            s.setValue("Systray/Animate", true);
            return;
        default:
            return;
        }
    }

    SettingsPage::saveAutoWidgetValue(widgetName, value);
}

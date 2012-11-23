/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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

#include "clientsettings.h"
#include "osxnotificationbackend.h"

#include <QCheckBox>
#include <QHBoxLayout>

#import <Foundation/NSUserNotification.h>

namespace {

void SendNotificationCenterMessage(NSString* title, NSString* subtitle) {
    NSUserNotificationCenter* center =
            [NSUserNotificationCenter defaultUserNotificationCenter];
    NSUserNotification* notification =
            [[NSUserNotification alloc] init];

    [notification setTitle: title];
    [notification setSubtitle: subtitle];

    [center deliverNotification: notification];

    [notification release];
}

}

OSXNotificationBackend::OSXNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent),
      _enabled(true)
{
    NotificationSettings notificationSettings;
    notificationSettings.initAndNotify("OSXNotification/Enabled", this, SLOT(enabledChanged(QVariant)), true);
}

void OSXNotificationBackend::enabledChanged(const QVariant &value)
{
    _enabled = value.toBool();
}

void OSXNotificationBackend::notify(const Notification &notification)
{
    if (!_enabled)
    {
        return;
    }

    NSString* message = [[NSString alloc] initWithUTF8String:notification.sender.toUtf8().constData()];
    NSString* summary = [[NSString alloc] initWithUTF8String:notification.message.toUtf8().constData()];

    SendNotificationCenterMessage(message, summary);

    [message release];
    [summary release];
}

void OSXNotificationBackend::close(uint notificationId)
{
}

SettingsPage *OSXNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}

OSXNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent)
    : SettingsPage("Internal", "OSXNotification", parent)
{
    _enabledBox = new QCheckBox(tr("Show OS X notifications"));
    connect(_enabledBox, SIGNAL(toggled(bool)), this, SLOT(widgetChanged()));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(_enabledBox);
}

void OSXNotificationBackend::ConfigWidget::widgetChanged()
{
    bool changed = (_enabled != _enabledBox->isChecked());
    if (changed != hasChanged())
        setChangedState(changed);
}

bool OSXNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}

void OSXNotificationBackend::ConfigWidget::defaults()
{
    _enabledBox->setChecked(true);
    widgetChanged();
}

void OSXNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    _enabled = s.value("OSXNotification/Enabled", false).toBool();
    _enabledBox->setChecked(_enabled);
    setChangedState(false);
}


void OSXNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("OSXNotification/Enabled", _enabledBox->isChecked());
    load();
}

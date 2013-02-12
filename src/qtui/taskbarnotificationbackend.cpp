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

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QSpinBox>

#include "taskbarnotificationbackend.h"

#include "clientsettings.h"
#include "iconloader.h"
#include "mainwin.h"
#include "qtui.h"

TaskbarNotificationBackend::TaskbarNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent)
{
    NotificationSettings notificationSettings;
    _enabled = notificationSettings.value("Taskbar/Enabled", true).toBool();
    _timeout = notificationSettings.value("Taskbar/Timeout", 0).toInt();

    notificationSettings.notify("Taskbar/Enabled", this, SLOT(enabledChanged(const QVariant &)));
    notificationSettings.notify("Taskbar/Timeout", this, SLOT(timeoutChanged(const QVariant &)));
}


void TaskbarNotificationBackend::notify(const Notification &notification)
{
    if (_enabled && (notification.type == Highlight || notification.type == PrivMsg)) {
        QApplication::alert(QtUi::mainWindow(), _timeout);
    }
}


void TaskbarNotificationBackend::close(uint notificationId)
{
    Q_UNUSED(notificationId);
}


void TaskbarNotificationBackend::enabledChanged(const QVariant &v)
{
    _enabled = v.toBool();
}


void TaskbarNotificationBackend::timeoutChanged(const QVariant &v)
{
    _timeout = v.toInt();
}


SettingsPage *TaskbarNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


/***************************************************************************/

TaskbarNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "TaskbarNotification", parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
#ifdef Q_WS_MAC
    layout->addWidget(enabledBox = new QCheckBox(tr("Activate dock entry, timeout:"), this));
#else
    layout->addWidget(enabledBox = new QCheckBox(tr("Mark taskbar entry, timeout:"), this));
#endif
    enabledBox->setIcon(SmallIcon("flag-blue"));
    enabledBox->setEnabled(true);

    timeoutBox = new QSpinBox(this);
    timeoutBox->setMinimum(0);
    timeoutBox->setMaximum(99);
    timeoutBox->setSpecialValueText(tr("Unlimited"));
    timeoutBox->setSuffix(tr(" seconds"));
    layout->addWidget(timeoutBox);
    layout->addStretch(20);

    connect(enabledBox, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
    connect(enabledBox, SIGNAL(toggled(bool)), timeoutBox, SLOT(setEnabled(bool)));
    connect(timeoutBox, SIGNAL(valueChanged(int)), SLOT(widgetChanged()));
}


void TaskbarNotificationBackend::ConfigWidget::widgetChanged()
{
    bool changed = (enabled != enabledBox->isChecked() || timeout/1000 != timeoutBox->value());
    if (changed != hasChanged()) setChangedState(changed);
}


bool TaskbarNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}


void TaskbarNotificationBackend::ConfigWidget::defaults()
{
    enabledBox->setChecked(true);
    timeoutBox->setValue(0);
    widgetChanged();
}


void TaskbarNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    enabled = s.value("Taskbar/Enabled", true).toBool();
    timeout = s.value("Taskbar/Timeout", 0).toInt();

    enabledBox->setChecked(enabled);
    timeoutBox->setValue(timeout/1000);

    setChangedState(false);
}


void TaskbarNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("Taskbar/Enabled", enabledBox->isChecked());
    s.setValue("Taskbar/Timeout", timeoutBox->value() * 1000);
    load();
}

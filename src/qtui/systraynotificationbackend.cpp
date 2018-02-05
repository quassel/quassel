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

#include "systraynotificationbackend.h"

#include "client.h"
#include "clientsettings.h"
#include "mainwin.h"
#include "networkmodel.h"
#include "qtui.h"
#include "systemtray.h"

SystrayNotificationBackend::SystrayNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent),
    _blockActivation(false)
{
    NotificationSettings notificationSettings;
    notificationSettings.initAndNotify("Systray/ShowBubble", this, SLOT(showBubbleChanged(QVariant)), true);

    connect(QtUi::mainWindow()->systemTray(), SIGNAL(messageClicked(uint)), SLOT(notificationActivated(uint)));
    connect(QtUi::mainWindow()->systemTray(), SIGNAL(activated(SystemTray::ActivationReason)),
        SLOT(notificationActivated(SystemTray::ActivationReason)));

    QApplication::instance()->installEventFilter(this);

    updateToolTip();
}


void SystrayNotificationBackend::notify(const Notification &n)
{
    if (n.type != Highlight && n.type != PrivMsg)
        return;

    _notifications.append(n);
    if (_showBubble) {
        QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
        QString message = QString("<%1> %2").arg(n.sender, n.message);
        QtUi::mainWindow()->systemTray()->showMessage(title, message, SystemTray::Information, 10000, n.notificationId);
    }

    updateToolTip();
}


void SystrayNotificationBackend::close(uint notificationId)
{
    QList<Notification>::iterator i = _notifications.begin();
    while (i != _notifications.end()) {
        if (i->notificationId == notificationId)
            i = _notifications.erase(i);
        else
            ++i;
    }

    QtUi::mainWindow()->systemTray()->closeMessage(notificationId);

    updateToolTip();
}


void SystrayNotificationBackend::notificationActivated(uint notificationId)
{
    if (!_blockActivation) {
        QList<Notification>::iterator i = _notifications.begin();
        while (i != _notifications.end()) {
            if (i->notificationId == notificationId) {
                if (QtUi::mainWindow()->systemTray()->mode() == SystemTray::Legacy)
                    _blockActivation = true;  // prevent double activation because both tray icon and bubble might send a signal
                emit activated(notificationId);
                break;
            }
        ++i;
        }
    }
}


void SystrayNotificationBackend::notificationActivated(SystemTray::ActivationReason reason)
{
    if (reason == SystemTray::Trigger) {
        if (_notifications.count())
            notificationActivated(_notifications.last().notificationId);
        else
            GraphicalUi::toggleMainWidget();
    }
}


// moving the mouse or releasing the button means that we're not dealing with a double activation
bool SystrayNotificationBackend::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) {
        _blockActivation = false;
    }
    return AbstractNotificationBackend::eventFilter(obj, event);
}


void SystrayNotificationBackend::showBubbleChanged(const QVariant &v)
{
    _showBubble = v.toBool();
}


void SystrayNotificationBackend::updateToolTip()
{
    QtUi::mainWindow()->systemTray()->setToolTip("Quassel IRC",
        _notifications.count() ? tr("%n pending highlight(s)", "", _notifications.count()) : QString());
}


SettingsPage *SystrayNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


/***************************************************************************/

SystrayNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "SystrayNotification", parent)
{
    _showBubbleBox = new QCheckBox(tr("Show a message in a popup"));
    _showBubbleBox->setIcon(QIcon::fromTheme("dialog-information"));
    connect(_showBubbleBox, SIGNAL(toggled(bool)), this, SLOT(widgetChanged()));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(_showBubbleBox);
}


void SystrayNotificationBackend::ConfigWidget::widgetChanged()
{
    bool changed = (_showBubble != _showBubbleBox->isChecked());
    if (changed != hasChanged())
        setChangedState(changed);
}


bool SystrayNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}


void SystrayNotificationBackend::ConfigWidget::defaults()
{
    _showBubbleBox->setChecked(false);
    widgetChanged();
}


void SystrayNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    _showBubble = s.value("Systray/ShowBubble", false).toBool();
    _showBubbleBox->setChecked(_showBubble);
    setChangedState(false);
}


void SystrayNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("Systray/ShowBubble", _showBubbleBox->isChecked());
    load();
}

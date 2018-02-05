/***************************************************************************
 *   Copyright (C) 2011-2016 by Hannah von Reth                            *
 *   vonreth@kde.org                                                       *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "snorenotificationbackend.h"

#include <QtGui>
#include <QtGlobal>
#include <QMetaObject>

#include "client.h"
#include "networkmodel.h"
#include "systraynotificationbackend.h"
#include "qtui.h"

#include <iostream>


#include <libsnore/snore.h>
#include <libsnore/notification/notification.h>


SnoreNotificationBackend::SnoreNotificationBackend (QObject *parent)
    : AbstractNotificationBackend(parent),
      m_icon(QIcon::fromTheme("quassel", QIcon(":/icons/quassel.png")))
{

    Snore::SnoreCore::instance().loadPlugins(
#ifndef HAVE_KDE
                Snore::SnorePlugin::Backend |
#endif
                Snore::SnorePlugin::SecondaryBackend | Snore::SnorePlugin::Settings);
    m_application = Snore::Application("Quassel", m_icon);
    m_application.hints().setValue("windows-app-id","QuasselProject.QuasselIRC");
    m_application.hints().setValue("pushover-token", "arNtsi983QSZUqU3KAZrFLKHGFPkdL");

    connect(&Snore::SnoreCore::instance(), SIGNAL(actionInvoked(Snore::Notification)), this, SLOT(actionInvoked(Snore::Notification)));


    m_alert = Snore::Alert(tr("Private Message"), m_icon);
    m_application.addAlert(m_alert);
    Snore::SnoreCore::instance().setDefaultApplication(m_application);

    NotificationSettings notificationSettings;
    bool enabled = notificationSettings.value("Snore/Enabled", false).toBool();
    setTraybackend(enabled);
    notificationSettings.notify("Snore/Enabled", this, SLOT(setTraybackend(const QVariant &)));
}

SnoreNotificationBackend::~SnoreNotificationBackend()
{
    Snore::SnoreCore::instance().deregisterApplication(m_application);
}

void SnoreNotificationBackend::notify(const Notification &n)
{
#ifndef HAVE_KDE
    if (m_systrayBackend != nullptr) {
        return;
    }
#endif
    QString title =  QString("%1 - %2").arg(Client::networkModel()->networkName(n.bufferId), Client::networkModel()->bufferName(n.bufferId));
    QString message = QString("<%1> %2").arg(n.sender, n.message);
    Snore::Notification noti(m_application, m_alert, title, message, m_icon);
    noti.hints().setValue("QUASSEL_ID", n.notificationId);
    m_notificationIds.insert(n.notificationId, noti.id());
    Snore::SnoreCore::instance().broadcastNotification(noti);
}

void SnoreNotificationBackend::close(uint notificationId)
{
#ifndef HAVE_KDE
    if (m_systrayBackend != nullptr) {
        return;
    }
#endif
    Snore::Notification n = Snore::SnoreCore::instance().getActiveNotificationByID(m_notificationIds.take(notificationId));
    if (n.isValid()) { // Don't close the notification if it no longer exists.
        Snore::SnoreCore::instance().requestCloseNotification(n, Snore::Notification::Closed);
    }
}

void SnoreNotificationBackend::actionInvoked(Snore::Notification n)
{
    emit activated(n.hints().value("QUASSEL_ID").toUInt());
}

SettingsPage *SnoreNotificationBackend::createConfigWidget()const
{
    return new ConfigWidget();
}


void SnoreNotificationBackend::setTraybackend(const QVariant &b)
{
#ifndef HAVE_KDE
    if (m_systrayBackend == nullptr) {
        m_systrayBackend = new SystrayNotificationBackend(this);
        QtUi::registerNotificationBackend(m_systrayBackend);
    }
#endif
    if (b.toBool()) {
        if (!Snore::SnoreCore::instance().aplications().contains(m_application.name())) {
            Snore::SnoreCore::instance().registerApplication(m_application);
        }
    } else {
        if (Snore::SnoreCore::instance().aplications().contains(m_application.name())) {
            Snore::SnoreCore::instance().deregisterApplication(m_application);
        }
    }
}

/***************************************************************************/

SnoreNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent)
    :SettingsPage("Internal", "SnoreNotification", parent)
{
    ui.setupUi(this);
    connect(ui.useSnoreCheckBox, SIGNAL(toggled(bool)), this, SLOT(useSnnoreChanged(bool)));
}

bool SnoreNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}

void SnoreNotificationBackend::ConfigWidget::defaults()
{
    useSnnoreChanged(false);
    ui.widget->reset();
}

void SnoreNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    bool enabled = s.value("Snore/Enabled", false).toBool();
    ui.useSnoreCheckBox->setChecked(enabled);
    ui.widget->setEnabled(enabled);
    setChangedState(false);
    QMetaObject::invokeMethod(this, "changed", Qt::QueuedConnection);//hack to make apply and accept button work for snore settings widget
}

void SnoreNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("Snore/Enabled", ui.useSnoreCheckBox->isChecked());
    ui.widget->accept();
    load();
}

void SnoreNotificationBackend::ConfigWidget::useSnnoreChanged(bool b)
{
    ui.useSnoreCheckBox->setChecked(b);
    ui.widget->setEnabled(b);
    setChangedState(true);
}



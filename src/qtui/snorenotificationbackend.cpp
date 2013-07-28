/***************************************************************************
*   Copyright (C) 2011 by the Patrick von Reth                            *
*   patrick.vonreth@gmail.com                                             *
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
#include <QDebug>

#include "client.h"
#include "iconloader.h"
#include "networkmodel.h"
#include "systraynotificationbackend.h"
#include "qtui.h"

#include <iostream>


#include <snore/core/snore.h>
#include <snore/core/notification/notification.h>


SnoreNotificationBackend::SnoreNotificationBackend (QObject *parent)
    :AbstractNotificationBackend(parent),
      m_systrayBackend(NULL)
{
    NotificationSettings notificationSettings;
    QString backend = notificationSettings.value("Snore/Backend", "SystemTray").toString();
    m_timeout = notificationSettings.value("Snore/Timeout",10).toInt();

    notificationSettings.notify("Snore/Backend", this, SLOT(backendChanged(const QVariant &)));
    notificationSettings.notify("Snore/Backend", this, SLOT(timeoutChanged(const QVariant &)));

    //TODO: try to get an instance of the tray icon to be able to show popups
    m_snore = new Snore::SnoreCore();
    m_snore->hints().setValue("WINDOWS_APP_ID","QuasselProject.QuasselIRC");
    m_snore->loadPlugins(Snore::PluginContainer::BACKEND);
    Snore::Application *a = new Snore::Application("Quassel",Snore::Icon(DesktopIcon("quassel").toImage()));

    connect(m_snore,SIGNAL(actionInvoked(Snore::Notification)),this,SLOT(actionInvoked(Snore::Notification)));

    m_icon = Snore::Icon(DesktopIcon("dialog-information").toImage());

    a->addAlert(new Snore::Alert(tr("Private Message"),tr("Private Message")));

    m_snore->addApplication(a);
    m_snore->applicationIsInitialized (a);

    m_snore->setPrimaryNotificationBackend(backend);
}

SnoreNotificationBackend::~SnoreNotificationBackend(){
    m_snore->removeApplication("Quassel");
    m_snore->deleteLater();
}

void SnoreNotificationBackend::backendChanged(const QVariant &v){
    QString backend = v.toString();
    if(backend == "SystemTray"){
        if(m_systrayBackend == NULL){
            m_systrayBackend = new SystrayNotificationBackend(this);
            QtUi::registerNotificationBackend(m_systrayBackend);
        }
    }else{
        if(m_systrayBackend != NULL){
            QtUi::unregisterNotificationBackend(m_systrayBackend);
            delete m_systrayBackend;
            m_systrayBackend = NULL;
        }
        m_snore->setPrimaryNotificationBackend(backend);
    }
}

void SnoreNotificationBackend::timeoutChanged(const QVariant &v){
    m_timeout = v.toInt();
}

void SnoreNotificationBackend::notify(const Notification &n){
    if(m_systrayBackend != NULL)
        return;
    QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
    QString message = QString("<%1> %2").arg(n.sender, n.message);
    Snore::Notification noti("Quassel",tr("Private Message"),title,message,m_icon,m_timeout);
    noti.hints().setValue("QUASSEL_ID",n.notificationId);
    m_notificationIds.insert(n.notificationId,noti.id());
    m_snore->broadcastNotification(noti);
}

void SnoreNotificationBackend::close(uint notificationId){
    if(m_systrayBackend != NULL)
        return;
    Snore::Notification n = m_snore->getActiveNotificationByID(m_notificationIds.take(notificationId));
    m_snore->requestCloseNotification(n,Snore::NotificationEnums::CloseReasons::CLOSED);
}

void SnoreNotificationBackend::actionInvoked(Snore::Notification n){
    emit activated(n.hints().value("QUASSEL_ID").toUInt());
}

SettingsPage *SnoreNotificationBackend::createConfigWidget()const{
    return new ConfigWidget(m_snore);
}


/***************************************************************************/

SnoreNotificationBackend::ConfigWidget::ConfigWidget(Snore::SnoreCore *snore,QWidget *parent)
    :SettingsPage("Internal", "SnoreNotification", parent),
      m_snore(snore)
{
    ui.setupUi(this);
    ui.backends->insertItems(0,m_snore->notificationBackends());

    connect(ui.backends, SIGNAL(currentIndexChanged(QString)), SLOT(backendChanged(QString)));
    connect(ui.timeout,SIGNAL(valueChanged(int)),this,SLOT(timeoutChanged(int)));
}

void SnoreNotificationBackend::ConfigWidget::backendChanged(const QString &b){
        ui.backends->setCurrentIndex(ui.backends->findText(b));
        setChangedState(true);
}

void SnoreNotificationBackend::ConfigWidget::timeoutChanged(int i){
    ui.timeout->setValue(i);
    setChangedState(true);

}

bool SnoreNotificationBackend::ConfigWidget::hasDefaults() const {
    return true;
}

void SnoreNotificationBackend::ConfigWidget::defaults() {
    backendChanged("SystemTray");
    timeoutChanged(10);
}

void SnoreNotificationBackend::ConfigWidget::load() {
    NotificationSettings s;
    QString backend = s.value("Snore/Backend", "SystemTray").toString();
    int timeout = s.value("Snore/Timeout",10).toInt();
    ui.backends->setCurrentIndex(ui.backends->findText(backend));
    ui.timeout->setValue(timeout);
    setChangedState(false);
}

void SnoreNotificationBackend::ConfigWidget::save() {
    NotificationSettings s;
    s.setValue("Snore/Backend", ui.backends->currentText());
    s.setValue("Snore/Timeout",ui.timeout->value());
    load();
}

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

#include <iostream>


#include <snore/core/snoreserver.h>
#include <snore/core/notification/notification.h>
#include <snore/core/interface.h>


SnoreNotificationBackend::SnoreNotificationBackend (QObject *parent)
    :AbstractNotificationBackend(parent)
{
    NotificationSettings notificationSettings;
    QString backend = notificationSettings.value("Snore/Backend", "SystemTray").toString();

    notificationSettings.notify("Snore/Backend", this, SLOT(backendChanged(const QVariant &)));

    //TODO: try to get an instance of the tray icon to be able to show popups
    m_snore = new Snore::SnoreServer();
    QDir pluginsDir(qApp->applicationDirPath()+"/snoreplugins");
    if(!pluginsDir.exists())
        pluginsDir = QDir(LIBSNORE_PLUGIN_PATH);
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.path()+"/"+fileName);
        QObject *plugin = loader.instance();
        if (plugin == NULL) {
            qDebug()<<"Failed loading plugin: "<<pluginsDir.path()+"/"+fileName<<loader.errorString();
            continue;
        }
        Snore::Notification_Backend *sp = dynamic_cast< Snore::Notification_Backend*>(plugin);
        if(sp==NULL){
            qDebug()<<"Error:"<<fileName<<" is not a Snarl backend" ;
            plugin->deleteLater();
            continue;
        }
        qDebug()<<"Loading Notification Backend"<<sp->name();
        m_snore->publicatePlugin(sp);
    }

    Snore::Application *a= new Snore::Application("Quassel",Snore::SnoreIcon(DesktopIcon("quassel").toImage()));


    connect(m_snore,SIGNAL(actionInvoked(Snore::Notification)),this,SLOT(actionInvoked(Snore::Notification)));

    a->addAlert(new Snore::Alert(tr("Private Message"),tr("Private Message")));

    m_snore->addApplication(a);
    m_snore->applicationIsInitialized (a);

    m_snore->setPrimaryNotificationBackend(backend);

    m_action = new Snore::Notification::Action(1,"View");

}

SnoreNotificationBackend::~SnoreNotificationBackend(){
    m_snore->removeApplication("Quassel");
    m_snore->deleteLater();
}

void SnoreNotificationBackend::backendChanged(const QVariant &v){
    m_snore->setPrimaryNotificationBackend(v.toString());
}

void SnoreNotificationBackend::notify(const Notification &n){
    QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
    QString message = QString("<%1> %2").arg(n.sender, n.message);
    Snore::Notification noti("Quassel",tr("Private Message"),title,message,Snore::SnoreIcon(DesktopIcon("dialog-information").toImage()));
    noti.addAction(m_action);
    m_snore->broadcastNotification(noti);
    m_notifications.insert(noti.id(),noti);
    m_notificationIds.insert(n.notificationId,noti.id());
}

void SnoreNotificationBackend::close(uint notificationId){
    Snore::Notification n = m_notifications.take(m_notificationIds.take(notificationId));
    m_snore->closeNotification(n,Snore::NotificationEnums::CloseReasons::CLOSED);
}

void SnoreNotificationBackend::actionInvoked(Snore::Notification n){
    foreach(uint i,m_notificationIds.values()){
        if(i==n.id()){
            emit activated(m_notificationIds.key(i));
            break;
        }
    }
}

SettingsPage *SnoreNotificationBackend::createConfigWidget()const{
    return new ConfigWidget(m_snore);
}


/***************************************************************************/

SnoreNotificationBackend::ConfigWidget::ConfigWidget(Snore::SnoreServer  *snore,QWidget *parent)
    :SettingsPage("Internal", "SnoreNotification", parent),
      m_snore(snore)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget( m_backends = new QComboBox(this));
    m_backends->insertItems(0,m_snore->primaryNotificationBackends());

    connect(m_backends, SIGNAL(currentIndexChanged(QString)), SLOT(backendChanged(QString)));
}

void SnoreNotificationBackend::ConfigWidget::backendChanged(QString b){
    if(b!=m_snore->primaryNotificationBackend())
        setChangedState(true);
}

bool SnoreNotificationBackend::ConfigWidget::hasDefaults() const {
    return true;
}

void SnoreNotificationBackend::ConfigWidget::defaults() {
    m_backends->setCurrentIndex(0);
    backendChanged(m_backends->currentText());
}

void SnoreNotificationBackend::ConfigWidget::load() {
    NotificationSettings s;
    m_backend= s.value("Snore/Backend", "SystemTray").toString();
    backendChanged(m_backend);
}

void SnoreNotificationBackend::ConfigWidget::save() {
    NotificationSettings s;
    s.setValue("Snore/Backend", m_backends->currentText());
    load();
}

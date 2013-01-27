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

#include <KNotification>
#include <KNotifyConfigWidget>
#include <QTextDocument>
#include <QVBoxLayout>

#include "knotificationbackend.h"

#include "client.h"
#include "icon.h"
#include "iconloader.h"
#include "mainwin.h"
#include "networkmodel.h"
#include "qtui.h"

KNotificationBackend::KNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent)
{
    connect(QtUi::mainWindow()->systemTray(), SIGNAL(activated(SystemTray::ActivationReason)),
        SLOT(notificationActivated(SystemTray::ActivationReason)));

    updateToolTip();
}


void KNotificationBackend::notify(const Notification &n)
{
    QString type;
    switch (n.type) {
    case Highlight:
        type = "Highlight"; break;
    case HighlightFocused:
        type = "HighlightFocused"; break;
    case PrivMsg:
        type = "PrivMsg"; break;
    case PrivMsgFocused:
        type = "PrivMsgFocused"; break;
    }

    QString message = QString("<b>&lt;%1&gt;</b> %2").arg(n.sender, Qt::escape(n.message));
    KNotification *notification = KNotification::event(type, message, DesktopIcon("dialog-information"), QtUi::mainWindow(),
        KNotification::RaiseWidgetOnActivation
        |KNotification::CloseWhenWidgetActivated
        |KNotification::CloseOnTimeout);
    connect(notification, SIGNAL(activated(uint)), SLOT(notificationActivated()));
    notification->setActions(QStringList("View"));
    notification->setProperty("notificationId", n.notificationId);

    _notifications.append(qMakePair(n.notificationId, QPointer<KNotification>(notification)));

    updateToolTip();
    QtUi::mainWindow()->systemTray()->setAlert(true);
}


void KNotificationBackend::removeNotificationById(uint notificationId)
{
    QList<QPair<uint, QPointer<KNotification> > >::iterator i = _notifications.begin();
    while (i != _notifications.end()) {
        if (i->first == notificationId) {
            if (i->second)
                i->second->close();
            i = _notifications.erase(i);
        }
        else
            ++i;
    }
    updateToolTip();
}


void KNotificationBackend::close(uint notificationId)
{
    removeNotificationById(notificationId);
    //if(!_notifications.count()) // FIXME make configurable
    QtUi::mainWindow()->systemTray()->setAlert(false);
}


void KNotificationBackend::notificationActivated()
{
    uint id = 0;
    KNotification *n = qobject_cast<KNotification *>(sender());
    if (n)
        id = n->property("notificationId").toUInt();

    notificationActivated(id);
}


void KNotificationBackend::notificationActivated(SystemTray::ActivationReason reason)
{
    if (reason == SystemTray::Trigger) {
        if (_notifications.count())
            notificationActivated(_notifications.first().first);  // oldest one
        else
            GraphicalUi::toggleMainWidget();
    }
}


void KNotificationBackend::notificationActivated(uint notificationId)
{
    emit activated(notificationId);
}


void KNotificationBackend::updateToolTip()
{
    QtUi::mainWindow()->systemTray()->setToolTip("Quassel IRC",
        _notifications.count() ? tr("%n pending highlight(s)", "", _notifications.count()) : QString());
}


SettingsPage *KNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


/***************************************************************************/

KNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "KNotification", parent)
{
    _widget = new KNotifyConfigWidget(this);
    _widget->setApplication("quassel");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(_widget);

    connect(_widget, SIGNAL(changed(bool)), SLOT(widgetChanged(bool)));
}


void KNotificationBackend::ConfigWidget::widgetChanged(bool changed)
{
    setChangedState(changed);
}


void KNotificationBackend::ConfigWidget::load()
{
    setChangedState(false);
}


void KNotificationBackend::ConfigWidget::save()
{
    _widget->save();
    load();
}

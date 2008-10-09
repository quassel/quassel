/***************************************************************************
*   Copyright (C) 2005-08 by the Quassel Project                          *
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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "desktopnotificationbackend.h"

#include "client.h"
#include "clientsettings.h"
#include "networkmodel.h"

DesktopNotificationBackend::DesktopNotificationBackend(QObject *parent) : AbstractNotificationBackend(parent) {
  _dbusInterface = new org::freedesktop::Notifications(
    "org.freedesktop.Notifications",
    "/org/freedesktop/Notifications",
    QDBusConnection::sessionBus(), this);
  _dbusNotificationId = 0;
  connect(_dbusInterface, SIGNAL(NotificationClosed(uint, uint)), SLOT(desktopNotificationClosed(uint, uint)));
  connect(_dbusInterface, SIGNAL(ActionInvoked(uint, const QString &)), SLOT(desktopNotificationInvoked(uint, const QString&)));

  NotificationSettings notificationSettings;
  _enabled = notificationSettings.value("DesktopNotification/Enabled", false).toBool();
  _xHint = notificationSettings.value("DesktopNotification/XHint", 0).toInt();
  _yHint = notificationSettings.value("DesktopNotification/YHint", 0).toInt();
  notificationSettings.notify("DesktopNotification/Enabled", this, SLOT(enabledChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/XHint", this, SLOT(xHintChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/YHint", this, SLOT(yHintChanged(const QVariant &)));
}

DesktopNotificationBackend::~DesktopNotificationBackend() {

}

void DesktopNotificationBackend::enabledChanged(const QVariant &v) {
  _enabled = v.toBool();
}

void DesktopNotificationBackend::xHintChanged(const QVariant &v) {
  _xHint = v.toInt();
}

void DesktopNotificationBackend::yHintChanged(const QVariant &v) {
  _yHint = v.toInt();
}

void DesktopNotificationBackend::notify(const Notification &n) {
  if(_enabled) {
    QStringList actions;
    QMap<QString, QVariant> hints;

    hints["x"] = _xHint; // Standard hint: x location for the popup to show up
    hints["y"] = _yHint; // Standard hint: y location for the popup to show up

    // actions << "click" << "Click Me!";

    QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
    QString message = QString("<%1> %2").arg(n.sender, n.message);

    QDBusReply<uint> reply = _dbusInterface->Notify(
      "Quassel IRC", // Application name
      _dbusNotificationId, // ID of previous notification to replace
      "quassel", // Icon to display
      title, // Summary / Header of the message to display
      message, // Body of the message to display
      actions, // Actions from which the user may choose
      hints, // Hints to the server displaying the message
      5000 // Timeout in milliseconds
    );

    if(!reply.isValid()) {
      /* ERROR */
      // could also happen if no notification service runs, so... whatever :)
      //qDebug() << "Error on sending notification..." << reply.error();
      return;
    }
    _dbusNotificationId = reply.value();
  }
}

void DesktopNotificationBackend::close(uint notificationId) {
  Q_UNUSED(notificationId);
}

void DesktopNotificationBackend::desktopNotificationClosed(uint id, uint reason) {
  Q_UNUSED(id); Q_UNUSED(reason);
  // qDebug() << "OID: " << notificationId << " ID: " << id << " Reason: " << reason << " Time: " << QTime::currentTime().toString();
  _dbusNotificationId = 0;
}


void DesktopNotificationBackend::desktopNotificationInvoked(uint id, const QString & action) {
  Q_UNUSED(id); Q_UNUSED(action);
  // qDebug() << "OID: " << notificationId << " ID: " << id << " Action: " << action << " Time: " << QTime::currentTime().toString();
}



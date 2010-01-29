/***************************************************************************
*   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include <QTextDocument>

#include "client.h"
#include "clientsettings.h"
#include "networkmodel.h"

DesktopNotificationBackend::DesktopNotificationBackend(QObject *parent)
  : AbstractNotificationBackend(parent),
  _lastDbusId(0)
{
  _dbusInterface = new org::freedesktop::Notifications(
    "org.freedesktop.Notifications",
    "/org/freedesktop/Notifications",
    QDBusConnection::sessionBus(), this);

  if(!_dbusInterface->isValid()) {
    qWarning() << "DBus notification service not available!";
    return;
  }

  QStringList desktopCapabilities = _dbusInterface->GetCapabilities();
  _daemonSupportsMarkup = desktopCapabilities.contains("body-markup");

  connect(_dbusInterface, SIGNAL(NotificationClosed(uint, uint)), SLOT(desktopNotificationClosed(uint, uint)));
  connect(_dbusInterface, SIGNAL(ActionInvoked(uint, const QString &)), SLOT(desktopNotificationInvoked(uint, const QString&)));

  NotificationSettings notificationSettings;
  _enabled = notificationSettings.value("DesktopNotification/Enabled", false).toBool();
  _useHints = notificationSettings.value("DesktopNotification/UseHints", false).toBool();
  _xHint = notificationSettings.value("DesktopNotification/XHint", 0).toInt();
  _yHint = notificationSettings.value("DesktopNotification/YHint", 0).toInt();
  _queueNotifications = notificationSettings.value("DesktopNotification/QueueNotifications", true).toBool();
  _timeout = notificationSettings.value("DesktopNotification/Timeout", 10000).toInt();
  _useTimeout = notificationSettings.value("DesktopNotification/UseTimeout", true).toBool();

  notificationSettings.notify("DesktopNotification/Enabled", this, SLOT(enabledChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/UseHints", this, SLOT(useHintsChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/XHint", this, SLOT(xHintChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/YHint", this, SLOT(yHintChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/Timeout", this, SLOT(timeoutChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/UseTimeout", this, SLOT(useTimeoutChanged(const QVariant &)));
  notificationSettings.notify("DesktopNotification/QueueNotifications", this, SLOT(queueNotificationsChanged(const QVariant &)));
}

void DesktopNotificationBackend::enabledChanged(const QVariant &v) {
  _enabled = v.toBool();
}

void DesktopNotificationBackend::useHintsChanged(const QVariant &v) {
  _useHints = v.toBool();
}

void DesktopNotificationBackend::xHintChanged(const QVariant &v) {
  _xHint = v.toInt();
}

void DesktopNotificationBackend::yHintChanged(const QVariant &v) {
  _yHint = v.toInt();
}

void DesktopNotificationBackend::queueNotificationsChanged(const QVariant &v) {
  _queueNotifications = v.toBool();
}

void DesktopNotificationBackend::timeoutChanged(const QVariant &v) {
  _timeout = v.toInt();
}

void DesktopNotificationBackend::useTimeoutChanged(const QVariant &v) {
  _useTimeout = v.toBool();
}

void DesktopNotificationBackend::notify(const Notification &n) {
  if(_enabled && _dbusInterface->isValid() && (n.type == Highlight || n.type == PrivMsg)) {
    QStringList actions;
    QMap<QString, QVariant> hints;

    if(_useHints) {
      hints["x"] = _xHint; // Standard hint: x location for the popup to show up
      hints["y"] = _yHint; // Standard hint: y location for the popup to show up
    }

    uint oldId = _queueNotifications ? 0 : _lastDbusId;

    actions << "activate" << "View";

    QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
    QString message = QString("<%1> %2").arg(n.sender, n.message);

    if(_daemonSupportsMarkup)
      message = Qt::escape(message);

    QDBusReply<uint> reply = _dbusInterface->Notify(
      "Quassel IRC", // Application name
      oldId, // ID of previous notification to replace
      "quassel", // Icon to display
      title, // Summary / Header of the message to display
      message, // Body of the message to display
      actions, // Actions from which the user may choose
      hints, // Hints to the server displaying the message
      _useTimeout? _timeout : 0 // Timeout in milliseconds
    );

    if(!reply.isValid()) {
      /* ERROR */
      // could also happen if no notification service runs, so... whatever :)
      // qDebug() << "Error on sending notification..." << reply.error();
      return;
    }
    uint dbusid = reply.value();
    _idMap.insert(n.notificationId, dbusid);
    _lastDbusId = dbusid;
  }
}

void DesktopNotificationBackend::close(uint notificationId) {
  uint dbusId = _idMap.value(notificationId, 0);
  if(dbusId) {
    _idMap.remove(notificationId);
    _dbusInterface->CloseNotification(dbusId);
  }
  _lastDbusId = 0;
}

void DesktopNotificationBackend::desktopNotificationClosed(uint id, uint reason) {
  Q_UNUSED(reason);
  _idMap.remove(_idMap.key(id));
  _lastDbusId = 0;
}


void DesktopNotificationBackend::desktopNotificationInvoked(uint id, const QString & action) {
  Q_UNUSED(action);
  foreach(uint ourid, _idMap.keys()) {
    if(_idMap.value(ourid) != id)
      continue;
    emit activated(ourid);
    return;
  }

  emit activated();
}

SettingsPage *DesktopNotificationBackend::createConfigWidget() const {
  return new ConfigWidget();
}

/***************************************************************************/

DesktopNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "DesktopNotification", parent) {
  ui.setupUi(this);

  connect(ui.enabled, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
  connect(ui.useHints, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
  connect(ui.xHint, SIGNAL(valueChanged(int)), SLOT(widgetChanged()));
  connect(ui.yHint, SIGNAL(valueChanged(int)), SLOT(widgetChanged()));
  connect(ui.queueNotifications, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
  connect(ui.useTimeout, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
  connect(ui.timeout, SIGNAL(valueChanged(int)), SLOT(widgetChanged()));
}

void DesktopNotificationBackend::ConfigWidget::widgetChanged() {
  bool changed =
       enabled != ui.enabled->isChecked()
    || useHints != ui.useHints->isChecked()
    || xHint != ui.xHint->value()
    || yHint != ui.yHint->value()
    || queueNotifications != ui.queueNotifications->isChecked()
    || timeout/1000 != ui.timeout->value()
    || useTimeout != ui.useTimeout->isChecked();
  if(changed != hasChanged()) setChangedState(changed);
}

bool DesktopNotificationBackend::ConfigWidget::hasDefaults() const {
  return true;
}

void DesktopNotificationBackend::ConfigWidget::defaults() {
  ui.enabled->setChecked(false);
  ui.useTimeout->setChecked(true);
  ui.timeout->setValue(10);
  ui.useHints->setChecked(false);
  ui.xHint->setValue(0);
  ui.yHint->setValue(0);
  ui.queueNotifications->setChecked(true);
  widgetChanged();
}

void DesktopNotificationBackend::ConfigWidget::load() {
  NotificationSettings s;
  enabled = s.value("DesktopNotification/Enabled", false).toBool();
  useTimeout = s.value("DesktopNotification/UseTimeout", true).toBool();
  timeout = s.value("DesktopNotification/Timeout", 10000).toInt();
  useHints = s.value("DesktopNotification/UseHints", false).toBool();
  xHint = s.value("DesktopNotification/XHint", 0).toInt();
  yHint = s.value("DesktopNotification/YHint", 0).toInt();
  queueNotifications = s.value("DesktopNotification/QueueNotifications", true).toBool();

  ui.enabled->setChecked(enabled);
  ui.useTimeout->setChecked(useTimeout);
  ui.timeout->setValue(timeout/1000);
  ui.useHints->setChecked(useHints);
  ui.xHint->setValue(xHint);
  ui.yHint->setValue(yHint);
  ui.queueNotifications->setChecked(queueNotifications);

  setChangedState(false);
}

void DesktopNotificationBackend::ConfigWidget::save() {
  NotificationSettings s;
  s.setValue("DesktopNotification/Enabled", ui.enabled->isChecked());
  s.setValue("DesktopNotification/UseTimeout", ui.useTimeout->isChecked());
  s.setValue("DesktopNotification/Timeout", ui.timeout->value() * 1000);
  s.setValue("DesktopNotification/UseHints", ui.useHints->isChecked());
  s.setValue("DesktopNotification/XHint", ui.xHint->value());
  s.setValue("DesktopNotification/YHint", ui.yHint->value());
  s.setValue("DesktopNotification/QueueNotifications", ui.queueNotifications->isChecked());

  load();
}

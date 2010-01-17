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

#ifndef QT_NO_SYSTEMTRAYICON

#include "systraynotificationbackend.h"

#include <QtGui>

#include "client.h"
#include "clientsettings.h"
#include "icon.h"
#include "mainwin.h"
#include "networkmodel.h"
#include "qtui.h"
#include "systemtray.h"

SystrayNotificationBackend::SystrayNotificationBackend(QObject *parent)
  : AbstractNotificationBackend(parent)
{
  NotificationSettings notificationSettings;
  _showBubble = notificationSettings.value("Systray/ShowBubble", true).toBool();
  _animate = notificationSettings.value("Systray/Animate", true).toBool();

  notificationSettings.notify("Systray/ShowBubble", this, SLOT(showBubbleChanged(const QVariant &)));
  notificationSettings.notify("Systray/Animate", this, SLOT(animateChanged(const QVariant &)));

  connect(QtUi::mainWindow()->systemTray(), SIGNAL(messageClicked()), SLOT(notificationActivated()));
  connect(QtUi::mainWindow()->systemTray(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                                            SLOT(notificationActivated(QSystemTrayIcon::ActivationReason)));
}

void SystrayNotificationBackend::notify(const Notification &notification) {
  if(notification.type != Highlight && notification.type != PrivMsg)
    return;

  _notifications.append(notification);
  if(_showBubble)
    showBubble();

  if(_animate)
    QtUi::mainWindow()->systemTray()->setAlert(true);
}

void SystrayNotificationBackend::close(uint notificationId) {
  QList<Notification>::iterator i = _notifications.begin();
  while(i != _notifications.end()) {
    if(i->notificationId == notificationId)
      i = _notifications.erase(i);
    else
      ++i;
  }

  closeBubble();

  if(!_notifications.count())
    QtUi::mainWindow()->systemTray()->setAlert(false);
}

void SystrayNotificationBackend::showBubble() {
  // fancy stuff later: show messages in order
  // for now, we just show the last message
  if(_notifications.isEmpty())
    return;
  Notification n = _notifications.last();
  QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
  QString message = QString("<%1> %2").arg(n.sender, n.message);
  QtUi::mainWindow()->systemTray()->showMessage(title, message);
}

void SystrayNotificationBackend::closeBubble() {
  // there really seems to be no sane way to close the bubble... :(
#ifdef Q_WS_X11
  QtUi::mainWindow()->systemTray()->showMessage("", "", QSystemTrayIcon::NoIcon, 1);
#endif
}

void SystrayNotificationBackend::notificationActivated() {
  if(QtUi::mainWindow()->systemTray()->isAlerted()) {
    QtUi::mainWindow()->systemTray()->setInhibitActivation();
    uint id = _notifications.count()? _notifications.last().notificationId : 0;
    emit activated(id);
  }
}

void SystrayNotificationBackend::notificationActivated(QSystemTrayIcon::ActivationReason reason) {
  if(reason == QSystemTrayIcon::Trigger) {
    notificationActivated();
  }
}

void SystrayNotificationBackend::showBubbleChanged(const QVariant &v) {
  _showBubble = v.toBool();
}

void SystrayNotificationBackend::animateChanged(const QVariant &v) {
  _animate = v.toBool();
}

SettingsPage *SystrayNotificationBackend::createConfigWidget() const {
  return new ConfigWidget();
}

/***************************************************************************/

SystrayNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "SystrayNotification", parent) {
  QGroupBox *groupBox = new QGroupBox(tr("System Tray Icon"), this);
  _animateBox = new QCheckBox(tr("Animate"));
  connect(_animateBox, SIGNAL(toggled(bool)), this, SLOT(widgetChanged()));
  _showBubbleBox = new QCheckBox(tr("Show bubble"));
  connect(_showBubbleBox, SIGNAL(toggled(bool)), this, SLOT(widgetChanged()));
  QVBoxLayout *layout = new QVBoxLayout(groupBox);
  layout->addWidget(_animateBox);
  layout->addWidget(_showBubbleBox);
  layout->addStretch(1);
  QVBoxLayout *globalLayout = new QVBoxLayout(this);
  globalLayout->addWidget(groupBox);

}

void SystrayNotificationBackend::ConfigWidget::widgetChanged() {
  bool changed = (_showBubble != _showBubbleBox->isChecked() || _animate != _animateBox->isChecked());
  if(changed != hasChanged()) setChangedState(changed);
}

bool SystrayNotificationBackend::ConfigWidget::hasDefaults() const {
  return true;
}

void SystrayNotificationBackend::ConfigWidget::defaults() {
  _animateBox->setChecked(true);
  _showBubbleBox->setChecked(false);
  widgetChanged();
}

void SystrayNotificationBackend::ConfigWidget::load() {
  NotificationSettings s;
  _animate = s.value("Systray/Animate", true).toBool();
  _showBubble = s.value("Systray/ShowBubble", false).toBool();
  _animateBox->setChecked(_animate);
  _showBubbleBox->setChecked(_showBubble);
  setChangedState(false);
}

void SystrayNotificationBackend::ConfigWidget::save() {
  NotificationSettings s;
  s.setValue("Systray/Animate", _animateBox->isChecked());
  s.setValue("Systray/ShowBubble", _showBubbleBox->isChecked());
  load();
}

#endif /* QT_NO_SYSTEMTRAYICON */

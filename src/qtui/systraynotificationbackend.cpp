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

#include "systraynotificationbackend.h"

#include <QtGui>

#include "client.h"
#include "clientsettings.h"
#include "icon.h"
#include "mainwin.h"
#include "networkmodel.h"
#include "qtui.h"

SystrayNotificationBackend::SystrayNotificationBackend(QObject *parent) : AbstractNotificationBackend(parent) {
  NotificationSettings notificationSettings;
  _showBubble = notificationSettings.value("Systray/ShowBubble", true).toBool();
  _animate = notificationSettings.value("Systray/Animate", true).toBool();

  notificationSettings.notify("Systray/ShowBubble", this, SLOT(showBubbleChanged(const QVariant &)));
  notificationSettings.notify("Systray/Animate", this, SLOT(animateChanged(const QVariant &)));

  _configWidget = new ConfigWidget();
  _iconActive = false;
  connect(&_animationTimer, SIGNAL(timeout()), SLOT(blink()));
}

SystrayNotificationBackend::~SystrayNotificationBackend() {
  delete _configWidget;
}

void SystrayNotificationBackend::notify(const Notification &notification) {
  /* fancy stuff to be implemented later: show notifications in order
  _notifications.append(notification);
  if(_showBubble && _notifications.count() == 1) {
    showBubble();
  }
  */
  _notifications.clear();
  _notifications.append(notification);
  if(_showBubble) {
    showBubble();
  }
  if(_animate) {
    startAnimation();
  }
}

void SystrayNotificationBackend::close(uint notificationId) {
  Q_UNUSED(notificationId);
  /* fancy stuff to be implemented later
  int idx = _notifications.indexOf(notificationId);

  if(_notifications.isEmpty()) {
  */
  _notifications.clear();
  closeBubble();
  stopAnimation();
}

void SystrayNotificationBackend::showBubble() {
  // fancy stuff later: show messages in order
  // for now, we just show the last message
  if(_notifications.isEmpty()) return;
  Notification n = _notifications.takeLast();
  QString title = Client::networkModel()->networkName(n.bufferId) + " - " + Client::networkModel()->bufferName(n.bufferId);
  QString message = QString("<%1> %2").arg(n.sender, n.message);
  QtUi::mainWindow()->systemTrayIcon()->showMessage(title, message);
}

void SystrayNotificationBackend::closeBubble() {
  // there really seems to be no decent way to close the bubble...
  QtUi::mainWindow()->systemTrayIcon()->showMessage("", "", QSystemTrayIcon::NoIcon, 1);
}

void SystrayNotificationBackend::showBubbleChanged(const QVariant &v) {
  _showBubble = v.toBool();
}

void SystrayNotificationBackend::startAnimation() {
  if(!_animationTimer.isActive())
    _animationTimer.start(500);
}

void SystrayNotificationBackend::stopAnimation() {
  _animationTimer.stop();
  QtUi::mainWindow()->systemTrayIcon()->setIcon(Icon("quassel"));
  _iconActive = false;
}

void SystrayNotificationBackend::blink() {
  QtUi::mainWindow()->systemTrayIcon()->setIcon(_iconActive ? Icon("quassel") : Icon("quassel_newmessage"));
  _iconActive = !_iconActive;
}

void SystrayNotificationBackend::animateChanged(const QVariant &v) {
  _animate = v.toBool();
}

SettingsPage *SystrayNotificationBackend::configWidget() const {
  return _configWidget;
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

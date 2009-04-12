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

#include <QVBoxLayout>

#include <KNotification>
#include <KNotifyConfigWidget>
#include <QTextDocument>

#include "knotificationbackend.h"

#include "client.h"
#include "icon.h"
#include "iconloader.h"
#include "networkmodel.h"
#include "qtui.h"
#include "systemtray.h"

KNotificationBackend::KNotificationBackend(QObject *parent) : AbstractNotificationBackend(parent) {

}

void KNotificationBackend::notify(const Notification &n) {
  QString type;
  switch(n.type) {
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
                                KNotification::Persistent|KNotification::RaiseWidgetOnActivation|KNotification::CloseWhenWidgetActivated);
  connect(notification, SIGNAL(activated(uint)), SLOT(notificationActivated()));
  connect(notification, SIGNAL(closed()), SLOT(notificationClosed()));
  notification->setActions(QStringList("View"));
  _notificationIds[notification] = n.notificationId;

  QtUi::mainWindow()->systemTray()->setAlert(true);
}

void KNotificationBackend::close(uint notificationId) {
  Q_UNUSED(notificationId);
  QtUi::mainWindow()->systemTray()->setAlert(false);
}

void KNotificationBackend::notificationActivated() {
  uint id = 0;
  KNotification *n = qobject_cast<KNotification *>(sender());
  if(n && _notificationIds.contains(n))
    id = _notificationIds.value(n);

  emit activated(id);
}

void KNotificationBackend::notificationClosed() {
  KNotification *n = qobject_cast<KNotification *>(sender());
  if(n && _notificationIds.contains(n))
    _notificationIds.remove(n);
}

SettingsPage *KNotificationBackend::createConfigWidget() const {
  return new ConfigWidget();
}

/***************************************************************************/

KNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent) : SettingsPage("Internal", "KNotification", parent) {
  _widget = new KNotifyConfigWidget(this);
  _widget->setApplication("quassel");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(_widget);

  connect(_widget, SIGNAL(changed(bool)), SLOT(widgetChanged(bool)));
}

void KNotificationBackend::ConfigWidget::widgetChanged(bool changed) {
  setChangedState(changed);
}

void KNotificationBackend::ConfigWidget::load() {
  setChangedState(false);
}

void KNotificationBackend::ConfigWidget::save() {
  _widget->save();
  load();
}

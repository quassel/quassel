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

#include "qtui.h"

#include "abstractnotificationbackend.h"
#include "actioncollection.h"
#include "chatlinemodel.h"
#include "mainwin.h"
#include "networkmodelactionprovider.h"
#include "qtuimessageprocessor.h"
#include "qtuisettings.h"
#include "qtuistyle.h"
#include "types.h"
#include "util.h"

QHash<QString, ActionCollection *> QtUi::_actionCollections;
QPointer<QtUi> QtUi::_instance = 0;
QPointer<MainWin> QtUi::_mainWin = 0;
QList<AbstractNotificationBackend *> QtUi::_notificationBackends;
QList<AbstractNotificationBackend::Notification> QtUi::_notifications;
QtUiStyle *QtUi::_style = 0;

QtUi::QtUi() : AbstractUi() {
  if(_instance != 0) {
    qWarning() << "QtUi has been instantiated again!";
    return;
  }
  _instance = this;

  _actionProvider = new NetworkModelActionProvider(this);

  QtUiSettings uiSettings;
  Quassel::loadTranslation(uiSettings.value("Locale", QLocale::system()).value<QLocale>());

  _mainWin = new MainWin();
  _style = new QtUiStyle;

  connect(_mainWin, SIGNAL(connectToCore(const QVariantMap &)), this, SIGNAL(connectToCore(const QVariantMap &)));
  connect(_mainWin, SIGNAL(disconnectFromCore()), this, SIGNAL(disconnectFromCore()));
}

QtUi::~QtUi() {
  unregisterAllNotificationBackends();
  delete _style;
  delete _mainWin;
}

void QtUi::init() {
  _mainWin->init();
}

ActionCollection *QtUi::actionCollection(const QString &category) {
  if(_actionCollections.contains(category))
    return _actionCollections.value(category);
  ActionCollection *coll = new ActionCollection(mainWindow());
  coll->addAssociatedWidget(mainWindow());
  _actionCollections.insert(category, coll);
  return coll;
}

MessageModel *QtUi::createMessageModel(QObject *parent) {
  return new ChatLineModel(parent);
}

AbstractMessageProcessor *QtUi::createMessageProcessor(QObject *parent) {
  return new QtUiMessageProcessor(parent);
}

void QtUi::connectedToCore() {
  _mainWin->connectedToCore();
}

void QtUi::disconnectedFromCore() {
  _mainWin->disconnectedFromCore();
}

void QtUi::registerNotificationBackend(AbstractNotificationBackend *backend) {
  if(!_notificationBackends.contains(backend)) {
    _notificationBackends.append(backend);
    instance()->connect(backend, SIGNAL(activated()), SLOT(notificationActivated()));
  }
}

void QtUi::unregisterNotificationBackend(AbstractNotificationBackend *backend) {
  _notificationBackends.removeAll(backend);
}

void QtUi::unregisterAllNotificationBackends() {
  _notificationBackends.clear();
}

const QList<AbstractNotificationBackend *> &QtUi::notificationBackends() {
  return _notificationBackends;
}

uint QtUi::invokeNotification(BufferId bufId, const QString &sender, const QString &text) {
  static int notificationId = 0;
  //notificationId++;
  AbstractNotificationBackend::Notification notification(++notificationId, bufId, sender, text);
  _notifications.append(notification);
  foreach(AbstractNotificationBackend *backend, _notificationBackends)
    backend->notify(notification);
  return notificationId;
}

void QtUi::closeNotification(uint notificationId) {
  QList<AbstractNotificationBackend::Notification>::iterator i = _notifications.begin();
  while(i != _notifications.end()) {
    if((*i).notificationId == notificationId) {
      foreach(AbstractNotificationBackend *backend, _notificationBackends)
        backend->close(notificationId);
      i = _notifications.erase(i);
      break;
    } else {
      ++i;
    }
  }
}

void QtUi::closeNotifications(BufferId bufferId) {
  QList<AbstractNotificationBackend::Notification>::iterator i = _notifications.begin();
  while(i != _notifications.end()) {
    if(!bufferId.isValid() || (*i).bufferId == bufferId) {
      foreach(AbstractNotificationBackend *backend, _notificationBackends)
        backend->close((*i).notificationId);
      i = _notifications.erase(i);
    } else {
      ++i;
    }
  }
}

const QList<AbstractNotificationBackend::Notification> &QtUi::activeNotifications() {
  return _notifications;
}

void QtUi::notificationActivated() {
  // this might not work with some window managers
  _mainWin->raise();
  _mainWin->activateWindow();
}

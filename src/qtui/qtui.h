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

#ifndef QTUI_H
#define QTUI_H

#include "graphicalui.h"

#include "abstractnotificationbackend.h"
#include "mainwin.h"

class MainWin;
class MessageModel;
class QtUiMessageProcessor;
class QtUiStyle;

//! This class encapsulates Quassel's Qt-based GUI.
/** This is basically a wrapper around MainWin, which is necessary because we cannot derive MainWin
 *  from both QMainWindow and AbstractUi (because of multiple inheritance of QObject).
 */
class QtUi : public GraphicalUi {
  Q_OBJECT

public:
  QtUi();
  ~QtUi();

  MessageModel *createMessageModel(QObject *parent);
  AbstractMessageProcessor *createMessageProcessor(QObject *parent);

  inline static QtUi *instance();
  inline static QtUiStyle *style();
  inline static MainWin *mainWindow();

  /* Notifications */

  static void registerNotificationBackend(AbstractNotificationBackend *);
  static void unregisterNotificationBackend(AbstractNotificationBackend *);
  static void unregisterAllNotificationBackends();
  static const QList<AbstractNotificationBackend *> &notificationBackends();
  static uint invokeNotification(BufferId bufId, AbstractNotificationBackend::NotificationType type, const QString &sender, const QString &text);
  static void closeNotification(uint notificationId);
  static void closeNotifications(BufferId bufferId = BufferId());
  static const QList<AbstractNotificationBackend::Notification> &activeNotifications();

public slots:
  virtual void init();

protected slots:
  void connectedToCore();
  void disconnectedFromCore();
  void notificationActivated(uint notificationId);

private:
  static QPointer<QtUi> _instance;
  static QPointer<MainWin> _mainWin;
  static QtUiStyle *_style;
  static QList<AbstractNotificationBackend *> _notificationBackends;
  static QList<AbstractNotificationBackend::Notification> _notifications;
};

QtUi *QtUi::instance() { return _instance ? _instance.data() : new QtUi(); }
QtUiStyle *QtUi::style() { return _style; }
MainWin *QtUi::mainWindow() { return _mainWin; }

#endif

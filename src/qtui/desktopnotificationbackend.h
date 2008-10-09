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

#ifndef DESKTOPNOTIFICATIONBACKEND_H_
#define DESKTOPNOTIFICATIONBACKEND_H_

#include "abstractnotificationbackend.h"

#include "settingspage.h"

#include "desktopnotificationinterface.h"

//! Implements the freedesktop.org notifications specification (via D-Bus)
/**
 *  cf. http://www.galago-project.org/specs/notification/
 *
 *  This class will only be available if -DHAVE_DBUS is enabled
 */
class DesktopNotificationBackend : public AbstractNotificationBackend {
  Q_OBJECT

public:
  DesktopNotificationBackend(QObject *parent = 0);
  ~DesktopNotificationBackend();

  void notify(const Notification &);
  void close(uint notificationId);
  SettingsPage *configWidget() const;

private slots:
  void desktopNotificationClosed(uint id, uint reason);
  void desktopNotificationInvoked(uint id, const QString &action);

  void enabledChanged(const QVariant &);
  void xHintChanged(const QVariant &);
  void yHintChanged(const QVariant &);

private:
  org::freedesktop::Notifications *_dbusInterface;
  quint32 _dbusNotificationId;

  bool _enabled;
  int _xHint, _yHint;
};

#endif

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

#ifndef TASKBARNOTIFICATIONBACKEND_H_
#define TASKBARNOTIFICATIONBACKEND_H_

#include "abstractnotificationbackend.h"

#include "settingspage.h"

class QCheckBox;
class QSpinBox;

class TaskbarNotificationBackend : public AbstractNotificationBackend {
  Q_OBJECT

public:
  TaskbarNotificationBackend(QObject *parent = 0);
  ~TaskbarNotificationBackend();

  void notify(const Notification &);
  void close(uint notificationId);
  SettingsPage *configWidget() const;

private slots:
  void enabledChanged(const QVariant &);
  void timeoutChanged(const QVariant &);

private:
  class ConfigWidget;

  SettingsPage *_configWidget;
  bool _enabled;
  int _timeout;
};

class TaskbarNotificationBackend::ConfigWidget : public SettingsPage {
  Q_OBJECT

public:
  ConfigWidget(QWidget *parent = 0);
  void save();
  void load();
  bool hasDefaults() const;
  void defaults();

private slots:
  void widgetChanged();

private:
  QCheckBox *enabledBox;
  QSpinBox *timeoutBox;

  bool enabled;
  int timeout;
};

#endif

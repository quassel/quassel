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

#ifndef SYSTRAYNOTIFICATIONBACKEND_H_
#define SYSTRAYNOTIFICATIONBACKEND_H_

#include "abstractnotificationbackend.h"

#include "settingspage.h"

class QCheckBox;

class SystrayNotificationBackend : public AbstractNotificationBackend {
  Q_OBJECT

public:
  SystrayNotificationBackend(QObject *parent = 0);

  void notify(const Notification &);
  void close(uint notificationId);
  virtual SettingsPage *createConfigWidget() const;

private slots:
  void showBubble();
  void closeBubble();

  void animateChanged(const QVariant &);
  void showBubbleChanged(const QVariant &);

private:
  class ConfigWidget;

  bool _showBubble;
  bool _animate;
  QList<Notification> _notifications;
};

class SystrayNotificationBackend::ConfigWidget : public SettingsPage {
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
  QCheckBox *_showBubbleBox, *_animateBox;
  bool _showBubble, _animate;
};

#endif

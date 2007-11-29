/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _SETTINGSPAGES_H_
#define _SETTINGSPAGES_H_

#include <QtCore>
#include <QtGui>

#include "settingspage.h"
#include "ui_buffermgmntsettingspage.h"
#include "ui_connectionsettingspage.h"
#include "ui_usermgmtsettingspage.h"

class BufferManagementSettingsPage : public QWidget, public SettingsInterface {
  Q_OBJECT
  Q_INTERFACES(SettingsInterface);

  public:
    QString category() { return tr("Buffers"); }
    QString title() { return tr("Buffer Management"); }
    QWidget *settingsWidget() { return this; }

    BufferManagementSettingsPage();

    void applyChanges();


  private:
    Ui::BufferManagementSettingsPage ui;

};

class ConnectionSettingsPage : public QWidget, public SettingsInterface {
  Q_OBJECT
  Q_INTERFACES(SettingsInterface);

  public:
    QString category() { return tr("Behavior"); }
    QString title() { return tr("Connection"); }
    QWidget *settingsWidget() { return this; }

    ConnectionSettingsPage();

    void applyChanges();

  private:
    Ui::ConnectionSettingsPage ui;

};

class AccountManagementSettingsPage : public QWidget, public SettingsInterface {
  Q_OBJECT
  Q_INTERFACES(SettingsInterface);

  public:
    QString category() { return tr("Administration"); }
    QString title() { return tr("Account Management"); }
    QWidget *settingsWidget() { return this; }

    AccountManagementSettingsPage();

    void applyChanges();

  private:
    Ui::AccountManagementSettingsPage ui;

};




#endif

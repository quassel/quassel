/***************************************************************************
 *   Copyright (C) 2009 by the Quassel Project                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef COREACCOUNTSETTINGSPAGE_H
#define COREACCOUNTSETTINGSPAGE_H

#include "clientsettings.h"
#include "settingspage.h"

#include "ui_coreconnectionsettingspage.h"

class CoreConnectionSettingsPage : public SettingsPage {
  Q_OBJECT

public:
  CoreConnectionSettingsPage(QWidget *parent = 0);

  inline bool hasDefaults() const { return true; }

public slots:
  void save();
  void load();
  void defaults();

signals:

private slots:
  void widgetHasChanged();

private:
  Ui::CoreConnectionSettingsPage ui;
  CoreConnectionSettings::NetworkDetectionMode _detectionMode;

  void setRadioButtons(CoreConnectionSettings::NetworkDetectionMode mode);
  CoreConnectionSettings::NetworkDetectionMode modeFromRadioButtons() const;

  inline QString settingsKey() const { return QString("CoreConnection"); }
};

#endif

/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef APPEARANCESETTINGSPAGE_H
#define APPEARANCESETTINGSPAGE_H

#include <QHash>
#include <QList>
#include <QLocale>

#include "settingspage.h"
#include "ui_appearancesettingspage.h"

class AppearanceSettingsPage : public SettingsPage {
  Q_OBJECT

public:
  AppearanceSettingsPage(QWidget *parent = 0);

  inline bool hasDefaults() const { return true; }

public slots:
  void save();
  void load();
  void defaults();
		 
private slots:
  void widgetHasChanged();
  
private:
  Ui::AppearanceSettingsPage ui;
  QHash<QString, QVariant> settings;
  QList<QLocale> _locales;

  bool testHasChanged();
  void initStyleComboBox();
  void initLanguageComboBox();
  QLocale selectedLocale() const;
};

#endif

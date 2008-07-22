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

#ifndef ALIASESSETTINGSPAGE_H
#define ALIASESSETTINGSPAGE_H

#include "settingspage.h"
#include "ui_aliasessettingspage.h"

#include "aliasesmodel.h"

class AliasesSettingsPage : public SettingsPage {
  Q_OBJECT

public:
  AliasesSettingsPage(QWidget *parent = 0);

public slots:
  void save();
  void load();
//   void defaults();
		 
// private slots:
//   void widgetHasChanged();
  
private:
  Ui::AliasesSettingsPage ui;

  AliasesModel _aliasesModel;

  //   bool testHasChanged();

private slots:
  void enableDialog();
  void deleteSelectedAlias();
};

#endif //ALIASESSETTINGSPAGE_H

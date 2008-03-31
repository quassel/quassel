/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Blank Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Blank Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Blank Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _BLANKSETTINGSPAGE_H_
#define _BLANKSETTINGSPAGE_H_

#include <QHash>

#include "settingspage.h"
#include "ui_blanksettingspage.h"

class BlankSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    BlankSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

  public slots:
    void save();
    void load();
    void defaults();

  private slots:
    void widgetHasChanged();

  private:
    Ui::BlankSettingsPage ui;
    QHash<QString, QVariant> settings;

    bool testHasChanged();
};

#endif

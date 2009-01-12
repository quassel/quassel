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

#ifndef SETTINGSPAGEDLG_H
#define SETTINGSPAGEDLG_H

#include <QtGui>
#include "ui_settingspagedlg.h"

#include "settingspage.h"

class SettingsPageDlg : public QDialog {
  Q_OBJECT
  public:
    SettingsPageDlg(SettingsPage * page, QWidget *parent = 0);

    SettingsPage *currentPage() const;

  private slots:
    void buttonClicked(QAbstractButton *);
    bool applyChanges();
    void undoChanges();
    void reload();
    void loadDefaults();
    void setButtonStates();

  private:
    Ui::SettingsPageDlg ui;

    SettingsPage *_currentPage;

};

#endif

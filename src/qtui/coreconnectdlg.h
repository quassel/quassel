/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef CORECONNECTDLG_H
#define CORECONNECTDLG_H_

#include <QDialog>

#include "coreaccount.h"

#include "ui_coreconnectauthdlg.h"

class CoreAccountSettingsPage;

class CoreConnectDlg : public QDialog
{
    Q_OBJECT

public:
    CoreConnectDlg(QWidget *parent = 0);
    AccountId selectedAccount() const;

    void accept();

private:
    CoreAccountSettingsPage *_settingsPage;
};


class CoreConnectAuthDlg : public QDialog
{
    Q_OBJECT

public:
    CoreConnectAuthDlg(CoreAccount *account, QWidget *parent = 0);

    void accept();

private slots:
    void setButtonStates();

private:
    Ui::CoreConnectAuthDlg ui;
    CoreAccount *_account;
};


#endif

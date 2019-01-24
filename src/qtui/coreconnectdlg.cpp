/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "coreconnectdlg.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "clientsettings.h"
#include "coreaccountsettingspage.h"
#include "icon.h"

CoreConnectDlg::CoreConnectDlg(QWidget* parent)
    : QDialog(parent)
{
    _settingsPage = new CoreAccountSettingsPage(this);
    _settingsPage->setStandAlone(true);
    _settingsPage->load();

    CoreAccountSettings s;
    AccountId lastAccount = s.lastAccount();
    if (lastAccount.isValid())
        _settingsPage->setSelectedAccount(lastAccount);

    setWindowTitle(tr("Connect to Core"));
    setWindowIcon(icon::get("network-disconnect"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(_settingsPage);

    auto* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(_settingsPage, &CoreAccountSettingsPage::connectToCore, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AccountId CoreConnectDlg::selectedAccount() const
{
    return _settingsPage->selectedAccount();
}

void CoreConnectDlg::accept()
{
    _settingsPage->save();
    QDialog::accept();
}

/******** CoreConnectAuthDlg ****************************************************************/

CoreConnectAuthDlg::CoreConnectAuthDlg(CoreAccount* account, QWidget* parent)
    : QDialog(parent)
    , _account(account)
{
    ui.setupUi(this);

    connect(ui.user, &QLineEdit::textChanged, this, &CoreConnectAuthDlg::setButtonStates);
    connect(ui.password, &QLineEdit::textChanged, this, &CoreConnectAuthDlg::setButtonStates);

    ui.label->setText(tr("Please enter your credentials for %1:").arg(account->accountName()));
    ui.user->setText(account->user());
    ui.password->setText(account->password());
    ui.rememberPasswd->setChecked(account->storePassword());

    if (ui.user->text().isEmpty())
        ui.user->setFocus();
    else
        ui.password->setFocus();
}

void CoreConnectAuthDlg::accept()
{
    _account->setUser(ui.user->text());
    _account->setPassword(ui.password->text());
    _account->setStorePassword(ui.rememberPasswd->isChecked());

    QDialog::accept();
}

void CoreConnectAuthDlg::setButtonStates()
{
    bool valid = !(ui.user->text().isEmpty() || ui.password->text().isEmpty());
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

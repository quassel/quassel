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

#include "passwordchangedlg.h"

#include <QMessageBox>
#include <QPushButton>

#include "client.h"

PasswordChangeDlg::PasswordChangeDlg(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    CoreAccount account = Client::currentCoreAccount();
    ui.infoLabel->setText(tr("This changes the password for your username <b>%1</b> "
                             "on the Quassel Core running at <b>%2</b>.")
                          .arg(account.user(), account.hostName()));

    connect(ui.oldPasswordEdit, SIGNAL(textChanged(QString)), SLOT(inputChanged()));
    connect(ui.newPasswordEdit, SIGNAL(textChanged(QString)), SLOT(inputChanged()));
    connect(ui.confirmPasswordEdit, SIGNAL(textChanged(QString)), SLOT(inputChanged()));
    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(changePassword()));

    connect(Client::instance(), SIGNAL(passwordChanged(bool)), SLOT(passwordChanged(bool)));

    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}


void PasswordChangeDlg::inputChanged()
{
    bool ok = !ui.oldPasswordEdit->text().isEmpty() && !ui.newPasswordEdit->text().isEmpty()
              && ui.newPasswordEdit->text() == ui.confirmPasswordEdit->text();
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}


void PasswordChangeDlg::changePassword()
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    Client::changePassword(ui.oldPasswordEdit->text(), ui.newPasswordEdit->text());
}


void PasswordChangeDlg::passwordChanged(bool success)
{
    if (!success) {
        QMessageBox box(QMessageBox::Warning, tr("Password Not Changed"),
                        tr("<b>Password change failed</b>"),
                        QMessageBox::Ok, this);
        box.setInformativeText(tr("The core reported an error when trying to change your password. Make sure you entered your old password correctly!"));
        box.exec();
    }
    else {
        accept();
    }
}

/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
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

#include <QtGui>
#include "coreconnectdlg.h"
#include "clientproxy.h"
#include "global.h"
#include "client.h"
#include "clientsettings.h"

CoreConnectDlg::CoreConnectDlg(QWidget *parent, bool /*doAutoConnect*/) : QDialog(parent) {
  ui.setupUi(this); //qDebug() << "new dlg";

  coreState = 0;
  if(Global::runMode == Global::Monolithic) {
    connect(ui.internalCore, SIGNAL(toggled(bool)), ui.hostEdit, SLOT(setDisabled(bool)));
    connect(ui.internalCore, SIGNAL(toggled(bool)), ui.port, SLOT(setDisabled(bool)));
    ui.internalCore->setChecked(true);
  } else {
    ui.internalCore->hide();
  }
  connect(ui.newAccount, SIGNAL(clicked()), this, SLOT(createAccount()));
  connect(ui.delAccount, SIGNAL(clicked()), this, SLOT(removeAccount()));
  connect(ui.buttonBox1, SIGNAL(accepted()), this, SLOT(doConnect()));
  connect(ui.hostEdit, SIGNAL(textChanged(const QString &)), this, SLOT(checkInputValid()));
  connect(ui.userEdit, SIGNAL(textChanged(const QString &)), this, SLOT(checkInputValid()));
  connect(ui.internalCore, SIGNAL(toggled(bool)), this, SLOT(checkInputValid()));
  connect(ui.accountList, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(accountChanged(const QString &)));
  connect(ui.autoConnect, SIGNAL(clicked(bool)), this, SLOT(autoConnectToggled(bool)));

  connect(Client::instance(), SIGNAL(coreConnectionMsg(const QString &)), ui.connectionStatus, SLOT(setText(const QString &)));
  connect(Client::instance(), SIGNAL(coreConnectionProgress(uint, uint)), this, SLOT(updateProgressBar(uint, uint)));
  connect(Client::instance(), SIGNAL(coreConnectionError(QString)), this, SLOT(coreConnectionError(QString)));
  connect(Client::instance(), SIGNAL(connected()), this, SLOT(coreConnected()));

  AccountSettings s;
  ui.accountList->addItems(s.knownAccounts());
  curacc = s.lastAccount();
  if(!ui.accountList->count()) {
    //if(doAutoConnect) reject();
    /*
    setAccountEditEnabled(false);
    QString newacc = QInputDialog::getText(this, tr("Create Account"), tr(
                                           "In order to connect to a Quassel Core, you need to create an account.<br>"
                                           "Please enter a name for this account now:"), QLineEdit::Normal, tr("Default"));
    if(!newacc.isEmpty()) {
      ui.accountList->addItem(newacc);
      ui.hostEdit->setText("localhost");
      ui.port->setValue(DEFAULT_PORT);
      ui.internalCore->setChecked(false);
      setAccountEditEnabled(true);
    }
    */
    // FIXME We create a default account here that just connects to the internal core
    curacc = "Default";
    ui.accountList->addItem("Default");
    ui.internalCore->setChecked(true);
    ui.userEdit->setText("Default");
    ui.passwdEdit->setText("password");
    ui.rememberPasswd->setChecked(true);
    accountChanged(curacc);
    ui.passwdEdit->setText("password");
    ui.accountList->setCurrentIndex(0);
    ui.autoConnect->setChecked(true);
    autoConnectToggled(true);

  } else {
    if(!curacc.isEmpty()) {
      //if(doAutoConnect) { qDebug() << "auto";
      //  AccountSettings s;
      //  int idx = ui.accountList->findText(s.autoConnectAccount());
      //  if(idx < 0) reject();
      //  else {
      //    ui.accountList->setCurrentIndex(idx);
      //    doConnect();
      //  }
      //} else {
        int idx = ui.accountList->findText(curacc);
        ui.accountList->setCurrentIndex(idx);
      //}
    }
  }
}

void CoreConnectDlg::setAccountEditEnabled(bool en) {
  ui.accountList->setEnabled(en);
  ui.hostEdit->setEnabled(en && !ui.internalCore->isChecked());
  ui.userEdit->setEnabled(en);
  ui.passwdEdit->setEnabled(en);
  ui.port->setEnabled(en && !ui.internalCore->isChecked());
  ui.editAccount->setEnabled(en);
  ui.delAccount->setEnabled(en);
  ui.internalCore->setEnabled(en);
  ui.rememberPasswd->setEnabled(en);
  //ui.autoConnect->setEnabled(en);
  ui.autoConnect->setEnabled(false); // FIXME temporär
  ui.buttonBox1->button(QDialogButtonBox::Ok)->setEnabled(en && checkInputValid());
}

void CoreConnectDlg::accountChanged(const QString &text) {
  AccountSettings s;
  if(!curacc.isEmpty()) {
    VarMap oldAcc;
    oldAcc["User"] = ui.userEdit->text();
    oldAcc["Host"] = ui.hostEdit->text();
    oldAcc["Port"] = ui.port->value();
    oldAcc["InternalCore"] = ui.internalCore->isChecked();
    if(ui.rememberPasswd->isChecked()) oldAcc["Password"] = ui.passwdEdit->text();
    s.setValue(curacc, "AccountData", oldAcc);
  }
  ui.autoConnect->setChecked(false);
  if(!text.isEmpty()) { // empty text: just save stuff
    curacc = text;
    s.setLastAccount(curacc);
    VarMap newAcc = s.value(curacc, "AccountData").toMap();
    ui.userEdit->setText(newAcc["User"].toString());
    ui.hostEdit->setText(newAcc["Host"].toString());
    ui.port->setValue(newAcc["Port"].toInt());
    ui.internalCore->setChecked(newAcc["InternalCore"].toBool());
    if(newAcc.contains("Password")) {
      ui.passwdEdit->setText(newAcc["Password"].toString());
      ui.rememberPasswd->setChecked(true);
    } else ui.rememberPasswd->setChecked(false);
    if(s.autoConnectAccount() == curacc) ui.autoConnect->setChecked(true);
  }
}

void CoreConnectDlg::autoConnectToggled(bool autoConnect) {
  AccountSettings s;
  if(autoConnect) s.setAutoConnectAccount(curacc);
  else s.setAutoConnectAccount("");
}

bool CoreConnectDlg::checkInputValid() {
  bool res = (ui.internalCore->isChecked() || ui.hostEdit->text().count()) && ui.userEdit->text().count();
  ui.buttonBox1->button(QDialogButtonBox::Ok)->setEnabled(res);
  return res;
}

void CoreConnectDlg::createAccount() {
  QString accname = QInputDialog::getText(this, tr("Create Account"), tr("Please enter a name for the new account:"));
  if(accname.isEmpty()) return;
  if(ui.accountList->findText(accname) >= 0) {
    QMessageBox::warning(this, tr("Account name already exists!"), tr("An account named '%1' already exists, and account names must be unique!").arg(accname));
    return;
  }
  VarMap defdata;
  ui.accountList->addItem(accname);
  ui.accountList->setCurrentIndex(ui.accountList->findText(accname));
  setAccountEditEnabled(true);
}

void CoreConnectDlg::removeAccount() {
  QString acc = ui.accountList->currentText();
  int res = QMessageBox::warning(this, tr("Delete account?"), tr("Do you really want to delete the data for the account '%1'?<br>"
                                                       "Note that this only affects your local account settings and will not remove "
                                                       "any data from the core.").arg(acc),
                             QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
  if(res == QMessageBox::Yes) {
    AccountSettings s;
    s.removeAccount(acc);
    curacc = "";
    ui.accountList->removeItem(ui.accountList->findText(acc));
    if(!ui.accountList->count()) setAccountEditEnabled(false);
  }
}

bool CoreConnectDlg::willDoInternalAutoConnect() {
  AccountSettings s;
  if(ui.autoConnect->isChecked() && s.autoConnectAccount() == curacc && ui.internalCore->isChecked()) {
    return true;
  }
  return false;
}

void CoreConnectDlg::doAutoConnect() {
  AccountSettings s;
  if(s.autoConnectAccount() == curacc) {
    doConnect();
  }
}

void CoreConnectDlg::doConnect() {
  accountChanged(); // save current account info

  VarMap conninfo;
  ui.stackedWidget->setCurrentIndex(1);
  if(ui.internalCore->isChecked()) {
    ui.connectionGroupBox->setTitle(tr("Connecting to internal core"));
    ui.connectionProgress->hide();
  } else {
    ui.connectionGroupBox->setTitle(tr("Connecting to %1").arg(ui.hostEdit->text()));
    conninfo["Host"] = ui.hostEdit->text();
    conninfo["Post"] = ui.port->value();
  }
  conninfo["User"] = ui.userEdit->text();
  conninfo["Password"] = ui.passwdEdit->text();
  ui.profileLabel->hide(); ui.guiProfile->hide();
  ui.newGuiProfile->hide(); ui.alwaysUseProfile->hide();
  ui.connectionProgress->show();
  try {
    Client::instance()->connectToCore(conninfo);
  } catch(Exception e) {
    QString msg;
    //if(!e.msg().isEmpty()) msg = tr("<br>%1").arg(e.msg()); // FIXME throw more detailed (vulgo: any) error msg
    coreConnectionError(tr("Invalid user or password. Pleasy try again.%1").arg(msg));
    //QMessageBox::warning(this, tr("Unknown account"), tr("Invalid user or password. Pleasy try again.%1").arg(msg));
    //cancelConnect();
    return;
  }
}

void CoreConnectDlg::cancelConnect() {
  ui.stackedWidget->setCurrentIndex(0);
}

void CoreConnectDlg::setStartState() { /*
  ui.hostName->show(); ui.hostPort->show(); ui.hostLabel->show(); ui.portLabel->show();
  ui.statusText->setText(tr("Connect to Quassel Core running on:"));
  ui.buttonBox->button(QDialogButtonBox::Ok)->show();
  ui.hostName->setEnabled(true); ui.hostPort->setEnabled(true);
  ui.hostName->setSelection(0, ui.hostName->text().length()); */
  ui.stackedWidget->setCurrentIndex(0);
}

void CoreConnectDlg::hostEditChanged(QString /*txt*/) {
  //ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(txt.length());
}

void CoreConnectDlg::hostSelected() { /*
  ui.hostName->hide(); ui.hostPort->hide(); ui.hostLabel->hide(); ui.portLabel->hide();
  ui.statusText->setText(tr("Connecting to %1:%2" ).arg(ui.hostName->text()).arg(ui.hostPort->value()));
  ui.buttonBox->button(QDialogButtonBox::Ok)->hide();
  connect(ClientProxy::instance(), SIGNAL(coreConnected()), this, SLOT(coreConnected()));
  connect(ClientProxy::instance(), SIGNAL(coreConnectionError(QString)), this, SLOT(coreConnectionError(QString)));
  Client::instance()->connectToCore(ui.hostName->text(), ui.hostPort->value());
*/
}

void CoreConnectDlg::coreConnected() { /*
  ui.hostLabel->hide(); ui.hostName->hide(); ui.portLabel->hide(); ui.hostPort->hide();
  ui.statusText->setText(tr("Synchronizing..."));
  QSettings s;
  s.setValue("GUI/CoreHost", ui.hostName->text());
  s.setValue("GUI/CorePort", ui.hostPort->value());
  s.setValue("GUI/CoreAutoConnect", ui.autoConnect->isChecked());
  connect(ClientProxy::instance(), SIGNAL(recvPartialItem(quint32, quint32)), this, SLOT(updateProgressBar(quint32, quint32)));
  connect(ClientProxy::instance(), SIGNAL(csCoreState(QVariant)), this, SLOT(recvCoreState(QVariant)));
  ui.progressBar->show();
  VarMap initmsg;
  initmsg["GUIProtocol"] = GUI_PROTOCOL;
  // FIXME guiProxy->send(GS_CLIENT_INIT, QVariant(initmsg)); */
  ui.connectionStatus->setText(tr("Connected to core."));
  accept();
}

void CoreConnectDlg::coreConnectionError(QString err) {
  ui.stackedWidget->setCurrentIndex(0);
  show(); // just in case we started hidden
  QMessageBox::warning(this, tr("Connection Error"), tr("<b>Could not connect to Quassel Core!</b><br>\n") + err, QMessageBox::Retry);
  disconnect(ClientProxy::instance(), 0, this, 0);
  //ui.autoConnect->setChecked(false);
  setStartState();
}

void CoreConnectDlg::updateProgressBar(uint partial, uint total) {
  ui.connectionProgress->setMaximum(total);
  ui.connectionProgress->setValue(partial);
  //qDebug() << "progress:" << partial << total;
}

void CoreConnectDlg::recvCoreState(QVariant state) {
  //ui.progressBar->hide();
  coreState = state;
  accept();
}

QVariant CoreConnectDlg::getCoreState() {
  return coreState;
}

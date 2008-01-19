/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include <QDebug>
#include <QMessageBox>

#include "coreconnectdlg.h"

#include "clientsettings.h"
#include "clientsyncer.h"

CoreConnectDlg::CoreConnectDlg(QWidget *parent, bool autoconnect) : QDialog(parent) {
  ui.setupUi(this);

  clientSyncer = new ClientSyncer(this);

  setAttribute(Qt::WA_DeleteOnClose);

  doingAutoConnect = false;

  ui.stackedWidget->setCurrentWidget(ui.accountPage);
  ui.accountButtonBox->setFocus();
  ui.accountButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);

  CoreAccountSettings s;
  QString lastacc = s.lastAccount();
  autoConnectAccount = s.autoConnectAccount();
  accounts = s.retrieveAllAccounts();
  ui.accountList->addItems(accounts.keys());
  QList<QListWidgetItem *> l = ui.accountList->findItems(lastacc, Qt::MatchExactly);
  if(l.count()) ui.accountList->setCurrentItem(l[0]);
  else ui.accountList->setCurrentRow(0);

  setAccountWidgetStates();

  connect(clientSyncer, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)),this, SLOT(initPhaseSocketState(QAbstractSocket::SocketState)));
  connect(clientSyncer, SIGNAL(connectionError(const QString &)), this, SLOT(initPhaseError(const QString &)));
  connect(clientSyncer, SIGNAL(connectionMsg(const QString &)), this, SLOT(initPhaseMsg(const QString &)));
  connect(clientSyncer, SIGNAL(startLogin()), this, SLOT(startLogin()));
  connect(clientSyncer, SIGNAL(loginFailed(const QString &)), this, SLOT(loginFailed(const QString &)));
  connect(clientSyncer, SIGNAL(loginSuccess()), this, SLOT(startSync()));
  connect(clientSyncer, SIGNAL(sessionProgress(quint32, quint32)), this, SLOT(coreSessionProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(networksProgress(quint32, quint32)), this, SLOT(coreNetworksProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(channelsProgress(quint32, quint32)), this, SLOT(coreChannelsProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(ircUsersProgress(quint32, quint32)), this, SLOT(coreIrcUsersProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(syncFinished()), this, SLOT(accept()));

  connect(ui.user, SIGNAL(textChanged(const QString &)), this, SLOT(setLoginWidgetStates()));
  connect(ui.password, SIGNAL(textChanged(const QString &)), this, SLOT(setLoginWidgetStates()));

  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(restartPhaseNull()));
  connect(ui.syncButtonBox->button(QDialogButtonBox::Abort), SIGNAL(clicked()), this, SLOT(restartPhaseNull()));

  if(autoconnect && ui.accountList->count() && !autoConnectAccount.isEmpty() && autoConnectAccount == ui.accountList->currentItem()->text()) {
    doingAutoConnect = true;
    on_accountButtonBox_accepted();
  }
}

CoreConnectDlg::~CoreConnectDlg() {
  if(ui.accountList->selectedItems().count()) {
    CoreAccountSettings s;
    s.setLastAccount(ui.accountList->selectedItems()[0]->text());
  }
}


/****************************************************
 * Account Management
 ***************************************************/

void CoreConnectDlg::on_accountList_itemSelectionChanged() {
  setAccountWidgetStates();
}

void CoreConnectDlg::setAccountWidgetStates() {
  QList<QListWidgetItem *> selectedItems = ui.accountList->selectedItems();
  ui.editAccount->setEnabled(selectedItems.count());
  ui.deleteAccount->setEnabled(selectedItems.count());
  ui.autoConnect->setEnabled(selectedItems.count());
  if(selectedItems.count()) {
    ui.autoConnect->setChecked(selectedItems[0]->text() == autoConnectAccount);
  }
}

void CoreConnectDlg::on_autoConnect_clicked(bool state) {
  if(!state) {
    autoConnectAccount = QString();
  } else {
    if(ui.accountList->selectedItems().count()) {
      autoConnectAccount = ui.accountList->selectedItems()[0]->text();
    } else {
      qWarning() << "Checked auto connect without an enabled item!";  // should never happen!
      autoConnectAccount = QString();
    }
  }
  setAccountWidgetStates();
}

void CoreConnectDlg::on_addAccount_clicked() {
  QStringList existing;
  for(int i = 0; i < ui.accountList->count(); i++) existing << ui.accountList->item(i)->text();
  CoreAccountEditDlg dlg(QString(), QVariantMap(), existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    accounts[dlg.accountName()] = dlg.accountData();
    ui.accountList->addItem(dlg.accountName());
    ui.accountList->setCurrentItem(ui.accountList->findItems(dlg.accountName(), Qt::MatchExactly)[0]);
  }
}

void CoreConnectDlg::on_editAccount_clicked() {
  QStringList existing;
  for(int i = 0; i < ui.accountList->count(); i++) existing << ui.accountList->item(i)->text();
  QString current = ui.accountList->currentItem()->text();
  QVariantMap acct = accounts[current];
  CoreAccountEditDlg dlg(current, acct, existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    if(current != dlg.accountName()) {
      if(autoConnectAccount == current) autoConnectAccount = dlg.accountName();
      accounts.remove(current);
      current = dlg.accountName();
      ui.accountList->currentItem()->setText(current);
    }
    accounts[current] = dlg.accountData();
  }
  //ui.accountList->setCurrent
}

void CoreConnectDlg::on_deleteAccount_clicked() {
  QString current = ui.accountList->currentItem()->text();
  int ret = QMessageBox::question(this, tr("Remove Account Settings"),
                                  tr("Do you really want to remove your local settings for this Quassel Core account?<br>"
                                  "Note: This will <em>not</em> remove or change any data on the Core itself!"),
                                  QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
  if(ret == QMessageBox::Yes) {
    int idx = ui.accountList->currentRow();
    delete ui.accountList->item(idx);
    ui.accountList->setCurrentRow(qMin(idx, ui.accountList->count()));
    accounts.remove(current);
  }
}

void CoreConnectDlg::on_accountList_itemDoubleClicked(QListWidgetItem *item) {
  Q_UNUSED(item);
  on_accountButtonBox_accepted();
}

void CoreConnectDlg::on_accountButtonBox_accepted() {
  // save accounts
  CoreAccountSettings s;
  s.storeAllAccounts(accounts);
  s.setAutoConnectAccount(autoConnectAccount);

  ui.stackedWidget->setCurrentWidget(ui.loginPage);
  accountName = ui.accountList->currentItem()->text();
  account = s.retrieveAccount(accountName);
  s.setLastAccount(accountName);
  connectToCore();
}

/*****************************************************
 * Connecting to the Core
 ****************************************************/

/*** Phase One: initializing the core connection ***/

void CoreConnectDlg::connectToCore() {
  ui.connectIcon->setPixmap(QPixmap::fromImage(QImage(":/22x22/actions/network-disconnect")));
  ui.connectLabel->setText(tr("Connect to %1").arg(account["Host"].toString()));
  ui.coreInfoLabel->setText("");
  ui.loginStack->setCurrentWidget(ui.loginEmptyPage);
  ui.loginButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
  disconnect(ui.loginButtonBox, 0, this, 0);
  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(restartPhaseNull()));


  //connect(Client::instance(), SIGNAL(coreConnectionPhaseOne(const QVariantMap &)), this, SLOT(phaseOneFinished
  clientSyncer->connectToCore(account);
}

void CoreConnectDlg::initPhaseError(const QString &error) {
  doingAutoConnect = false;
  ui.connectIcon->setPixmap(QPixmap::fromImage(QImage(":/22x22/status/dialog-error")));
  //ui.connectLabel->setBrush(QBrush("red"));
  ui.connectLabel->setText(tr("<div style=color:red;>Connection to %1 failed!</div>").arg(account["Host"].toString()));
  ui.coreInfoLabel->setText(error);
  ui.loginButtonBox->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Cancel);
  ui.loginButtonBox->button(QDialogButtonBox::Retry)->setDefault(true);
  disconnect(ui.loginButtonBox, 0, this, 0);
  connect(ui.loginButtonBox, SIGNAL(accepted()), this, SLOT(restartPhaseNull()));
  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CoreConnectDlg::initPhaseMsg(const QString &msg) {
  ui.coreInfoLabel->setText(msg);
}

void CoreConnectDlg::initPhaseSocketState(QAbstractSocket::SocketState state) {
  QString s;
  QString host = account["Host"].toString();
  switch(state) {
    case QAbstractSocket::UnconnectedState: s = tr("Not connected to %1.").arg(host); break;
    case QAbstractSocket::HostLookupState: s = tr("Looking up %1...").arg(host); break;
    case QAbstractSocket::ConnectingState: s = tr("Connecting to %1...").arg(host); break;
    case QAbstractSocket::ConnectedState: s = tr("Connected to %1").arg(host); break;
    default: s = tr("Unknown connection state to %1"); break;
  }
  ui.connectLabel->setText(s);
}

void CoreConnectDlg::restartPhaseNull() {
  doingAutoConnect = false;
  ui.stackedWidget->setCurrentWidget(ui.accountPage);
  clientSyncer->disconnectFromCore();
}

/*********************************************************
 * Phase Two: Login
 *********************************************************/

void CoreConnectDlg::startLogin() {
  ui.connectIcon->setPixmap(QPixmap::fromImage(QImage(":/22x22/actions/network-connect")));
  ui.loginStack->setCurrentWidget(ui.loginCredentialsPage);
  ui.loginStack->setMinimumSize(ui.loginStack->sizeHint()); ui.loginStack->updateGeometry();
  ui.loginButtonBox->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
  if(!account["User"].toString().isEmpty()) {
    ui.user->setText(account["User"].toString());
    if(account["RememberPasswd"].toBool()) {
      ui.password->setText(account["Password"].toString());
      ui.rememberPasswd->setChecked(true);
      ui.loginButtonBox->setFocus();
    } else {
      ui.rememberPasswd->setChecked(false);
      ui.password->setFocus();
    }
  } else ui.user->setFocus();
  disconnect(ui.loginButtonBox, 0, this, 0);
  connect(ui.loginButtonBox, SIGNAL(accepted()), this, SLOT(doLogin()));
  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(restartPhaseNull()));
  if(doingAutoConnect) doLogin();
}

void CoreConnectDlg::doLogin() {
  ui.loginGroup->setTitle(tr("Logging in..."));
  ui.user->setDisabled(true);
  ui.password->setDisabled(true);
  ui.rememberPasswd->setDisabled(true);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
  account["User"] = ui.user->text();
  account["RememberPasswd"] = ui.rememberPasswd->isChecked();
  if(ui.rememberPasswd->isChecked()) account["Password"] = ui.password->text();
  else account.remove("Password");
  CoreAccountSettings s;
  s.storeAccount(accountName, account);
  clientSyncer->loginToCore(account["User"].toString(), account["Password"].toString());
}

void CoreConnectDlg::setLoginWidgetStates() {
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDisabled(ui.user->text().isEmpty() || ui.password->text().isEmpty());
}

void CoreConnectDlg::loginFailed(const QString &error) {
  ui.loginGroup->setTitle(tr("Login"));
  ui.user->setEnabled(true);
  ui.password->setEnabled(true);
  ui.rememberPasswd->setEnabled(true);
  ui.coreInfoLabel->setText(error);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  ui.password->setFocus();
  doingAutoConnect = false;
}

/************************************************************
 * Phase Three: Syncing
 ************************************************************/

void CoreConnectDlg::startSync() {
  ui.sessionProgress->setRange(0, 1);
  ui.sessionProgress->setValue(0);
  ui.networksProgress->setRange(0, 1);
  ui.networksProgress->setValue(0);
  ui.channelsProgress->setRange(0, 1);
  ui.channelsProgress->setValue(0);
  ui.ircUsersProgress->setRange(0, 1);
  ui.ircUsersProgress->setValue(0);

  ui.stackedWidget->setCurrentWidget(ui.syncPage);
  // clean up old page
  ui.loginGroup->setTitle(tr("Login"));
  ui.user->setEnabled(true);
  ui.password->setEnabled(true);
  ui.rememberPasswd->setEnabled(true);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}


void CoreConnectDlg::coreSessionProgress(quint32 val, quint32 max) {
  ui.sessionProgress->setRange(0, max);
  ui.sessionProgress->setValue(val);

}

void CoreConnectDlg::coreNetworksProgress(quint32 val, quint32 max) {
  if(max == 0) {
    ui.networksProgress->setFormat("0/0");
    ui.networksProgress->setRange(0, 1);
    ui.networksProgress->setValue(1);
  } else {
    ui.networksProgress->setFormat("%v/%m");
    ui.networksProgress->setRange(0, max);
    ui.networksProgress->setValue(val);
  }
}

void CoreConnectDlg::coreChannelsProgress(quint32 val, quint32 max) {
  if(max == 0) {
    ui.channelsProgress->setFormat("0/0");
    ui.channelsProgress->setRange(0, 1);
    ui.channelsProgress->setValue(1);
  } else {
    ui.channelsProgress->setFormat("%v/%m");
    ui.channelsProgress->setRange(0, max);
    ui.channelsProgress->setValue(val);
  }
}

void CoreConnectDlg::coreIrcUsersProgress(quint32 val, quint32 max) {
  if(max == 0) {
    ui.ircUsersProgress->setFormat("0/0");
    ui.ircUsersProgress->setRange(0, 1);
    ui.ircUsersProgress->setValue(1);
  } else {
    ui.ircUsersProgress->setFormat("%v/%m");
    ui.ircUsersProgress->setRange(0, max);
    ui.ircUsersProgress->setValue(val);
  }
}

/*****************************************************************************************
 * CoreAccountEditDlg
 *****************************************************************************************/

CoreAccountEditDlg::CoreAccountEditDlg(const QString &name, const QVariantMap &acct, const QStringList &_existing, QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  existing = _existing;
  account = acct;
  if(!name.isEmpty()) {
    existing.removeAll(name);
    ui.host->setText(acct["Host"].toString());
    ui.port->setValue(acct["Port"].toUInt());
    ui.useInternal->setChecked(acct["UseInternal"].toBool());
    ui.accountName->setText(name);
  } else {
    setWindowTitle(tr("Add Core Account"));
  }
}

QString CoreAccountEditDlg::accountName() const {
  return ui.accountName->text();
}

QVariantMap CoreAccountEditDlg::accountData() {
  account["Host"] = ui.host->text();
  account["Port"] = ui.port->value();
  account["UseInternal"] = ui.useInternal->isChecked();
  return account;
}

void CoreAccountEditDlg::setWidgetStates() {
  bool ok = !accountName().isEmpty() && !existing.contains(accountName()) && (ui.useInternal->isChecked() || !ui.host->text().isEmpty());
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void CoreAccountEditDlg::on_host_textChanged(const QString &text) {
  Q_UNUSED(text);
  setWidgetStates();
}

void CoreAccountEditDlg::on_accountName_textChanged(const QString &text) {
  Q_UNUSED(text);
  setWidgetStates();
}

void CoreAccountEditDlg::on_useRemote_toggled(bool state) {
  Q_UNUSED(state);
  setWidgetStates();
}

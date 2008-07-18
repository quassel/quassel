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
#include <QNetworkProxy>

#include "coreconnectdlg.h"

#include "clientsettings.h"
#include "clientsyncer.h"
#include "coreconfigwizard.h"

CoreConnectDlg::CoreConnectDlg(QWidget *parent, bool autoconnect)
  : QDialog(parent)
{
  ui.setupUi(this);

  // make it look more native under Mac OS X:
  setWindowFlags(Qt::Sheet);

  clientSyncer = new ClientSyncer(this);
  wizard = 0;

  setAttribute(Qt::WA_DeleteOnClose);

  doingAutoConnect = false;

  ui.stackedWidget->setCurrentWidget(ui.accountPage);

  CoreAccountSettings s;
  AccountId lastacc = s.lastAccount();
  autoConnectAccount = s.autoConnectAccount();
  QListWidgetItem *currentItem = 0;
  foreach(AccountId id, s.knownAccounts()) {
    if(!id.isValid()) continue;
    QVariantMap data = s.retrieveAccountData(id);
    data["AccountId"] = QVariant::fromValue<AccountId>(id);
    accounts[id] = data;
    QListWidgetItem *item = new QListWidgetItem(data["AccountName"].toString(), ui.accountList);
    item->setData(Qt::UserRole, QVariant::fromValue<AccountId>(id));
    if(id == lastacc) currentItem = item;
  }
  if(currentItem) ui.accountList->setCurrentItem(currentItem);
  else ui.accountList->setCurrentRow(0);

  setAccountWidgetStates();

  ui.accountButtonBox->button(QDialogButtonBox::Ok)->setFocus();
  //ui.accountButtonBox->button(QDialogButtonBox::Ok)->setAutoDefault(true);

  connect(clientSyncer, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)),this, SLOT(initPhaseSocketState(QAbstractSocket::SocketState)));
  connect(clientSyncer, SIGNAL(connectionError(const QString &)), this, SLOT(initPhaseError(const QString &)));
  connect(clientSyncer, SIGNAL(connectionMsg(const QString &)), this, SLOT(initPhaseMsg(const QString &)));
  connect(clientSyncer, SIGNAL(encrypted(bool)), this, SLOT(encrypted(bool)));
  connect(clientSyncer, SIGNAL(startLogin()), this, SLOT(startLogin()));
  connect(clientSyncer, SIGNAL(loginFailed(const QString &)), this, SLOT(loginFailed(const QString &)));
  connect(clientSyncer, SIGNAL(loginSuccess()), this, SLOT(startSync()));
  connect(clientSyncer, SIGNAL(startCoreSetup(const QVariantList &)), this, SLOT(startCoreConfig(const QVariantList &)));
  connect(clientSyncer, SIGNAL(sessionProgress(quint32, quint32)), this, SLOT(coreSessionProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(networksProgress(quint32, quint32)), this, SLOT(coreNetworksProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(syncFinished()), this, SLOT(syncFinished()));

  connect(ui.user, SIGNAL(textChanged(const QString &)), this, SLOT(setLoginWidgetStates()));
  connect(ui.password, SIGNAL(textChanged(const QString &)), this, SLOT(setLoginWidgetStates()));

  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(restartPhaseNull()));
  connect(ui.syncButtonBox->button(QDialogButtonBox::Abort), SIGNAL(clicked()), this, SLOT(restartPhaseNull()));

  if(autoconnect && ui.accountList->count() && autoConnectAccount.isValid()
     && autoConnectAccount == ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>()) {
    doingAutoConnect = true;
    on_accountButtonBox_accepted();
  }
}

CoreConnectDlg::~CoreConnectDlg() {
  if(ui.accountList->selectedItems().count()) {
    CoreAccountSettings s;
    s.setLastAccount(ui.accountList->selectedItems()[0]->data(Qt::UserRole).value<AccountId>());
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
    ui.autoConnect->setChecked(selectedItems[0]->data(Qt::UserRole).value<AccountId>() == autoConnectAccount);
  }
  ui.accountButtonBox->button(QDialogButtonBox::Ok)->setEnabled(ui.accountList->count());
}

void CoreConnectDlg::on_autoConnect_clicked(bool state) {
  if(!state) {
    autoConnectAccount = 0;
  } else {
    if(ui.accountList->selectedItems().count()) {
      autoConnectAccount = ui.accountList->selectedItems()[0]->data(Qt::UserRole).value<AccountId>();
    } else {
      qWarning() << "Checked auto connect without an enabled item!";  // should never happen!
      autoConnectAccount = 0;
    }
  }
  setAccountWidgetStates();
}

void CoreConnectDlg::on_addAccount_clicked() {
  QStringList existing;
  for(int i = 0; i < ui.accountList->count(); i++) existing << ui.accountList->item(i)->text();
  CoreAccountEditDlg dlg(0, QVariantMap(), existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    // find free ID
    AccountId id = accounts.count() + 1;
    for(AccountId i = 1; i <= accounts.count(); i++) {
      if(!accounts.keys().contains(i)) {
        id = i;
        break;
      }
    }
    QVariantMap data = dlg.accountData();
    data["AccountId"] = QVariant::fromValue<AccountId>(id);
    accounts[id] = data;
    QListWidgetItem *item = new QListWidgetItem(data["AccountName"].toString(), ui.accountList);
    item->setData(Qt::UserRole, QVariant::fromValue<AccountId>(id));
    ui.accountList->setCurrentItem(item);
  }
}

void CoreConnectDlg::on_editAccount_clicked() {
  QStringList existing;
  for(int i = 0; i < ui.accountList->count(); i++) existing << ui.accountList->item(i)->text();
  AccountId id = ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>();
  QVariantMap acct = accounts[id];
  CoreAccountEditDlg dlg(id, acct, existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    QVariantMap data = dlg.accountData();
    ui.accountList->currentItem()->setText(data["AccountName"].toString());
    accounts[id] = data;
  }
}

void CoreConnectDlg::on_deleteAccount_clicked() {
  AccountId id = ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>();
  int ret = QMessageBox::question(this, tr("Remove Account Settings"),
                                  tr("Do you really want to remove your local settings for this Quassel Core account?<br>"
                                  "Note: This will <em>not</em> remove or change any data on the Core itself!"),
                                  QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
  if(ret == QMessageBox::Yes) {
    int idx = ui.accountList->currentRow();
    delete ui.accountList->takeItem(idx);
    ui.accountList->setCurrentRow(qMin(idx, ui.accountList->count()-1));
    accounts[id]["Delete"] = true;  // we only flag this here, actual deletion happens on accept!
    setAccountWidgetStates();
  }
}

void CoreConnectDlg::on_accountList_itemDoubleClicked(QListWidgetItem *item) {
  Q_UNUSED(item);
  on_accountButtonBox_accepted();
}

void CoreConnectDlg::on_accountButtonBox_accepted() {
  // save accounts
  CoreAccountSettings s;
  foreach(QVariantMap acct, accounts.values()) {
    AccountId id = acct["AccountId"].value<AccountId>();
    if(acct.contains("Delete")) {
      s.removeAccount(id);
    } else {
      s.storeAccountData(id, acct);
    }
  }
  s.setAutoConnectAccount(autoConnectAccount);

  ui.stackedWidget->setCurrentWidget(ui.loginPage);
  account = ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>();
  accountData = accounts[account];
  s.setLastAccount(account);
  connectToCore();
}

/*****************************************************
 * Connecting to the Core
 ****************************************************/

/*** Phase One: initializing the core connection ***/

void CoreConnectDlg::connectToCore() {
  ui.secureConnection->hide();
  ui.connectIcon->setPixmap(QPixmap::fromImage(QImage(":/22x22/actions/network-disconnect")));
  ui.connectLabel->setText(tr("Connect to %1").arg(accountData["Host"].toString()));
  ui.coreInfoLabel->setText("");
  ui.loginStack->setCurrentWidget(ui.loginEmptyPage);
  ui.loginButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
  disconnect(ui.loginButtonBox, 0, this, 0);
  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(restartPhaseNull()));

  clientSyncer->connectToCore(accountData);
}

void CoreConnectDlg::initPhaseError(const QString &error) {
  doingAutoConnect = false;
  ui.secureConnection->hide();
  ui.connectIcon->setPixmap(QPixmap::fromImage(QImage(":/22x22/status/dialog-error")));
  //ui.connectLabel->setBrush(QBrush("red"));
  ui.connectLabel->setText(tr("<div style=color:red;>Connection to %1 failed!</div>").arg(accountData["Host"].toString()));
  ui.coreInfoLabel->setText(error);
  ui.loginButtonBox->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Cancel);
  ui.loginButtonBox->button(QDialogButtonBox::Retry)->setFocus();
  disconnect(ui.loginButtonBox, 0, this, 0);
  connect(ui.loginButtonBox, SIGNAL(accepted()), this, SLOT(restartPhaseNull()));
  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CoreConnectDlg::initPhaseMsg(const QString &msg) {
  ui.coreInfoLabel->setText(msg);
}

void CoreConnectDlg::encrypted(bool useSsl) {
  if(useSsl)
    ui.secureConnection->show();
  else
    ui.secureConnection->hide();
}

void CoreConnectDlg::initPhaseSocketState(QAbstractSocket::SocketState state) {
  QString s;
  QString host = accountData["Host"].toString();
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
  //ui.loginStack->setMinimumSize(ui.loginStack->sizeHint()); ui.loginStack->updateGeometry();
  ui.loginButtonBox->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setFocus();
  if(!accountData["User"].toString().isEmpty()) {
    ui.user->setText(accountData["User"].toString());
    if(accountData["RememberPasswd"].toBool()) {
      ui.password->setText(accountData["Password"].toString());
      ui.rememberPasswd->setChecked(true);
      ui.loginButtonBox->button(QDialogButtonBox::Ok)->setFocus();
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
  QVariantMap loginData;
  loginData["User"] = ui.user->text();
  loginData["Password"] = ui.password->text();
  loginData["RememberPasswd"] = ui.rememberPasswd->isChecked();
  doLogin(loginData);
}

void CoreConnectDlg::doLogin(const QVariantMap &loginData) {
  disconnect(ui.loginButtonBox, 0, this, 0);
  connect(ui.loginButtonBox, SIGNAL(accepted()), this, SLOT(doLogin()));
  connect(ui.loginButtonBox, SIGNAL(rejected()), this, SLOT(restartPhaseNull()));
  ui.loginStack->setCurrentWidget(ui.loginCredentialsPage);
  ui.loginGroup->setTitle(tr("Logging in..."));
  ui.user->setDisabled(true);
  ui.password->setDisabled(true);
  ui.rememberPasswd->setDisabled(true);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
  accountData["User"] = loginData["User"];
  accountData["RememberPasswd"] = loginData["RememberPasswd"];
  if(loginData["RememberPasswd"].toBool()) accountData["Password"] = loginData["Password"];
  else accountData.remove("Password");
  ui.user->setText(loginData["User"].toString());
  ui.password->setText(loginData["Password"].toString());
  ui.rememberPasswd->setChecked(loginData["RememberPasswd"].toBool());
  CoreAccountSettings s;
  s.storeAccountData(account, accountData);
  clientSyncer->loginToCore(loginData["User"].toString(), loginData["Password"].toString());
}

void CoreConnectDlg::setLoginWidgetStates() {
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setDisabled(ui.user->text().isEmpty() || ui.password->text().isEmpty());
}

void CoreConnectDlg::loginFailed(const QString &error) {
  if(wizard) {
    wizard->reject();
  }
  ui.loginStack->setCurrentWidget(ui.loginCredentialsPage);
  ui.loginGroup->setTitle(tr("Login"));
  ui.user->setEnabled(true);
  ui.password->setEnabled(true);
  ui.rememberPasswd->setEnabled(true);
  ui.coreInfoLabel->setText(error);
  ui.loginButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  ui.password->setFocus();
  doingAutoConnect = false;
}

void CoreConnectDlg::startCoreConfig(const QVariantList &backends) {
  storageBackends = backends;
  ui.loginStack->setCurrentWidget(ui.coreConfigPage);

  //on_launchCoreConfigWizard_clicked();

}

void CoreConnectDlg::on_launchCoreConfigWizard_clicked() {
  Q_ASSERT(!wizard);
  wizard = new CoreConfigWizard(storageBackends, this);
  connect(wizard, SIGNAL(setupCore(const QVariant &)), clientSyncer, SLOT(doCoreSetup(const QVariant &)));
  connect(wizard, SIGNAL(loginToCore(const QVariantMap &)), this, SLOT(doLogin(const QVariantMap &)));
  connect(clientSyncer, SIGNAL(coreSetupSuccess()), wizard, SLOT(coreSetupSuccess()));
  connect(clientSyncer, SIGNAL(coreSetupFailed(const QString &)), wizard, SLOT(coreSetupFailed(const QString &)));
  connect(wizard, SIGNAL(accepted()), this, SLOT(configWizardAccepted()));
  connect(wizard, SIGNAL(rejected()), this, SLOT(configWizardRejected()));
  connect(clientSyncer, SIGNAL(loginSuccess()), wizard, SLOT(loginSuccess()));
  connect(clientSyncer, SIGNAL(syncFinished()), wizard, SLOT(syncFinished()));
  wizard->show();
}

void CoreConnectDlg::configWizardAccepted() {

  wizard->deleteLater();
  wizard = 0;
}

void CoreConnectDlg::configWizardRejected() {

  wizard->deleteLater();
  wizard = 0;
  //exit(1); // FIXME
}


/************************************************************
 * Phase Three: Syncing
 ************************************************************/

void CoreConnectDlg::startSync() {
  ui.sessionProgress->setRange(0, 1);
  ui.sessionProgress->setValue(0);
  ui.networksProgress->setRange(0, 1);
  ui.networksProgress->setValue(0);

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

void CoreConnectDlg::syncFinished() {
  if(!wizard) accept();
  else {
    hide();
    disconnect(wizard, 0, this, 0);
    connect(wizard, SIGNAL(finished(int)), this, SLOT(accept()));
  }
}

/*****************************************************************************************
 * CoreAccountEditDlg
 *****************************************************************************************/
CoreAccountEditDlg::CoreAccountEditDlg(AccountId id, const QVariantMap &acct, const QStringList &_existing, QWidget *parent)
  : QDialog(parent)
{
  ui.setupUi(this);
  existing = _existing;
  if(id.isValid()) {
    existing.removeAll(acct["AccountName"].toString());
    ui.host->setText(acct["Host"].toString());
    ui.port->setValue(acct["Port"].toUInt());
    ui.useInternal->setChecked(acct["UseInternal"].toBool());
    ui.accountName->setText(acct["AccountName"].toString());
#ifndef QT_NO_OPENSSL
    ui.useSsl->setChecked(acct["useSsl"].toBool());
#else
    ui.useSsl->setChecked(false);
    ui.useSsl->setEnabled(false);
#endif
    ui.useProxy->setChecked(acct["useProxy"].toBool());
    ui.proxyHost->setText(acct["proxyHost"].toString());
    ui.proxyPort->setValue(acct["proxyPort"].toUInt());
    ui.proxyType->setCurrentIndex(acct["proxyType"].toInt() == QNetworkProxy::Socks5Proxy ? 0 : 1);
    ui.proxyUser->setText(acct["proxyUser"].toString());
    ui.proxyPassword->setText(acct["proxyPassword"].toString());
  } else {
    setWindowTitle(tr("Add Core Account"));
#ifdef QT_NO_OPENSSL
    ui.useSsl->setChecked(false);
    ui.useSsl->setEnabled(false);
#endif
  }

#ifndef BUILD_MONO
  // if we don't have a mono build we hide the option to use the internal connection and force the setting to use remote host
  ui.useInternal->setChecked(false);
  ui.useInternal->hide();
  ui.useRemote->hide();
  ui.labelUseBuiltinCore->hide();
#endif
}

QVariantMap CoreAccountEditDlg::accountData() {
  account["AccountName"] = ui.accountName->text().trimmed();
  account["Host"] = ui.host->text().trimmed();
  account["Port"] = ui.port->value();
  account["UseInternal"] = ui.useInternal->isChecked();
  account["useSsl"] = ui.useSsl->isChecked();
  account["useProxy"] = ui.useProxy->isChecked();
  account["proxyHost"] = ui.proxyHost->text().trimmed();
  account["proxyPort"] = ui.proxyPort->value();
  account["proxyType"] = ui.proxyType->currentIndex() == 0 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy;
  account["proxyUser"] = ui.proxyUser->text().trimmed();
  account["proxyPassword"] = ui.proxyPassword->text().trimmed();
  return account;
}

void CoreAccountEditDlg::setWidgetStates() {
  bool ok = !ui.accountName->text().trimmed().isEmpty() && !existing.contains(ui.accountName->text()) && (ui.useInternal->isChecked() || !ui.host->text().isEmpty());
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

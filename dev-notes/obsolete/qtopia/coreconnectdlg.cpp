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
#define DEVELMODE
#include <QtGui>
#include <QSoftMenuBar>

#include "coreconnectdlg.h"
#include "client.h"
#include "clientsettings.h"
#include "clientsyncer.h"
#include "global.h"

CoreConnectDlg::CoreConnectDlg(QWidget *parent, bool /*doAutoConnect*/) : QDialog(parent) {
  ui.setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);

  clientSyncer = new ClientSyncer(this);
  connect(clientSyncer, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)),this, SLOT(initPhaseSocketState(QAbstractSocket::SocketState)));
  connect(clientSyncer, SIGNAL(connectionError(const QString &)), this, SLOT(initPhaseError(const QString &)));
  connect(clientSyncer, SIGNAL(connectionMsg(const QString &)), this, SLOT(initPhaseMsg(const QString &)));
  connect(clientSyncer, SIGNAL(startLogin()), this, SLOT(startLogin()));
  connect(clientSyncer, SIGNAL(loginFailed(const QString &)), this, SLOT(loginFailed(const QString &)));
  connect(clientSyncer, SIGNAL(loginSuccess()), this, SLOT(startSync()));
  //connect(clientSyncer, SIGNAL(startCoreSetup(const QVariantList &)), this, SLOT(startCoreConfig(const QVariantList &)));

  QMenu *menu = new QMenu(this);
  newAccAction = new QAction(QIcon(":icon/new"), tr("New"), this);
  delAccAction = new QAction(QIcon(":icon/trash"), tr("Delete"), this);
  editAccAction = new QAction(QIcon(":icon/settings"), tr("Properties..."), this);
  menu->addAction(newAccAction);
  menu->addAction(delAccAction);
  menu->addAction(editAccAction);
  QSoftMenuBar::addMenuTo(this, menu);
  QSoftMenuBar::setCancelEnabled(this, true);
  ui.newAccount->setDefaultAction(newAccAction);
  ui.delAccount->setDefaultAction(delAccAction);
  ui.editAccount->setDefaultAction(editAccAction);
  connect(newAccAction, SIGNAL(triggered()), this, SLOT(createAccount()));
  connect(delAccAction, SIGNAL(triggered()), this, SLOT(removeAccount()));
  connect(editAccAction, SIGNAL(triggered()), this, SLOT(editAccount()));
  connect(ui.accountList, SIGNAL(itemSelectionChanged()), this, SLOT(setWidgetStates()));
  connect(ui.doConnect, SIGNAL(clicked()), this, SLOT(doConnect()));

  ui.accountList->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.accountList->setSortingEnabled(true);

  CoreAccountSettings s;
  AccountId lastacc = s.lastAccount();
  autoConnectAccount = s.autoConnectAccount();
  QListWidgetItem *currentItem = 0;
  foreach(AccountId id, s.knownAccounts()) {
    if(!id.isValid()) continue;
    QVariantMap data = s.retrieveAccountData(id);
    accounts[id] = data;
    QListWidgetItem *item = new QListWidgetItem(data["AccountName"].toString(), ui.accountList);
    item->setData(Qt::UserRole, QVariant::fromValue<AccountId>(id));
    if(id == lastacc) currentItem = item;
  }
  if(currentItem) ui.accountList->setCurrentItem(currentItem);
  else ui.accountList->setCurrentRow(0);
  setWidgetStates();
#ifdef DEVELMODE
  doConnect(); // shortcut for development
#endif
}

CoreConnectDlg::~CoreConnectDlg() {
  //qDebug() << "destroy";
}

void CoreConnectDlg::setWidgetStates() {
  editAccAction->setEnabled(ui.accountList->selectedItems().count());
  delAccAction->setEnabled(ui.accountList->selectedItems().count());
  ui.doConnect->setEnabled(ui.accountList->selectedItems().count());
}

void CoreConnectDlg::createAccount() {
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
    CoreAccountSettings s;
    s.storeAccountData(id, data);
    QListWidgetItem *item = new QListWidgetItem(data["AccountName"].toString(), ui.accountList);
    item->setData(Qt::UserRole, QVariant::fromValue<AccountId>(id));
    ui.accountList->setCurrentItem(item);
  }
}

void CoreConnectDlg::editAccount() {
  QStringList existing;
  for(int i = 0; i < ui.accountList->count(); i++) existing << ui.accountList->item(i)->text();
  AccountId id = ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>();
  QVariantMap acct = accounts[id];
  CoreAccountEditDlg dlg(id, acct, existing, this);
  if(dlg.exec() == QDialog::Accepted) {
    QVariantMap data = dlg.accountData();
    ui.accountList->currentItem()->setText(data["AccountName"].toString());
    accounts[id] = data;
    CoreAccountSettings s;
    s.storeAccountData(id, data);
  }
}

void CoreConnectDlg::removeAccount() {
  AccountId id = ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>();
  int ret = QMessageBox::question(this, tr("Remove Account Settings"),
                                  tr("Do you really want to remove your local settings for this Quassel Core account?<br>"
                                      "Note: This will <em>not</em> remove or change any data on the Core itself!"),
                                      QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
  if(ret == QMessageBox::Yes) {
    int idx = ui.accountList->currentRow();
    delete ui.accountList->takeItem(idx);
    ui.accountList->setCurrentRow(qMin(idx, ui.accountList->count()-1));
    CoreAccountSettings s;
    s.removeAccount(id);
    setWidgetStates();
  }
}

void CoreConnectDlg::doConnect() {
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

  //ui.stackedWidget->setCurrentWidget(ui.loginPage);
  account = ui.accountList->currentItem()->data(Qt::UserRole).value<AccountId>();
  accountData = accounts[account];
  s.setLastAccount(account);
  
  clientSyncer->connectToCore(accountData);
//  qDebug() << "logging in " << accountData["User"].toString() << accountData["Password"].toString();
//  clientSyncer->loginToCore(accountData["User"].toString(), accountData["Password"].toString());
//  qDebug() << "logged in";
  //connectToCore();
  //if(!ui.accountList->selectedItems().count()) return;
//  AccountSettings s;
//  QVariantMap connInfo; // = s.value(acc, "AccountData").toMap();
  //connInfo["AccountName"] = acc;

  progressDlg = new CoreConnectProgressDlg(clientSyncer, this);
  connect(progressDlg, SIGNAL(accepted()), this, SLOT(connectionSuccess()));
  connect(progressDlg, SIGNAL(rejected()), this, SLOT(connectionFailure()));
  progressDlg->showMaximized();
 // progressDlg->connectToCore(connInfo);
}

void CoreConnectDlg::initPhaseError(const QString &error) {
  qDebug() << "connection error:" << error;
}

void CoreConnectDlg::initPhaseMsg(const QString &msg) {

}

void CoreConnectDlg::initPhaseSocketState(QAbstractSocket::SocketState state) {
  /*
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
  */
}

void CoreConnectDlg::restartPhaseNull() {
  clientSyncer->disconnectFromCore();
}

/*********************************************************
 * Phase Two: Login
 *********************************************************/

void CoreConnectDlg::startLogin() {
  clientSyncer->loginToCore(accountData["User"].toString(), accountData["Password"].toString());
}


void CoreConnectDlg::loginFailed(const QString &error) {

}

void CoreConnectDlg::startSync() {
  
  
}


void CoreConnectDlg::connectionSuccess() {
  /*
  if(progressDlg->isConnected()) {
    progressDlg->deleteLater();
    accept();
  } else {
    connectionFailure();
  }
  */
  accept();
}

void CoreConnectDlg::connectionFailure() {
  progressDlg->deleteLater();
  Client::instance()->disconnectFromCore();
}

QVariant CoreConnectDlg::getCoreState() {
//  return coreState;
}


/****************************************************************************************************/

CoreAccountEditDlg::CoreAccountEditDlg(AccountId id, const QVariantMap &acct, const QStringList &_existing, QWidget *parent) : QDialog(parent), account(acct) {
  ui.setupUi(this);
  setModal(true);
  showMaximized();

  existing = _existing;
  account = acct;
  if(id.isValid()) {
    existing.removeAll(acct["AccountName"].toString());
    ui.hostEdit->setText(acct["Host"].toString());
    ui.port->setValue(acct["Port"].toUInt());
    ui.accountEdit->setText(acct["AccountName"].toString());
    ui.userEdit->setText(acct["User"].toString());
    ui.passwdEdit->setText(acct["Password"].toString());
    ui.hostEdit->setFocus();
  } else {
    ui.port->setValue(Global::defaultPort);
    ui.accountEdit->setFocus();
    setWindowTitle(tr("Add Core Account"));
  }
}

QVariantMap CoreAccountEditDlg::accountData() {
  account["AccountName"] = ui.accountEdit->text().trimmed();
  account["Host"] = ui.hostEdit->text().trimmed();
  account["Port"] = ui.port->value();
  account["User"] = ui.userEdit->text();
  account["Password"] = ui.passwdEdit->text();
  return account;
}

void CoreAccountEditDlg::accept() {
  if(ui.userEdit->text().isEmpty() || ui.hostEdit->text().isEmpty() || ui.accountEdit->text().isEmpty()) {
    int res = QMessageBox::warning(this, tr("Missing information"),
                                   tr("Please enter all required information or discard changes to return to account selection."),
                                      QMessageBox::Discard|QMessageBox::Retry);
    if(res != QMessageBox::Retry) reject();
    return;
  }
  
  if(existing.contains(ui.accountEdit->text())) {
    int res = QMessageBox::warning(this, tr("Non-unique account name"),
                                   tr("Account names need to be unique. Please enter a different name or discard all changes to "
                                      "return to account selection."),
                                      QMessageBox::Discard|QMessageBox::Retry);
    if(res != QMessageBox::Retry) reject();
    ui.accountEdit->setSelection(0, ui.accountEdit->text().length());
    ui.accountEdit->setFocus();
    return;
  }
  QDialog::accept();
}

/********************************************************************************************/

CoreConnectProgressDlg::CoreConnectProgressDlg(ClientSyncer *clientSyncer, QDialog *parent) : QDialog(parent) {
  ui.setupUi(this);

  setModal(true);

  connect(clientSyncer, SIGNAL(sessionProgress(quint32, quint32)), this, SLOT(coreSessionProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(networksProgress(quint32, quint32)), this, SLOT(coreNetworksProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(channelsProgress(quint32, quint32)), this, SLOT(coreChannelsProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(ircUsersProgress(quint32, quint32)), this, SLOT(coreIrcUsersProgress(quint32, quint32)));
  connect(clientSyncer, SIGNAL(syncFinished()), this, SLOT(syncFinished()));

  ui.sessionProgress->setRange(0, 1);
  ui.sessionProgress->setValue(0);
  ui.networksProgress->setRange(0, 1);
  ui.networksProgress->setValue(0);
  ui.channelsProgress->setRange(0, 1);
  ui.channelsProgress->setValue(0);
  ui.ircUsersProgress->setRange(0, 1);
  ui.ircUsersProgress->setValue(0);
}

void CoreConnectProgressDlg::coreSessionProgress(quint32 val, quint32 max) {
  ui.sessionProgress->setRange(0, max);
  ui.sessionProgress->setValue(val);

}

void CoreConnectProgressDlg::coreNetworksProgress(quint32 val, quint32 max) {
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

void CoreConnectProgressDlg::coreChannelsProgress(quint32 val, quint32 max) {
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

void CoreConnectProgressDlg::coreIrcUsersProgress(quint32 val, quint32 max) {
  if(max == 0) {
    ui.ircUsersProgress->setFormat("0/0");
    ui.ircUsersProgress->setRange(0, 1);
    ui.ircUsersProgress->setValue(1);
  } else {
    if(val % 100) return;
    ui.ircUsersProgress->setFormat("%v/%m");
    ui.ircUsersProgress->setRange(0, max);
    ui.ircUsersProgress->setValue(val);
  }
}

void CoreConnectProgressDlg::syncFinished() {
  accept();
}


/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include <QtGui>
#include <QSoftMenuBar>

#include "coreconnectdlg.h"
#include "client.h"
#include "clientsettings.h"
#include "global.h"

CoreConnectDlg::CoreConnectDlg(QWidget *parent, bool /*doAutoConnect*/) : QDialog(parent) {
  ui.setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);

  coreState = 0;

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

  AccountSettings s;
  ui.accountList->addItems(s.knownAccounts());
  if(ui.accountList->count()) ui.accountList->item(0)->setSelected(true);
  setWidgetStates();
  doConnect(); // shortcut for development
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
  editAccount("");
}

void CoreConnectDlg::editAccount() {
  if(!ui.accountList->selectedItems().count()) return;
  QString acc = ui.accountList->selectedItems()[0]->text();
  editAccount(acc);
}

void CoreConnectDlg::editAccount(QString acc) {
  EditCoreAcctDlg *dlg = new EditCoreAcctDlg(acc, this);
  dlg->showMaximized();
  int res = dlg->exec();
  if(res == QDialog::Accepted) {
    AccountSettings s;
    ui.accountList->clear();
    ui.accountList->addItems(s.knownAccounts());
    QList<QListWidgetItem *> list = ui.accountList->findItems(dlg->accountName(), Qt::MatchExactly);
    Q_ASSERT(list.count() == 1);
    list[0]->setSelected(true);
    setWidgetStates();
  }
  dlg->deleteLater();
}

void CoreConnectDlg::removeAccount() {
  if(ui.accountList->selectedItems().count()) {
    QListWidgetItem *item = ui.accountList->selectedItems()[0];
    int res = QMessageBox::warning(this, tr("Delete account?"), tr("Do you really want to delete the data for the account '%1'?<br>"
        "Note that this only affects your local account settings and will not remove "
        "any data from the core.").arg(item->text()), QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if(res == QMessageBox::Yes) {
      AccountSettings s;
      s.removeAccount(item->text());
      item = ui.accountList->takeItem(ui.accountList->row(item));
      delete item;
      setWidgetStates();
    }
  }
}

void CoreConnectDlg::doConnect() {
  if(!ui.accountList->selectedItems().count()) return;
  QString acc = ui.accountList->selectedItems()[0]->text();
  AccountSettings s;
  QVariantMap connInfo = s.value(acc, "AccountData").toMap();
  connInfo["AccountName"] = acc;

  progressDlg = new CoreConnectProgressDlg(this);
  connect(progressDlg, SIGNAL(accepted()), this, SLOT(connectionSuccess()));
  connect(progressDlg, SIGNAL(rejected()), this, SLOT(connectionFailure()));
  progressDlg->showMaximized();
  progressDlg->connectToCore(connInfo);
}

void CoreConnectDlg::connectionSuccess() {
  if(progressDlg->isConnected()) {
    progressDlg->deleteLater();
    accept();
  } else {
    connectionFailure();
  }
}

void CoreConnectDlg::connectionFailure() {
  progressDlg->deleteLater();
  Client::instance()->disconnectFromCore();
}

QVariant CoreConnectDlg::getCoreState() {
  return coreState;
}

/****************************************************************************************************/

EditCoreAcctDlg::EditCoreAcctDlg(QString accname, QDialog *parent) : QDialog(parent), accName(accname) {
  ui.setupUi(this);
  setModal(true);

  ui.accountEdit->setText(accountName());
  if(accName.isEmpty()) {
    ui.port->setValue(DEFAULT_PORT);
    ui.accountEdit->setFocus();
  } else {
    ui.hostEdit->setFocus();
    AccountSettings s;
    QVariantMap data = s.value(accName, "AccountData").toMap();
    ui.hostEdit->setText(data["Host"].toString());
    ui.port->setValue(data["Port"].toUInt());
    ui.userEdit->setText(data["User"].toString());
    //if(data.contains("Password")) {
      ui.passwdEdit->setText(data["Password"].toString());
    //  ui.rememberPasswd->setChecked(true);
    //} else ui.rememberPasswd->setChecked(false);
  }
}

QString EditCoreAcctDlg::accountName() const {
  return accName;
}

void EditCoreAcctDlg::accept() {
  AccountSettings s;
  if(ui.userEdit->text().isEmpty() || ui.hostEdit->text().isEmpty() || ui.accountEdit->text().isEmpty()) {
    int res = QMessageBox::warning(this, tr("Missing information"),
                                   tr("Please enter all required information or discard changes to return to account selection."),
                                      QMessageBox::Discard|QMessageBox::Retry);
    if(res != QMessageBox::Retry) reject();
    return;
  }
  if(ui.accountEdit->text() != accountName() && s.knownAccounts().contains(ui.accountEdit->text())) {
    int res = QMessageBox::warning(this, tr("Non-unique account name"),
                                   tr("Account names need to be unique. Please enter a different name or discard all changes to "
                                      "return to account selection."),
                                      QMessageBox::Discard|QMessageBox::Retry);
    if(res != QMessageBox::Retry) reject();
    ui.accountEdit->setSelection(0, ui.accountEdit->text().length());
    ui.accountEdit->setFocus();
    return;
  }
  if(accountName() != ui.accountEdit->text()) {
    s.removeAccount(accountName());
    accName = ui.accountEdit->text();
  }
  QVariantMap accData;
  accData["User"] = ui.userEdit->text();
  accData["Host"] = ui.hostEdit->text();
  accData["Port"] = ui.port->value();
  accData["Password"] = ui.passwdEdit->text();
  s.setValue(accountName(), "AccountData", accData);
  QDialog::accept();
}

/********************************************************************************************/

CoreConnectProgressDlg::CoreConnectProgressDlg(QDialog *parent) : QDialog(parent) {
  ui.setupUi(this);

  setModal(true);

  connectsuccess = false;

  connect(Client::instance(), SIGNAL(coreConnectionMsg(const QString &)), ui.connectionStatus, SLOT(setText(const QString &)));
  connect(Client::instance(), SIGNAL(coreConnectionProgress(uint, uint)), this, SLOT(updateProgressBar(uint, uint)));
  connect(Client::instance(), SIGNAL(coreConnectionError(QString)), this, SLOT(coreConnectionError(QString)));
  connect(Client::instance(), SIGNAL(connected()), this, SLOT(coreConnected()));

}

bool CoreConnectProgressDlg::isConnected() const {
  return connectsuccess;
}

void CoreConnectProgressDlg::connectToCore(QVariantMap connInfo) {
  Client::instance()->connectToCore(connInfo);

}

void CoreConnectProgressDlg::coreConnected() {
  connectsuccess = true;
  accept();
}

void CoreConnectProgressDlg::coreConnectionError(QString err) {
  QMessageBox::warning(this, tr("Connection Error"), tr("<b>Could not connect to Quassel Core!</b><br>\n") + err, QMessageBox::Ok);
  reject();
}

void CoreConnectProgressDlg::updateProgressBar(uint partial, uint total) {
  ui.connectionProgress->setMaximum(total);
  ui.connectionProgress->setValue(partial);
}


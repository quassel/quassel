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

#ifndef CORECONNECTDLG_H
#define CORECONNECTDLG_H

#include <QAbstractSocket>

#include "types.h"

#include "ui_coreconnectdlg.h"
#include "ui_coreaccounteditdlg.h"

class ClientSyncer;
class CoreConfigWizard;

class CoreConnectDlg : public QDialog {
  Q_OBJECT

public:
  CoreConnectDlg(bool = false, QWidget *parent = 0);
  ~CoreConnectDlg();

// signals:
//   void newClientSyncer(ClientSyncer *);

private slots:
  /*** Phase Null: Accounts ***/
  void restartPhaseNull();

  void on_accountList_itemSelectionChanged();
  void on_autoConnect_clicked(bool);

  void on_addAccount_clicked();
  void on_editAccount_clicked();
  void on_deleteAccount_clicked();
  void on_useInternalCore_clicked();
  void on_viewSslCertButton_clicked();
  void on_ignoreWarningsButton_clicked();

  void on_accountList_itemDoubleClicked(QListWidgetItem *item);
  void on_accountButtonBox_accepted();

  void setAccountWidgetStates();

  /*** Phase One: Connection ***/
  void connectToCore();

  void initPhaseError(const QString &error);
  void initPhaseWarnings(const QStringList &warnings);
  void initPhaseMsg(const QString &msg);
  void initPhaseSocketState(QAbstractSocket::SocketState);

  /*** Phase Two: Login ***/
  void startLogin();
  void doLogin();
  void doLogin(const QVariantMap &loginData);
  void loginFailed(const QString &);
  void startCoreConfig(const QVariantList &backends);
  void configWizardAccepted();
  void configWizardRejected();
  void on_launchCoreConfigWizard_clicked();

  void setLoginWidgetStates();

  /*** Phase Three: Sync ***/
  void startSync();
  void syncFinished();

  void coreSessionProgress(quint32, quint32);
  void coreNetworksProgress(quint32, quint32);

private:
  AccountId findFreeAccountId();

  Ui::CoreConnectDlg ui;

  AccountId autoConnectAccount;
  QHash<AccountId, QVariantMap> accounts;
  AccountId _internalAccountId;
  QVariantMap accountData;
  AccountId account;

  bool doingAutoConnect;

  QVariantList storageBackends;

  ClientSyncer *clientSyncer;
  CoreConfigWizard *wizard;
};


// ========================================
//  CoreAccountEditDlg
// ========================================
class CoreAccountEditDlg : public QDialog {
  Q_OBJECT

public:
  CoreAccountEditDlg(AccountId id, const QVariantMap &data, const QStringList &existing = QStringList(), QWidget *parent = 0);

  QVariantMap accountData();

private slots:
  void on_host_textChanged(const QString &);
  void on_accountName_textChanged(const QString &);

  void setWidgetStates();

private:
  Ui::CoreAccountEditDlg ui;

  QStringList existing;
  QVariantMap account;
};

// ========================================
//  SslCertDisplayDialog
// ========================================
class QSslCertificate;

class SslCertDisplayDialog : public QDialog {
  Q_OBJECT

public:
  SslCertDisplayDialog(const QString &host, const QSslCertificate &cert, QWidget *parent = 0);
};
#endif

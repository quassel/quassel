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

#ifndef _CORECONNECTDLG_H
#define _CORECONNECTDLG_H

#include <QAbstractSocket>

#include "types.h"

#include "ui_coreconnectdlg.h"
#include "ui_coreconnectprogressdlg.h"
#include "ui_coreaccounteditdlg.h"

class ClientSyncer;
class CoreConnectProgressDlg;

class CoreConnectDlg : public QDialog {
  Q_OBJECT

  public:
    CoreConnectDlg(QWidget *parent = 0, bool doAutoConnect = false);
    ~CoreConnectDlg();
    QVariant getCoreState();

  private slots:
    void createAccount();
    void removeAccount();
    void editAccount();
    void setWidgetStates();
    void doConnect();
    void connectionSuccess();
    void connectionFailure();

    /*** Phase One: Connection ***/

    void restartPhaseNull();
    void initPhaseError(const QString &error);
    void initPhaseMsg(const QString &msg);
    void initPhaseSocketState(QAbstractSocket::SocketState);

    /*** Phase Two: Login ***/
    void startLogin();
    void loginFailed(const QString &);

    /*** Phase Three: Sync ***/
    void startSync();

  private:
    Ui::CoreConnectDlg ui;
    ClientSyncer *clientSyncer;

    AccountId autoConnectAccount;
    QHash<AccountId, QVariantMap> accounts;
    QVariantMap accountData;
    AccountId account;

    void editAccount(QString);

    QAction *newAccAction, *editAccAction, *delAccAction;

    CoreConnectProgressDlg *progressDlg;
};

class CoreAccountEditDlg : public QDialog {
  Q_OBJECT

  public:
    CoreAccountEditDlg(AccountId id, const QVariantMap &data, const QStringList &existing = QStringList(), QWidget *parent = 0);
    QVariantMap accountData();

  public slots:
    void accept();

  private slots:


  private:
    Ui::CoreAccountEditDlg ui;
    QVariantMap account;
    QStringList existing;

};

class CoreConnectProgressDlg : public QDialog {
  Q_OBJECT

  public:
    CoreConnectProgressDlg(ClientSyncer *, QDialog *parent = 0);

  private slots:

    void syncFinished();

    void coreSessionProgress(quint32, quint32);
    void coreNetworksProgress(quint32, quint32);
    void coreChannelsProgress(quint32, quint32);
    void coreIrcUsersProgress(quint32, quint32);

  private:
    Ui::CoreConnectProgressDlg ui;

};

#endif

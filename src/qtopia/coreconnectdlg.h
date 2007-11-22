/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
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

#ifndef _CORECONNECTDLG_H
#define _CORECONNECTDLG_H

#include "ui_coreconnectdlg.h"
#include "ui_coreconnectprogressdlg.h"
#include "ui_editcoreacctdlg.h"

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

  private:
    Ui::CoreConnectDlg ui;
    QVariant coreState;

    void editAccount(QString);

    QAction *newAccAction, *editAccAction, *delAccAction;

    CoreConnectProgressDlg *progressDlg;
};

class EditCoreAcctDlg : public QDialog {
  Q_OBJECT

  public:
    EditCoreAcctDlg(QString accname = 0, QDialog *parent = 0);
    QString accountName() const;

  public slots:
    void accept();

  private slots:


  private:
    Ui::EditCoreAcctDlg ui;
    QString accName;

};

class CoreConnectProgressDlg : public QDialog {
  Q_OBJECT

  public:
    CoreConnectProgressDlg(QDialog *parent = 0);
    bool isConnected() const;

  public slots:
    void connectToCore(QVariantMap connInfo);

  private slots:
    void coreConnected();
    void coreConnectionError(QString);
    void updateProgressBar(uint partial, uint total);

  private:
    Ui::CoreConnectProgressDlg ui;
    bool connectsuccess;

};

#endif

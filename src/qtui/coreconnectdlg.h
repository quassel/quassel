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

#include "ui_coreconnectdlg.h"

class CoreConnectDlg: public QDialog {
  Q_OBJECT

  public:
    CoreConnectDlg(QWidget *parent, bool doAutoConnect = false);
    ~CoreConnectDlg();
    QVariant getCoreState();

    bool willDoInternalAutoConnect();
    
  public slots:
    void doAutoConnect();

  private slots:
    void createAccount();
    void removeAccount();
    void accountChanged(const QString & = "");
    void setAccountEditEnabled(bool);
    void autoConnectToggled(bool);
    bool checkInputValid();
    void hostEditChanged(QString);
    void hostSelected();
    void doConnect();

    void coreConnected();
    void coreConnectionError(QString);
    //void coreConnectionMsg(const QString &);
    //void coreConnectionProgress(uint partial, uint total);
    void updateProgressBar(uint partial, uint total);
    void recvCoreState(QVariant);
    
    void showConfigWizard(const QVariantMap &coredata);

  private:
    Ui::CoreConnectDlg ui;
    QVariant coreState;

    void cancelConnect();
    void setStartState();
    QVariantMap accountData;
    QString curacc;
};

#endif

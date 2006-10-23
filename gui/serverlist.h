/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

#ifndef _SERVERLIST_H_
#define _SERVERLIST_H_

#include <QtGui>
#include <QMap>
#include <QList>
#include <QVariant>
#include "global.h"

#include "ui_serverlistdlg.h"
#include "ui_networkeditdlg.h"
#include "ui_servereditdlg.h"

class ServerListDlg : public QDialog {
  Q_OBJECT

  public:
    ServerListDlg(QWidget *parent);
    virtual ~ServerListDlg();

    bool showOnStartup();

  public slots:
    void editIdentities(bool end = false);
    virtual void reject() { exit(0); }
    virtual void accept();

  signals:
    void requestConnect(QStringList networks);

  private slots:
    void updateButtons();
    void on_showOnStartup_stateChanged(int);
    void on_addButton_clicked();
    void on_editButton_clicked();
    void on_deleteButton_clicked();

  private:
    Ui::ServerListDlg ui;

    void updateNetworkTree();
    //VarMap networks;
    //VarMap identities;  <-- this is now stored in global
};

class NetworkEditDlg : public QDialog {
  Q_OBJECT

  public:
    NetworkEditDlg(QWidget *parent, VarMap network);

    VarMap getNetwork() { return network; }
  public slots:
    virtual void accept();

  private slots:
    void on_networkName_textChanged(QString);
    void on_addServer_clicked();
    void on_editServer_clicked();
    void on_deleteServer_clicked();
    void on_upServer_clicked();
    void on_downServer_clicked();
    void on_editIdentities_clicked();

    void updateWidgets();
    void updateServerButtons();
  private:
    Ui::NetworkEditDlg ui;

    VarMap network;
    //VarMap identities;
    QString oldName;

    VarMap createDefaultNetwork();
    QString checkValidity();
};

class ServerEditDlg : public QDialog {
  Q_OBJECT

  public:
    ServerEditDlg(QWidget *parent, VarMap server = VarMap());

    VarMap getServer();

  private slots:
    void on_serverAddress_textChanged();

  private:
    Ui::ServerEditDlg ui;

};

#endif

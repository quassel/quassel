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

#include "ui_serverlistdlg.h"
#include "ui_identitiesdlg.h"
#include "ui_identitieseditdlg.h"
#include "ui_nickeditdlg.h"
#include "ui_networkeditdlg.h"

typedef QMap<QString, QVariant> VarMap;

class ServerListDlg : public QDialog {
  Q_OBJECT

  public:
    ServerListDlg(QWidget *parent);
    virtual ~ServerListDlg();

    bool showOnStartup();

  public slots:
    void editIdentities();

  private slots:
    void updateButtons();
    void on_showOnStartup_stateChanged(int);
    void on_addButton_clicked();

  private:
    Ui::ServerListDlg ui;

    void loadNetworks();
    void storeNetworks();
    void loadIdentities();
    void storeIdentities();

    VarMap networks;
    VarMap identities;
};

class NetworkEditDlg : public QDialog {
  Q_OBJECT

  public:
    NetworkEditDlg(QWidget *parent, VarMap network, VarMap identities);

  private:
    Ui::NetworkEditDlg ui;

    VarMap network;
    VarMap identities;

    VarMap createDefaultNetwork();
};

class IdentitiesDlg : public QDialog {
  Q_OBJECT

  public:
    IdentitiesDlg(QWidget *parent, VarMap identities);

    VarMap getIdentities() { return identities; }
    QMap<QString, QString> getNameMapping() { return nameMapping; }

  public slots:
    virtual void accept();

  private slots:
    void autoAwayChecked();
    void identityChanged(QString);
    void nickSelectionChanged();

    void addNick();
    void editNick();
    void delNick();
    void upNick();
    void downNick();

    void editIdentities();

    void globalDataUpdated(QString);

  private:
    Ui::IdentitiesDlg ui;
    VarMap identities;
    QMap<QString, QString> nameMapping;
    QString lastIdentity;

    QString checkValidity();
    VarMap createDefaultIdentity();
    QString getCurIdentity();
    void updateWidgets();
    void updateIdentity(QString);
};

class NickEditDlg : public QDialog {
  Q_OBJECT

  public:
    NickEditDlg(QWidget *parent, QString nick = QString());

    QString getNick();

  private slots:
    void textChanged(QString);

  private:
    Ui::NickEditDlg ui;

};

class IdentitiesEditDlg : public QDialog {
  Q_OBJECT

  public:
    IdentitiesEditDlg(QWidget *parent, VarMap identities, QMap<QString, QString> mapping, VarMap templ);

    VarMap getIdentities() { return identities; }
    QMap<QString, QString> getMapping() { return mapping; }

  private slots:
    void selectionChanged();

    void addIdentity();
    void duplicateIdentity();
    void renameIdentity();
    void deleteIdentity();

  private:
    Ui::IdentitiesEditDlg ui;

    VarMap identities;
    VarMap identTemplate;
    QMap<QString, QString> mapping;

    void sortList();
};

class RenameIdentityDlg : public QDialog {
  Q_OBJECT

  public:
    RenameIdentityDlg(QWidget *parent, QList<QString> reserved, QString name = QString());

    QString getName();

  private slots:
    void textChanged(QString);

  private:
    Ui::NickEditDlg ui;
    QList<QString> reserved;
};
 
#endif

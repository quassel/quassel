/***************************************************************************
 *   Copyright (C) 2009 by the Quassel Project                             *
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

#ifndef COREACCOUNTSETTINGSPAGE_H_
#define COREACCOUNTSETTINGSPAGE_H_

#include "settingspage.h"

#include "coreaccount.h"

#include "ui_coreaccounteditdlg.h"
#include "ui_coreaccountsettingspage.h"

class CoreAccountModel;

class CoreAccountSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    CoreAccountSettingsPage(QWidget *parent = 0);

    virtual inline bool hasDefaults() const { return false; }

  public slots:
    void save();
    void load();

  private slots:
    void on_addAccountButton_clicked();
    void on_editAccountButton_clicked();
    void on_deleteAccountButton_clicked();

    void setWidgetStates();

    void rowsAboutToBeRemoved(const QModelIndex &index, int start, int end);
    void rowsInserted(const QModelIndex &index, int start, int end);

  private:
    Ui::CoreAccountSettingsPage ui;

    CoreAccountModel *_model;
    inline CoreAccountModel *model() const { return _model; }

    AccountId _lastAccountId, _lastAutoConnectId;

    virtual QVariant loadAutoWidgetValue(const QString &widgetName);
    virtual void saveAutoWidgetValue(const QString &widgetName, const QVariant &value);

    void widgetHasChanged();
    bool testHasChanged();

    inline QString settingsKey() const { return QString("CoreAccounts"); }
};

// ========================================
//  CoreAccountEditDlg
// ========================================
class CoreAccountEditDlg : public QDialog {
  Q_OBJECT

public:
  CoreAccountEditDlg(const CoreAccount &account, QWidget *parent = 0);

  CoreAccount account();

private slots:
  void on_hostName_textChanged(const QString &);
  void on_accountName_textChanged(const QString &);
  void on_user_textChanged(const QString &);

  void setWidgetStates();

private:
  Ui::CoreAccountEditDlg ui;
  CoreAccount _account;
};

#endif

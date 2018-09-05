/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COREACCOUNTSETTINGSPAGE_H_
#define COREACCOUNTSETTINGSPAGE_H_

#include <QNetworkProxy>
#include <QSortFilterProxyModel>

#include "settingspage.h"

#include "coreaccount.h"

#include "ui_coreaccounteditdlg.h"
#include "ui_coreaccountsettingspage.h"

class CoreAccountModel;
class FilteredCoreAccountModel;

class CoreAccountSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    CoreAccountSettingsPage(QWidget *parent = nullptr);

    inline bool hasDefaults() const override { return false; }
    inline bool isStandAlone() const { return _standalone; }

    AccountId selectedAccount() const;

public slots:
    void save() override;
    void load() override;

    void setSelectedAccount(AccountId accId);
    void setStandAlone(bool);

signals:
    void connectToCore(AccountId accId);

private slots:
    void on_addAccountButton_clicked();
    void on_editAccountButton_clicked();
    void on_deleteAccountButton_clicked();
    void on_accountView_doubleClicked(const QModelIndex &index);

    void setWidgetStates();
    void widgetHasChanged();

    void rowsAboutToBeRemoved(const QModelIndex &index, int start, int end);
    void rowsInserted(const QModelIndex &index, int start, int end);

private:
    Ui::CoreAccountSettingsPage ui;

    CoreAccountModel *_model;
    inline CoreAccountModel *model() const { return _model; }
    FilteredCoreAccountModel *_filteredModel;
    inline FilteredCoreAccountModel *filteredModel() const { return _filteredModel; }

    AccountId _lastAccountId, _lastAutoConnectId;
    bool _standalone;

    void editAccount(const QModelIndex &);

    bool testHasChanged();

    inline QString settingsKey() const override { return QString("CoreAccounts"); }
};


// ========================================
//  CoreAccountEditDlg
// ========================================
class CoreAccountEditDlg : public QDialog
{
    Q_OBJECT

public:
    CoreAccountEditDlg(const CoreAccount &account, QWidget *parent = nullptr);

    CoreAccount account();

private slots:
    void on_hostName_textChanged(const QString &);
    void on_accountName_textChanged(const QString &);
    void on_user_textChanged(const QString &);
    void on_radioButtonManualProxy_toggled(bool checked);

    void setWidgetStates();

private:
    Ui::CoreAccountEditDlg ui;
    CoreAccount _account;
    enum ProxyType { NoProxy, SystemProxy, ManualProxy };
};


// ========================================
//  FilteredCoreAccountModel
// ========================================

//! This filters out the internal account from the non-monolithic client's UI
class FilteredCoreAccountModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilteredCoreAccountModel(CoreAccountModel *model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    AccountId _internalAccount;
};


#endif

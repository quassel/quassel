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

#include "coreaccountsettingspage.h"

#include "client.h"
#include "clientsettings.h"
#include "coreaccountmodel.h"
#include "icon.h"
#include "util.h"

CoreAccountSettingsPage::CoreAccountSettingsPage(QWidget* parent)
    : SettingsPage(tr("Remote Cores"), QString(), parent)
    , _lastAccountId(0)
    , _lastAutoConnectId(0)
{
    ui.setupUi(this);
    initAutoWidgets();
    ui.addAccountButton->setIcon(icon::get("list-add"));
    ui.editAccountButton->setIcon(icon::get("document-edit"));
    ui.deleteAccountButton->setIcon(icon::get("edit-delete"));

    _model = new CoreAccountModel(Client::coreAccountModel(), this);
    _filteredModel = new FilteredCoreAccountModel(_model, this);

    ui.accountView->setModel(filteredModel());
    ui.autoConnectAccount->setModel(filteredModel());

    connect(filteredModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &CoreAccountSettingsPage::rowsAboutToBeRemoved);
    connect(filteredModel(), &QAbstractItemModel::rowsInserted, this, &CoreAccountSettingsPage::rowsInserted);

    connect(ui.accountView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CoreAccountSettingsPage::setWidgetStates);
    connect(ui.autoConnectAccount, selectOverload<int>(&QComboBox::currentIndexChanged), this, &CoreAccountSettingsPage::widgetHasChanged);
    setWidgetStates();
}

void CoreAccountSettingsPage::setStandAlone(bool standalone)
{
    _standalone = standalone;
}

void CoreAccountSettingsPage::load()
{
    model()->update(Client::coreAccountModel());
    SettingsPage::load();

    CoreAccountSettings s;

    if (Quassel::runMode() != Quassel::Monolithic) {
        // make sure we don't have selected the internal account as autoconnect account

        if (s.autoConnectOnStartup() && s.autoConnectToFixedAccount()) {
            CoreAccount acc = model()->account(s.autoConnectAccount());
            if (acc.isInternal())
                ui.autoConnectOnStartup->setChecked(false);
        }
    }
    ui.accountView->setCurrentIndex(filteredModel()->index(0, 0));
    ui.accountView->selectionModel()->select(filteredModel()->index(0, 0), QItemSelectionModel::Select);

    QModelIndex idx = filteredModel()->mapFromSource(model()->accountIndex(s.autoConnectAccount()));
    ui.autoConnectAccount->setCurrentIndex(idx.isValid() ? idx.row() : 0);
    ui.autoConnectAccount->setProperty("storedValue", ui.autoConnectAccount->currentIndex());
    setWidgetStates();
    // Mark as no changes made, we just loaded settings
    setChangedState(false);
}

void CoreAccountSettingsPage::save()
{
    SettingsPage::save();
    Client::coreAccountModel()->update(model());
    Client::coreAccountModel()->save();
    CoreAccountSettings s;
    AccountId id = filteredModel()->index(ui.autoConnectAccount->currentIndex(), 0).data(CoreAccountModel::AccountIdRole).value<AccountId>();
    s.setAutoConnectAccount(id);
    ui.autoConnectAccount->setProperty("storedValue", ui.autoConnectAccount->currentIndex());
}

// TODO: Qt 4.6 - replace by proper rowsMoved() semantics
// NOTE: This is the filtered model
void CoreAccountSettingsPage::rowsAboutToBeRemoved(const QModelIndex& index, int start, int end)
{
    _lastAutoConnectId = _lastAccountId = 0;
    if (index.isValid() || start != end)
        return;

    // the current index is removed, so remember it in case it's reinserted immediately afterwards
    AccountId id = filteredModel()->index(start, 0).data(CoreAccountModel::AccountIdRole).value<AccountId>();
    if (start == ui.accountView->currentIndex().row())
        _lastAccountId = id;
    if (start == ui.autoConnectAccount->currentIndex())
        _lastAutoConnectId = id;
}

void CoreAccountSettingsPage::rowsInserted(const QModelIndex& index, int start, int end)
{
    if (index.isValid() || start != end)
        return;

    // check if the inserted index was just removed and select it in that case
    AccountId id = filteredModel()->index(start, 0).data(CoreAccountModel::AccountIdRole).value<AccountId>();
    if (id == _lastAccountId)
        ui.accountView->setCurrentIndex(filteredModel()->index(start, 0));
    if (id == _lastAutoConnectId)
        ui.autoConnectAccount->setCurrentIndex(start);
    _lastAccountId = _lastAutoConnectId = 0;
}

AccountId CoreAccountSettingsPage::selectedAccount() const
{
    QModelIndex index = ui.accountView->currentIndex();
    if (!index.isValid())
        return 0;
    return index.data(CoreAccountModel::AccountIdRole).value<AccountId>();
}

void CoreAccountSettingsPage::setSelectedAccount(AccountId accId)
{
    QModelIndex index = filteredModel()->mapFromSource(model()->accountIndex(accId));
    if (index.isValid())
        ui.accountView->setCurrentIndex(index);
}

void CoreAccountSettingsPage::on_addAccountButton_clicked()
{
    CoreAccountEditDlg dlg(CoreAccount(), this);
    if (dlg.exec() == QDialog::Accepted) {
        AccountId id = model()->createOrUpdateAccount(dlg.account());
        ui.accountView->setCurrentIndex(filteredModel()->mapFromSource(model()->accountIndex(id)));
        widgetHasChanged();
    }
}

void CoreAccountSettingsPage::on_editAccountButton_clicked()
{
    QModelIndex idx = ui.accountView->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    editAccount(idx);
}

void CoreAccountSettingsPage::editAccount(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    CoreAccountEditDlg dlg(model()->account(filteredModel()->mapToSource(index)), this);
    if (dlg.exec() == QDialog::Accepted) {
        AccountId id = model()->createOrUpdateAccount(dlg.account());
        ui.accountView->setCurrentIndex(filteredModel()->mapFromSource(model()->accountIndex(id)));
        widgetHasChanged();
    }
}

void CoreAccountSettingsPage::on_deleteAccountButton_clicked()
{
    if (!ui.accountView->selectionModel()->selectedIndexes().count())
        return;

    AccountId id = ui.accountView->selectionModel()->selectedIndexes().at(0).data(CoreAccountModel::AccountIdRole).value<AccountId>();
    if (id.isValid()) {
        model()->removeAccount(id);
        widgetHasChanged();
    }
}

void CoreAccountSettingsPage::on_accountView_doubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    if (isStandAlone())
        emit connectToCore(index.data(CoreAccountModel::AccountIdRole).value<AccountId>());
    else
        editAccount(index);
}

void CoreAccountSettingsPage::setWidgetStates()
{
    AccountId accId = selectedAccount();
    bool editable = accId.isValid() && accId != model()->internalAccount();

    ui.editAccountButton->setEnabled(editable);
    ui.deleteAccountButton->setEnabled(editable);
}

void CoreAccountSettingsPage::widgetHasChanged()
{
    setChangedState(testHasChanged());
    setWidgetStates();
}

bool CoreAccountSettingsPage::testHasChanged()
{
    if (ui.autoConnectAccount->currentIndex() != ui.autoConnectAccount->property("storedValue").toInt()) {
        return true;
    }
    if (*model() != *Client::coreAccountModel()) {
        return true;
    }

    return false;
}

/*****************************************************************************************
 * CoreAccountEditDlg
 *****************************************************************************************/
CoreAccountEditDlg::CoreAccountEditDlg(const CoreAccount& acct, QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    _account = acct;

    ui.hostName->setText(acct.hostName());
    ui.port->setValue(acct.port());
    ui.accountName->setText(acct.accountName());
    ui.user->setText(acct.user());
    ui.password->setText(acct.password());
    ui.rememberPassword->setChecked(acct.storePassword());

    ui.buttonGroupProxyType->setId(ui.radioButtonNoProxy, 0);
    ui.buttonGroupProxyType->setId(ui.radioButtonSystemProxy, 1);
    ui.buttonGroupProxyType->setId(ui.radioButtonManualProxy, 2);

    bool manualProxy = false;
    switch (acct.proxyType()) {
    case QNetworkProxy::NoProxy:
        ui.buttonGroupProxyType->button(0)->setChecked(true);
        break;
    case QNetworkProxy::DefaultProxy:
        ui.buttonGroupProxyType->button(1)->setChecked(true);
        break;
    case QNetworkProxy::Socks5Proxy:
        ui.buttonGroupProxyType->button(2)->setChecked(true);
        ui.proxyType->setCurrentIndex(0);
        manualProxy = true;
        break;
    case QNetworkProxy::HttpProxy:
        ui.buttonGroupProxyType->button(2)->setChecked(true);
        ui.proxyType->setCurrentIndex(1);
        manualProxy = true;
        break;
    default:
        break;
    }

    if (manualProxy) {
        ui.proxyHostName->setText(acct.proxyHostName());
        ui.proxyPort->setValue(acct.proxyPort());
        ui.proxyType->setEnabled(true);
        ui.proxyUser->setText(acct.proxyUser());
        ui.proxyPassword->setText(acct.proxyPassword());
    }

    if (acct.accountId().isValid())
        setWindowTitle(tr("Edit Core Account"));
    else
        setWindowTitle(tr("Add Core Account"));
}

CoreAccount CoreAccountEditDlg::account()
{
    _account.setAccountName(ui.accountName->text().trimmed());
    _account.setHostName(ui.hostName->text().trimmed());
    _account.setPort(ui.port->value());
    _account.setUser(ui.user->text().trimmed());
    _account.setPassword(ui.password->text());
    _account.setStorePassword(ui.rememberPassword->isChecked());

    QNetworkProxy::ProxyType proxyType = QNetworkProxy::NoProxy;
    int checkedId = ui.buttonGroupProxyType->checkedId();

    switch (checkedId) {
    case NoProxy:  // QNetworkProxy::NoProxy
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        _account.setProxyType(proxyType);
        break;
    case SystemProxy:  // QNetworkProxy::DefaultProxy:
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        _account.setProxyType(QNetworkProxy::DefaultProxy);
        break;
    case ManualProxy:  // QNetworkProxy::Socks5Proxy || QNetworkProxy::HttpProxy
        proxyType = ui.proxyType->currentIndex() == 0 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy;
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        _account.setProxyHostName(ui.proxyHostName->text().trimmed());
        _account.setProxyPort(ui.proxyPort->value());
        _account.setProxyType(proxyType);
        _account.setProxyUser(ui.proxyUser->text().trimmed());
        _account.setProxyPassword(ui.proxyPassword->text());
        break;
    default:
        break;
    }
    return _account;
}

void CoreAccountEditDlg::setWidgetStates()
{
    bool ok = !ui.accountName->text().trimmed().isEmpty() && !ui.user->text().trimmed().isEmpty() && !ui.hostName->text().isEmpty();
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void CoreAccountEditDlg::on_hostName_textChanged(const QString& text)
{
    Q_UNUSED(text);
    setWidgetStates();
}

void CoreAccountEditDlg::on_accountName_textChanged(const QString& text)
{
    Q_UNUSED(text);
    setWidgetStates();
}

void CoreAccountEditDlg::on_user_textChanged(const QString& text)
{
    Q_UNUSED(text)
    setWidgetStates();
}

void CoreAccountEditDlg::on_radioButtonManualProxy_toggled(bool checked)
{
    ui.proxyType->setEnabled(checked);
    ui.proxyHostName->setEnabled(checked);
    ui.proxyPort->setEnabled(checked);
    ui.proxyUser->setEnabled(checked);
    ui.proxyPassword->setEnabled(checked);
}

/*****************************************************************************************
 * FilteredCoreAccountModel
 *****************************************************************************************/

FilteredCoreAccountModel::FilteredCoreAccountModel(CoreAccountModel* model, QObject* parent)
    : QSortFilterProxyModel(parent)
{
    _internalAccount = model->internalAccount();
    setSourceModel(model);
}

bool FilteredCoreAccountModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (Quassel::runMode() == Quassel::Monolithic)
        return true;

    if (!_internalAccount.isValid())
        return true;

    return _internalAccount != sourceModel()->index(source_row, 0, source_parent).data(CoreAccountModel::AccountIdRole).value<AccountId>();
}

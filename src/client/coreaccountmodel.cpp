/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "coreaccountmodel.h"

#include "clientsettings.h"
#include "quassel.h"

CoreAccountModel::CoreAccountModel(QObject *parent)
    : QAbstractListModel(parent),
    _internalAccount(0)
{
}


CoreAccountModel::CoreAccountModel(const CoreAccountModel *other, QObject *parent)
    : QAbstractListModel(parent),
    _internalAccount(0)
{
    update(other);
}


void CoreAccountModel::update(const CoreAccountModel *other)
{
    clear();
    if (other->_accounts.count() > 0) {
        beginInsertRows(QModelIndex(), 0, other->_accounts.count() -1);
        _accounts = other->_accounts;
        endInsertRows();
    }
    _internalAccount = other->internalAccount();
    _removedAccounts = other->_removedAccounts;
}


void CoreAccountModel::load()
{
    clear();
    CoreAccountSettings s;
    foreach(AccountId accId, s.knownAccounts()) {
        QVariantMap map = s.retrieveAccountData(accId);
        CoreAccount acc;
        acc.fromVariantMap(map); // TODO Hook into kwallet/password saving stuff
        insertAccount(acc);
    }
    if (Quassel::runMode() == Quassel::Monolithic && !internalAccount().isValid()) {
        // Make sure we have an internal account in monolithic mode
        CoreAccount intAcc;
        intAcc.setInternal(true);
        intAcc.setAccountName(tr("Internal Core"));
        _internalAccount = createOrUpdateAccount(intAcc);
    }
}


void CoreAccountModel::save()
{
    CoreAccountSettings s;
    foreach(AccountId id, _removedAccounts) {
        s.removeAccount(id);
    }
    _removedAccounts.clear();
    foreach(const CoreAccount &acc, accounts()) {
        QVariantMap map = acc.toVariantMap(false); // TODO Hook into kwallet/password saving stuff
        s.storeAccountData(acc.accountId(), map);
    }
}


void CoreAccountModel::clear()
{
    if (rowCount()) {
        beginRemoveRows(QModelIndex(), 0, rowCount()-1);
        _internalAccount = 0;
        _accounts.clear();
        endRemoveRows();
    }
}


QVariant CoreAccountModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() >= 1)
        return QVariant();

    const CoreAccount &acc = accounts().at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return acc.accountName();
    case AccountIdRole:
        return QVariant::fromValue<AccountId>(acc.accountId());
    case UuidRole:
        return acc.uuid().toString();

    default:
        return QVariant();
    }
}


CoreAccount CoreAccountModel::account(AccountId id) const
{
    int idx = findAccountIdx(id);
    if (idx >= 0)
        return _accounts.value(idx);
    return CoreAccount();
}


CoreAccount CoreAccountModel::account(const QModelIndex &idx) const
{
    if (idx.isValid() && idx.row() < _accounts.count())
        return _accounts.value(idx.row());
    return CoreAccount();
}


QList<CoreAccount> CoreAccountModel::accounts() const
{
    return _accounts;
}


QList<AccountId> CoreAccountModel::accountIds() const
{
    QList<AccountId> list;
    foreach(const CoreAccount &acc, accounts())
    list << acc.accountId();
    return list;
}


bool CoreAccountModel::operator==(const CoreAccountModel &other) const
{
    return _accounts == other._accounts;
}


// TODO with Qt 4.6, use QAbstractItemModel move semantics to properly do this
AccountId CoreAccountModel::createOrUpdateAccount(const CoreAccount &newAcc)
{
    CoreAccount acc = newAcc;

    if (acc.uuid().isNull())
        acc.setUuid(QUuid::createUuid());

    if (!acc.accountId().isValid()) {
        // find free Id
        AccountId newId = 0;
        const QList<AccountId> &ids = accountIds();
        for (int i = 1;; i++) {
            if (!_removedAccounts.contains(i) && !ids.contains(i)) {
                newId = i;
                break;
            }
        }
        acc.setAccountId(newId);
        insertAccount(acc);
    }
    else {
        int idx = findAccountIdx(acc.accountId());
        if (idx >= 0) {
            if (acc.accountName() == accounts().at(idx).accountName()) {
                _accounts[idx] = acc;
                emit dataChanged(index(idx, 0), index(idx, 0));
            }
            else {
                takeAccount(acc.accountId());
                insertAccount(acc);
            }
        }
        else
            insertAccount(acc);
    }
    return acc.accountId();
}


void CoreAccountModel::insertAccount(const CoreAccount &acc)
{
    if (acc.isInternal()) {
        if (internalAccount().isValid()) {
            qWarning() << "Trying to insert a second internal account in CoreAccountModel, ignoring";
            return;
        }
        _internalAccount = acc.accountId();
    }

    // check for Quuid
    int idx = 0;
    while (idx<_accounts.count() && acc.accountName()> _accounts.at(idx).accountName() && !acc.isInternal())
        ++idx;

    beginInsertRows(QModelIndex(), idx, idx);
    _accounts.insert(idx, acc);
    endInsertRows();
}


CoreAccount CoreAccountModel::takeAccount(AccountId accId)
{
    int idx = findAccountIdx(accId);
    if (idx < 0)
        return CoreAccount();

    beginRemoveRows(QModelIndex(), idx, idx);
    CoreAccount acc = _accounts.takeAt(idx);
    endRemoveRows();

    if (acc.isInternal())
        _internalAccount = 0;

    return acc;
}


void CoreAccountModel::removeAccount(AccountId accId)
{
    takeAccount(accId);
    _removedAccounts.insert(accId);
}


QModelIndex CoreAccountModel::accountIndex(AccountId accId) const
{
    for (int i = 0; i < _accounts.count(); i++) {
        if (_accounts.at(i).accountId() == accId)
            return index(i, 0);
    }
    return QModelIndex();
}


int CoreAccountModel::findAccountIdx(AccountId id) const
{
    QModelIndex idx = accountIndex(id);
    return idx.isValid() ? idx.row() : -1;
}

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

#ifndef COREACCOUNTMODEL_H_
#define COREACCOUNTMODEL_H_

#include <QAbstractListModel>
#include <QUuid>

#include "coreaccount.h"

class CoreAccountModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum {
        AccountIdRole = Qt::UserRole,
        UuidRole
    };

    CoreAccountModel(QObject *parent = 0);
    CoreAccountModel(const CoreAccountModel *other, QObject *parent = 0);

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    CoreAccount account(const QModelIndex &) const;
    CoreAccount account(AccountId) const;
    QList<CoreAccount> accounts() const;
    QList<AccountId> accountIds() const;
    QModelIndex accountIndex(AccountId id) const;

    inline AccountId internalAccount() const;

    AccountId createOrUpdateAccount(const CoreAccount &newAccountData = CoreAccount());
    CoreAccount takeAccount(AccountId);
    void removeAccount(AccountId);

    void update(const CoreAccountModel *other);

    bool operator==(const CoreAccountModel &other) const;

public slots:
    void save();
    void load();
    void clear();

protected:
    void insertAccount(const CoreAccount &);
    int findAccountIdx(AccountId) const;

private:
    int listIndex(AccountId);

    QList<CoreAccount> _accounts;
    QSet<AccountId> _removedAccounts;
    AccountId _internalAccount;
};


// Inlines
int CoreAccountModel::rowCount(const QModelIndex &) const
{
    return _accounts.count();
}


AccountId CoreAccountModel::internalAccount() const
{
    return _internalAccount;
}


#endif

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

#pragma once

#include <QHash>
#include <QList>
#include <QUuid>

#include "syncableobject.h"

class Transfer;

class TransferManager : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

    Q_PROPERTY(TransferManager::TransferIdList transferIds READ transferIds WRITE setTransferIds)

public:
    using TransferIdList = QList<QUuid>;

    TransferManager(QObject *parent = nullptr);

    Transfer *transfer(const QUuid &uuid) const;
    TransferIdList transferIds() const;

signals:
    void transferAdded(const QUuid &uuid);
    void transferRemoved(const QUuid &uuid);

protected:
    void addTransfer(Transfer *transfer);
    void removeTransfer(const QUuid &uuid);

protected slots:
    virtual void setTransferIds(const TransferIdList &transferIds) { Q_UNUSED(transferIds) };
    virtual void onCoreTransferAdded(const QUuid &transferId) { Q_UNUSED(transferId) };

private:
    QHash<QUuid, Transfer *> _transfers;
};

QDataStream &operator<<(QDataStream &out, const TransferManager::TransferIdList &transferIds);
QDataStream &operator>>(QDataStream &in, TransferManager::TransferIdList &state);

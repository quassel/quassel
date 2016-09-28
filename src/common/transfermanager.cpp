/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "transfermanager.h"

#include "transfer.h"


INIT_SYNCABLE_OBJECT(TransferManager)

TransferManager::TransferManager(QObject *parent)
    : SyncableObject(parent)
{
    static auto regTypes = []() -> bool {
        qRegisterMetaType<TransferIdList>("TransferManager::TransferIdList");
        qRegisterMetaTypeStreamOperators<TransferIdList>("TransferManager::TransferIdList");
        return true;
    }();
    Q_UNUSED(regTypes);
}

Transfer *TransferManager::transfer(const QUuid &uuid) const
{
    return _transfers.value(uuid, nullptr);
}


TransferManager::TransferIdList TransferManager::transferIds() const
{
    return _transfers.keys();
}


void TransferManager::addTransfer(Transfer *transfer)
{
    QUuid uuid = transfer->uuid();
    if (_transfers.contains(uuid)) {
        qWarning() << "Cannot add the same file transfer twice!";
        transfer->deleteLater();
        return;
    }
    transfer->setParent(this);
    _transfers[uuid] = transfer;

    SYNC_OTHER(onCoreTransferAdded, ARG(uuid));
    emit transferAdded(uuid);
}


void TransferManager::removeTransfer(const QUuid &uuid)
{
    if (!_transfers.contains(uuid)) {
        qWarning() << "Can not find transfer" << uuid << "to remove!";
        return;
    }
    emit transferRemoved(uuid);
    auto transfer = _transfers.take(uuid);
    transfer->deleteLater();
}


QDataStream &operator<<(QDataStream &out, const TransferManager::TransferIdList &transferIds)
{
    out << static_cast<quint32>(transferIds.size());
    for (auto &&id : transferIds) {
        out << id;
    }
    return out;
}


QDataStream &operator>>(QDataStream &in, TransferManager::TransferIdList &transferIds)
{
    quint32 count;
    in >> count;
    transferIds.clear();
    transferIds.reserve(count);
    for (quint32 i = 0; i < count; ++i) {
        QUuid id;
        in >> id;
        transferIds << id;
    }
    return in;
}

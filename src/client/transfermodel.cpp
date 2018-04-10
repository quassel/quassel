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

#include "transfermodel.h"

#include <array>

#include "transfermanager.h"

namespace {
    constexpr int colCount{8};
}


int TransferModel::rowCount(const QModelIndex& index) const
{
    return index.isValid() ? 0 : _transferIds.size();
}


int TransferModel::columnCount(const QModelIndex& index) const
{
    return index.isValid() ? 0 : colCount;
}


QVariant TransferModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static std::array<QString, colCount> headers = {{
        tr("Type"), tr("File"), tr("Status"), tr("Progress"), tr("Transferred"), tr("Speed"), tr("Peer"), tr("Peer Address")
    }};

    if (section < 0 || section >= columnCount() || orientation != Qt::Horizontal)
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return headers[section];

    default:
        return {};
    }
}


QVariant TransferModel::data(const QModelIndex& index, int role) const
{
    if (!_manager)
        return {};
    if (index.column() < 0 || index.column() >= columnCount() || index.row() < 0 || index.row() >= rowCount())
        return {};

    auto t = _manager->transfer(_transferIds.at(index.row()));
    if (!t) {
        qWarning() << "Invalid transfer ID stored in TransferModel!";
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: // Type
            return t->direction() == Transfer::Direction::Send ? tr("Send") : tr("Receive");
        case 1: // File
            return t->fileName();
        case 2: // Status
            return t->prettyStatus();
        case 3: // Progress
            return (t->transferred() / t->fileSize()) * 100;
        case 4: // Transferred
            return t->transferred(); // TODO: use pretty units and show total
        case 5: // Speed
            return "n/a"; // TODO: fixme
        case 6: // Peer
            return t->nick();
        case 7: // Peer Address
            return QString("%1.%2").arg(t->address().toString(), t->port());
        }
        break;

    default:
        return {};
    }

    return {};
}


void TransferModel::setManager(const TransferManager *manager)
{
    if (_manager) {
        disconnect(_manager, 0, this, 0);
        beginResetModel();
        _transferIds.clear();
        endResetModel();
    }

    _manager = manager;
    if (_manager) {
        connect(manager, SIGNAL(transferAdded(QUuid)), SLOT(onTransferAdded(QUuid)));
        connect(manager, SIGNAL(transferRemoved(QUuid)), SLOT(onTransferRemoved(QUuid)));
        for (auto &&transferId : _manager->transferIds()) {
            onTransferAdded(transferId);
        }
    }
}


void TransferModel::onTransferAdded(const QUuid &transferId)
{
    auto transfer = _manager->transfer(transferId);
    if (!transfer) {
        qWarning() << "Invalid transfer ID!";
        return;
    }

    // TODO Qt5: use new connection syntax to make things much less complicated
    connect(transfer, SIGNAL(statusChanged(Transfer::Status)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(directionChanged(Transfer::Direction)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(addressChanged(QHostAddress)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(portChanged(quint16)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(fileNameChanged(QString)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(fileSizeChanged(quint64)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(transferredChanged(quint64)), SLOT(onTransferDataChanged()));
    connect(transfer, SIGNAL(nickChanged(QString)), SLOT(onTransferDataChanged()));

    beginInsertRows({}, rowCount(), rowCount());
    _transferIds.append(transferId);
    endInsertRows();
}


void TransferModel::onTransferRemoved(const QUuid &transferId)
{
    // Check if the transfer object still exists, which means we still should disconnect
    auto transfer = _manager->transfer(transferId);
    if (transfer)
        disconnect(transfer, 0, this, 0);

    for (auto row = 0; row < _transferIds.size(); ++row) {
        if (_transferIds[row] == transferId) {
            beginRemoveRows(QModelIndex(), row, row);
            _transferIds.remove(row);
            endRemoveRows();
            break;
        }
    }
}


void TransferModel::onTransferDataChanged()
{
    auto transfer = qobject_cast<Transfer *>(sender());
    if (!transfer)
        return;

    const auto& transferId = transfer->uuid();
    for (auto row = 0; row < _transferIds.size(); ++row) {
        if (_transferIds[row] == transferId) {
            // TODO Qt5: use proper column
            auto topLeft = createIndex(row, 0);
            auto bottomRight = createIndex(row, columnCount());
            emit dataChanged(topLeft, bottomRight);
            break;
        }
    }
}

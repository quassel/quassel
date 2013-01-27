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

#include "buffermodel.h"

#include <QAbstractItemView>

#include "client.h"
#include "networkmodel.h"
#include "quassel.h"

BufferModel::BufferModel(NetworkModel *parent)
    : QSortFilterProxyModel(parent),
    _selectionModelSynchronizer(this)
{
    setSourceModel(parent);
    if (Quassel::isOptionSet("debugbufferswitches")) {
        connect(_selectionModelSynchronizer.selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(debug_currentChanged(const QModelIndex &, const QModelIndex &)));
    }
    connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(newNetwork(NetworkId)));
    connect(this, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(newBuffers(const QModelIndex &, int, int)));
}


bool BufferModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
    Q_UNUSED(sourceRow);
    // only networks and buffers are allowed
    if (!parent.isValid())
        return true;
    if (parent.data(NetworkModel::ItemTypeRole) == NetworkModel::NetworkItemType)
        return true;

    return false;
}


void BufferModel::newNetwork(NetworkId id)
{
    const Network *net = Client::network(id);
    Q_ASSERT(net);
    connect(net, SIGNAL(connectionStateSet(Network::ConnectionState)),
        this, SLOT(networkConnectionChanged(Network::ConnectionState)));
}


void BufferModel::networkConnectionChanged(Network::ConnectionState state)
{
    switch (state) {
    case Network::Connecting:
    case Network::Initializing:
        if (currentIndex().isValid())
            return;
        {
            Network *net = qobject_cast<Network *>(sender());
            Q_ASSERT(net);
            setCurrentIndex(mapFromSource(Client::networkModel()->networkIndex(net->networkId())));
        }
        break;
    default:
        return;
    }
}


void BufferModel::synchronizeView(QAbstractItemView *view)
{
    _selectionModelSynchronizer.synchronizeSelectionModel(view->selectionModel());
}


void BufferModel::setCurrentIndex(const QModelIndex &newCurrent)
{
    _selectionModelSynchronizer.selectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::Current);
    _selectionModelSynchronizer.selectionModel()->select(newCurrent, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}


void BufferModel::switchToBuffer(const BufferId &bufferId)
{
    QModelIndex source_index = Client::networkModel()->bufferIndex(bufferId);
    setCurrentIndex(mapFromSource(source_index));
}


void BufferModel::switchToBufferIndex(const QModelIndex &bufferIdx)
{
    // we accept indexes that directly belong to us or our parent - nothing else
    if (bufferIdx.model() == this) {
        setCurrentIndex(bufferIdx);
        return;
    }

    if (bufferIdx.model() == sourceModel()) {
        setCurrentIndex(mapFromSource(bufferIdx));
        return;
    }

    qWarning() << "BufferModel::switchToBufferIndex(const QModelIndex &):" << bufferIdx << "does not belong to BufferModel or NetworkModel";
}


void BufferModel::switchToOrJoinBuffer(NetworkId networkId, const QString &name, bool isQuery)
{
    BufferId bufId = Client::networkModel()->bufferId(networkId, name);
    if (bufId.isValid()) {
        QModelIndex targetIdx = Client::networkModel()->bufferIndex(bufId);
        switchToBuffer(bufId);
        if (!targetIdx.data(NetworkModel::ItemActiveRole).toBool()) {
            qDebug() << "switchToOrJoinBuffer failed to switch even though bufId:" << bufId << "is valid.";
            Client::userInput(BufferInfo::fakeStatusBuffer(networkId), QString(isQuery ? "/QUERY %1" : "/JOIN %1").arg(name));
        }
    }
    else {
        _bufferToSwitchTo = qMakePair(networkId, name);
        Client::userInput(BufferInfo::fakeStatusBuffer(networkId), QString(isQuery ? "/QUERY %1" : "/JOIN %1").arg(name));
    }
}


void BufferModel::debug_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);
    qDebug() << "Switched current Buffer: " << current << current.data().toString() << "Buffer:" << current.data(NetworkModel::BufferIdRole).value<BufferId>();
}


void BufferModel::newBuffers(const QModelIndex &parent, int start, int end)
{
    if (parent.data(NetworkModel::ItemTypeRole) != NetworkModel::NetworkItemType)
        return;

    for (int row = start; row <= end; row++) {
        QModelIndex child = parent.child(row, 0);
        newBuffer(child.data(NetworkModel::BufferIdRole).value<BufferId>());
    }
}


void BufferModel::newBuffer(BufferId bufferId)
{
    BufferInfo bufferInfo = Client::networkModel()->bufferInfo(bufferId);
    if (_bufferToSwitchTo.first == bufferInfo.networkId()
        && _bufferToSwitchTo.second == bufferInfo.bufferName()) {
        _bufferToSwitchTo.first = 0;
        _bufferToSwitchTo.second.clear();
        switchToBuffer(bufferId);
    }
}


void BufferModel::switchToBufferAfterCreation(NetworkId network, const QString &name)
{
    _bufferToSwitchTo = qMakePair(network, name);
}

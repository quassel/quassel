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

#ifndef BUFFERMODEL_H
#define BUFFERMODEL_H

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QPair>

#include "network.h"
#include "networkmodel.h"
#include "types.h"
#include "selectionmodelsynchronizer.h"

class QAbstractItemView;

class BufferModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    BufferModel(NetworkModel *parent = 0);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const;

    inline const SelectionModelSynchronizer *selectionModelSynchronizer() const { return &_selectionModelSynchronizer; }
    inline QItemSelectionModel *standardSelectionModel() const { return _selectionModelSynchronizer.selectionModel(); }

    inline void synchronizeSelectionModel(QItemSelectionModel *selectionModel) { _selectionModelSynchronizer.synchronizeSelectionModel(selectionModel); }
    void synchronizeView(QAbstractItemView *view);

    inline QModelIndex currentIndex() { return standardSelectionModel()->currentIndex(); }
    inline BufferId currentBuffer() { return currentIndex().data(NetworkModel::BufferIdRole).value<BufferId>(); }

public slots:
    void setCurrentIndex(const QModelIndex &newCurrent);
    void switchToBuffer(const BufferId &bufferId);
    void switchToBufferIndex(const QModelIndex &bufferIdx);
    void switchToOrJoinBuffer(NetworkId network, const QString &bufferName, bool isQuery = false);
    void switchToOrStartQuery(NetworkId network, const QString &nick)
    {
        switchToOrJoinBuffer(network, nick, true);
    }


    void switchToBufferAfterCreation(NetworkId network, const QString &name);

private slots:
    void debug_currentChanged(QModelIndex current, QModelIndex previous);
    void newNetwork(NetworkId id);
    void networkConnectionChanged(Network::ConnectionState state);
    void newBuffers(const QModelIndex &parent, int start, int end);

private:
    void newBuffer(BufferId bufferId);

    SelectionModelSynchronizer _selectionModelSynchronizer;
    QPair<NetworkId, QString> _bufferToSwitchTo;
};


#endif // BUFFERMODEL_H

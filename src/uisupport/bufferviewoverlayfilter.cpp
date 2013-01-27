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

#include "bufferviewoverlayfilter.h"

#include "bufferviewoverlay.h"
#include "networkmodel.h"
#include "types.h"

BufferViewOverlayFilter::BufferViewOverlayFilter(QAbstractItemModel *model, BufferViewOverlay *overlay)
    : QSortFilterProxyModel(model),
    _overlay(0)
{
    setOverlay(overlay);
    setSourceModel(model);

    setDynamicSortFilter(true);
}


void BufferViewOverlayFilter::setOverlay(BufferViewOverlay *overlay)
{
    if (_overlay == overlay)
        return;

    if (_overlay) {
        disconnect(_overlay, 0, this, 0);
    }

    _overlay = overlay;

    if (!overlay) {
        invalidate();
        return;
    }

    connect(overlay, SIGNAL(destroyed()), this, SLOT(overlayDestroyed()));
    connect(overlay, SIGNAL(hasChanged()), this, SLOT(invalidate()));
    invalidate();
}


void BufferViewOverlayFilter::overlayDestroyed()
{
    setOverlay(0);
}


bool BufferViewOverlayFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!_overlay)
        return false;

    QModelIndex source_bufferIndex = sourceModel()->index(source_row, 0, source_parent);

    if (!source_bufferIndex.isValid()) {
        qWarning() << "filterAcceptsRow has been called with an invalid Child";
        return false;
    }

    NetworkModel::ItemType itemType = (NetworkModel::ItemType)sourceModel()->data(source_bufferIndex, NetworkModel::ItemTypeRole).toInt();

    NetworkId networkId = sourceModel()->data(source_bufferIndex, NetworkModel::NetworkIdRole).value<NetworkId>();
    if (!_overlay->networkIds().contains(networkId) && !_overlay->allNetworks()) {
        return false;
    }
    else if (itemType == NetworkModel::NetworkItemType) {
        // network items don't need further checks.
        return true;
    }

    int activityLevel = sourceModel()->data(source_bufferIndex, NetworkModel::BufferActivityRole).toInt();
    if (_overlay->minimumActivity() > activityLevel)
        return false;

    int bufferType = sourceModel()->data(source_bufferIndex, NetworkModel::BufferTypeRole).toInt();
    if (!(_overlay->allowedBufferTypes() & bufferType))
        return false;

    BufferId bufferId = sourceModel()->data(source_bufferIndex, NetworkModel::BufferIdRole).value<BufferId>();
    Q_ASSERT(bufferId.isValid());

    if (_overlay->bufferIds().contains(bufferId))
        return true;

    if (_overlay->tempRemovedBufferIds().contains(bufferId))
        return activityLevel > BufferInfo::OtherActivity;

    if (_overlay->removedBufferIds().contains(bufferId))
        return false;

    // the buffer is not known to us
    qDebug() << "BufferViewOverlayFilter::filterAcceptsRow()" << bufferId << "is unknown!";
    return false;
}

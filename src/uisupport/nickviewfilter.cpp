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

#include "nickviewfilter.h"

#include "buffersettings.h"
#include "graphicalui.h"
#include "iconloader.h"
#include "networkmodel.h"
#include "uistyle.h"

/******************************************************************************************
 * NickViewFilter
 ******************************************************************************************/
NickViewFilter::NickViewFilter(const BufferId &bufferId, NetworkModel *parent)
    : QSortFilterProxyModel(parent),
    _bufferId(bufferId)
{
    setSourceModel(parent);
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(TreeModel::SortRole);
}


bool NickViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // root node, networkindexes, the bufferindex of the buffer this filter is active for and it's children are accepted
    if (!source_parent.isValid())
        return true;

    QModelIndex source_child = source_parent.child(source_row, 0);
    return (sourceModel()->data(source_child, NetworkModel::BufferIdRole).value<BufferId>() == _bufferId);
}


QVariant NickViewFilter::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::FontRole:
    case Qt::ForegroundRole:
    case Qt::BackgroundRole:
    case Qt::DecorationRole:
        return GraphicalUi::uiStyle()->nickViewItemData(mapToSource(index), role);
    default:
        return QSortFilterProxyModel::data(index, role);
    }
}

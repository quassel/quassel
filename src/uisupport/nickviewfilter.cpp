/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "nickviewfilter.h"
#include "networkmodel.h"
#include "uisettings.h"

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
  loadColors();
}

void NickViewFilter::loadColors() {
  UiSettings s("QtUiStyle/Colors");
  _FgOnlineStatus = s.value("onlineStatusFG", QVariant(QColor(Qt::black))).value<QColor>();
  _FgAwayStatus = s.value("awayStatusFG", QVariant(QColor(Qt::gray))).value<QColor>();
  // FIXME: use the style interface instead of qsettings
}

QVariant NickViewFilter::data(const QModelIndex &index, int role) const {
  if(role == Qt::ForegroundRole)
    return foreground(index);
  else
    return QSortFilterProxyModel::data(index, role);
}

QVariant NickViewFilter::foreground(const QModelIndex &index) const {
  if(!index.data(NetworkModel::ItemActiveRole).toBool())
    return _FgAwayStatus;
  return _FgOnlineStatus;
}


bool NickViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  // root node, networkindexes, the bufferindex of the buffer this filter is active for and it's childs are accepted
  if(!source_parent.isValid())
    return true;

  QModelIndex source_child = source_parent.child(source_row, 0);
  return (sourceModel()->data(source_child, NetworkModel::BufferIdRole).value<BufferId>() == _bufferId);
}

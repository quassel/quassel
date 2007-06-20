/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include "bufferviewfilter.h"

/*****************************************
* The Filter for the Tree View
*****************************************/
BufferViewFilter::BufferViewFilter(QAbstractItemModel *model, Modes filtermode, QStringList nets, QObject *parent) : QSortFilterProxyModel(parent) {
  setSourceModel(model);
  mode = filtermode;
  networks = nets;
  
  connect(model, SIGNAL(invalidateFilter()), this, SLOT(invalidateMe()));
  connect(model, SIGNAL(updateSelection(const QModelIndex &, QItemSelectionModel::SelectionFlags)), this, SLOT(select(const QModelIndex &, QItemSelectionModel::SelectionFlags)));
    
  connect(this, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), model, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
  connect(this, SIGNAL(doubleClicked(const QModelIndex &)), model, SLOT(doubleClickReceived(const QModelIndex &)));
}

void BufferViewFilter::invalidateMe() {
  invalidateFilter();
}

void BufferViewFilter::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  emit updateSelection(mapFromSource(index), command);
}

void BufferViewFilter::changeCurrent(const QModelIndex &current, const QModelIndex &previous) {
  emit currentChanged(mapToSource(current), mapToSource(previous));
}

void BufferViewFilter::doubleClickReceived(const QModelIndex &clicked) {
  emit doubleClicked(mapToSource(clicked));
}

bool BufferViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  QModelIndex child = source_parent.child(source_row, 0);
  if(!child.isValid())
    return true; // can't imagine this case but true sounds good :)

  Buffer::Type bufferType = (Buffer::Type) child.data(BufferTreeModel::BufferTypeRole).toInt();
  if((mode & NoChannels) && bufferType == Buffer::ChannelBuffer) return false;
  if((mode & NoQueries) && bufferType == Buffer::QueryBuffer) return false;
  if((mode & NoServers) && bufferType == Buffer::ServerBuffer) return false;

  bool isActive = child.data(BufferTreeModel::BufferActiveRole).toBool();
  if((mode & NoActive) && isActive) return false;
  if((mode & NoInactive) && !isActive) return false;

  QString net = child.data(Qt::DisplayRole).toString();
  if((mode & SomeNets) && !networks.contains(net)) return false;
    
  return true;
}

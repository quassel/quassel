/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include "bufferviewfilter.h"

/*****************************************
* The Filter for the Tree View
*****************************************/
BufferViewFilter::BufferViewFilter(QAbstractItemModel *model, const Modes &filtermode, const QList<uint> &nets)
  : QSortFilterProxyModel(model),
    mode(filtermode),
    networks(QSet<uint>::fromList(nets))
{
  setSourceModel(model);
  setSortCaseSensitivity(Qt::CaseInsensitive);

  // FIXME
  // ok the following basically sucks. therfore it's commented out. Justice served.
  // a better solution would use dataChanged()
  
  // I have this feeling that this resulted in a fuckup once... no clue though right now and invalidateFilter isn't a slot -.-
  //connect(model, SIGNAL(invalidateFilter()), this, SLOT(invalidate()));
  // connect(model, SIGNAL(invalidateFilter()), this, SLOT(invalidateFilter_()));
}

void BufferViewFilter::invalidateFilter_() {
  QSortFilterProxyModel::invalidateFilter();
}

Qt::ItemFlags BufferViewFilter::flags(const QModelIndex &index) const {
  Qt::ItemFlags flags = mapToSource(index).flags();
  if(mode & FullCustom) {
    if(index == QModelIndex() || index.parent() == QModelIndex())
      flags |= Qt::ItemIsDropEnabled;
  }
  return flags;
}

bool BufferViewFilter::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
  // drops have to occur in the open field
  if(parent != QModelIndex())
    return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);

  if(!BufferTreeModel::mimeContainsBufferList(data))
    return false;

  QList< QPair<uint, uint> > bufferList = BufferTreeModel::mimeDataToBufferList(data);

  uint netId, bufferId;
  for(int i = 0; i < bufferList.count(); i++) {
    netId = bufferList[i].first;
    bufferId = bufferList[i].second;
    if(!networks.contains(netId)) {
      networks << netId;
    }
    addBuffer(bufferId);
  }
  return true;
}

void BufferViewFilter::addBuffer(const uint &bufferuid) {
  if(!buffers.contains(bufferuid)) {
    buffers << bufferuid;
    invalidateFilter();
  }
}

void BufferViewFilter::removeBuffer(const QModelIndex &index) {
  if(!(mode & FullCustom))
    return; // only custom buffers can be customized... obviously... :)
  
  if(index.parent() == QModelIndex())
    return; // only child elements can be deleted

  bool lastBuffer = (rowCount(index.parent()) == 1);
  uint netId = index.data(BufferTreeModel::NetworkIdRole).toUInt();
  uint bufferuid = index.data(BufferTreeModel::BufferUidRole).toUInt();

  if(buffers.contains(bufferuid)) {
    buffers.remove(bufferuid);
    
    if(lastBuffer) {
      networks.remove(netId);
      Q_ASSERT(!networks.contains(netId));
    }

    invalidateFilter();
  }
  
}


bool BufferViewFilter::filterAcceptBuffer(const QModelIndex &source_bufferIndex) const {
  Buffer::Type bufferType = (Buffer::Type) source_bufferIndex.data(BufferTreeModel::BufferTypeRole).toInt();
  
  if((mode & NoChannels) && bufferType == Buffer::ChannelType)
    return false;
  if((mode & NoQueries) && bufferType == Buffer::QueryType)
    return false;
  if((mode & NoServers) && bufferType == Buffer::StatusType)
    return false;

//   bool isActive = source_bufferIndex.data(BufferTreeModel::BufferActiveRole).toBool();
//   if((mode & NoActive) && isActive)
//     return false;
//   if((mode & NoInactive) && !isActive)
//     return false;

  if((mode & FullCustom)) {
    uint bufferuid = source_bufferIndex.data(BufferTreeModel::BufferUidRole).toUInt();
    return buffers.contains(bufferuid);
  }
    
  return true;
}

bool BufferViewFilter::filterAcceptNetwork(const QModelIndex &source_index) const {
  uint net = source_index.data(BufferTreeModel::NetworkIdRole).toUInt();
  return !((mode & (SomeNets | FullCustom)) && !networks.contains(net));
}

bool BufferViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  QModelIndex child = sourceModel()->index(source_row, 0, source_parent);
  
  if(!child.isValid()) {
    qDebug() << "filterAcceptsRow has been called with an invalid Child";
    return false;
  }

  if(source_parent == QModelIndex())
    return filterAcceptNetwork(child);
  else
    return filterAcceptBuffer(child);
}

bool BufferViewFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  int lefttype = left.data(BufferTreeModel::BufferTypeRole).toInt();
  int righttype = right.data(BufferTreeModel::BufferTypeRole).toInt();

  if(lefttype != righttype)
    return lefttype < righttype;
  else
    return QSortFilterProxyModel::lessThan(left, right);
}


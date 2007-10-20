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
BufferViewFilter::BufferViewFilter(QAbstractItemModel *model, const Modes &filtermode, const QStringList &nets) : QSortFilterProxyModel(model) {
  setSourceModel(model);
  setSortRole(BufferTreeModel::BufferNameRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
    
  mode = filtermode;
  networks = nets;
  
  connect(model, SIGNAL(invalidateFilter()), this, SLOT(invalidateMe()));
  connect(model, SIGNAL(selectionChanged(const QModelIndex &)),
          this, SLOT(select(const QModelIndex &)));
  
  connect(this, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          model, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
}

void BufferViewFilter::invalidateMe() {
  invalidateFilter();
}

void BufferViewFilter::select(const QModelIndex &index) {
  emit selectionChanged(mapFromSource(index));
}

void BufferViewFilter::changeCurrent(const QModelIndex &current, const QModelIndex &previous) {
  emit currentChanged(mapToSource(current), mapToSource(previous));
}

void BufferViewFilter::doubleClickReceived(const QModelIndex &clicked) {
  emit doubleClicked(mapToSource(clicked));
}

void BufferViewFilter::dropEvent(QDropEvent *event) {
  const QMimeData *data = event->mimeData();
  if(!(mode & FullCustom))
    return; // only custom buffers can be customized... obviously... :)
  
  if(!(data->hasFormat("application/Quassel/BufferItem/row")
       && data->hasFormat("application/Quassel/BufferItem/network")
       && data->hasFormat("application/Quassel/BufferItem/bufferInfo")))
    return; // whatever the drop is... it's not a buffer...
  
  event->accept();
  uint bufferuid = data->data("application/Quassel/BufferItem/bufferInfo").toUInt();
  QString networkname = QString::fromUtf8("application/Quassel/BufferItem/network");
  
  for(int rowid = 0; rowid < rowCount(); rowid++) {
    QModelIndex networkindex = index(rowid, 0);
    if(networkindex.data(Qt::DisplayRole) == networkname) {
      addBuffer(bufferuid);
      return;
    }
  }
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  addBuffer(bufferuid);
  endInsertRows();
}


void BufferViewFilter::addBuffer(const uint &bufferuid) {
  if(!customBuffers.contains(bufferuid)) {
    customBuffers << bufferuid;
    invalidateFilter();
  }
}

void BufferViewFilter::removeBuffer(const QModelIndex &index) {
  if(!(mode & FullCustom))
    return; // only custom buffers can be customized... obviously... :)
  
  if(index.parent() == QModelIndex())
    return; // only child elements can be deleted
  
  uint bufferuid = index.data(BufferTreeModel::BufferUidRole).toUInt();
  if(customBuffers.contains(bufferuid)) {
    beginRemoveRows(index.parent(), index.row(), index.row());
    customBuffers.removeAt(customBuffers.indexOf(bufferuid));
    endRemoveRows();
    invalidateFilter();
  }
  
}


bool BufferViewFilter::filterAcceptBuffer(const QModelIndex &source_bufferIndex) const {
  Buffer::Type bufferType = (Buffer::Type) source_bufferIndex.data(BufferTreeModel::BufferTypeRole).toInt();
  if((mode & NoChannels) && bufferType == Buffer::ChannelBuffer) return false;
  if((mode & NoQueries) && bufferType == Buffer::QueryBuffer) return false;
  if((mode & NoServers) && bufferType == Buffer::ServerBuffer) return false;

  bool isActive = source_bufferIndex.data(BufferTreeModel::BufferActiveRole).toBool();
  if((mode & NoActive) && isActive) return false;
  if((mode & NoInactive) && !isActive) return false;

  if((mode & FullCustom)) {
    uint bufferuid = source_bufferIndex.data(BufferTreeModel::BufferUidRole).toUInt();
    if(!customBuffers.contains(bufferuid))
      return false;
  }
    
  return true;
}

bool BufferViewFilter::filterAcceptNetwork(const QModelIndex &source_index) const {
  QString net = source_index.data(Qt::DisplayRole).toString();
  if((mode & SomeNets) && !networks.contains(net)) {
    return false;
  } else if(mode & FullCustom) {
    // let's check if we got a child that want's to show off
    int childcount = sourceModel()->rowCount(source_index);
    for(int rowid = 0; rowid < childcount; rowid++) {
      QModelIndex child = sourceModel()->index(rowid, 0, source_index);
      uint bufferuid = child.data(BufferTreeModel::BufferUidRole).toUInt();
      if(customBuffers.contains(bufferuid))
        return true;
    }
    return false;
  } else {
    return true;
  }
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

bool BufferViewFilter::lessThan(const QModelIndex &left, const QModelIndex &right) {
  // pretty interesting stuff here, eh?
  return QSortFilterProxyModel::lessThan(left, right);
}


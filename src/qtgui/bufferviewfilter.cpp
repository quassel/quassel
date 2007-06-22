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
  setSortRole(BufferTreeModel::BufferNameRole);
  setSortCaseSensitivity(Qt::CaseInsensitive);
    
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

void BufferViewFilter::enterDrag() {
  connect(sourceModel(), SIGNAL(addBuffer(const uint &, const QString &)),
          this, SLOT(addBuffer(const uint &, const QString &)));
}

void BufferViewFilter::leaveDrag() {
  disconnect(sourceModel(), SIGNAL(addBuffer(const uint &, const QString &)),
             this, SLOT(addBuffer(const uint &, const QString &)));
}

void BufferViewFilter::addBuffer(const uint &bufferuid, const QString &network) {
  if(!networks.contains(network)) {
    networks << network;
  }
  
  if(!customBuffers.contains(bufferuid)) {
    customBuffers << bufferuid;
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
    uint bufferuid = source_bufferIndex.data(BufferTreeModel::BufferIdRole).toUInt();
    if(!customBuffers.contains(bufferuid))
      return false;
  }
    
  return true;
}

bool BufferViewFilter::filterAcceptNetwork(const QModelIndex &source_index) const {
  QString net = source_index.data(Qt::DisplayRole).toString();
  if((mode & SomeNets) && !networks.contains(net))
    return false;
  else
    return true;
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
  return QSortFilterProxyModel::lessThan(left, right);
}


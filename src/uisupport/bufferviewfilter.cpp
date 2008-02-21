/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include <QColor>

#include "networkmodel.h"

#include "uisettings.h"

/*****************************************
* The Filter for the Tree View
*****************************************/
BufferViewFilter::BufferViewFilter(QAbstractItemModel *model, const Modes &filtermode, const QList<NetworkId> &nets)
  : QSortFilterProxyModel(model),
    mode(filtermode),
    networks(QSet<NetworkId>::fromList(nets))
{
  setSourceModel(model);
  setSortCaseSensitivity(Qt::CaseInsensitive);
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

  if(!NetworkModel::mimeContainsBufferList(data))
    return false;

  QList< QPair<NetworkId, BufferId> > bufferList = NetworkModel::mimeDataToBufferList(data);

  NetworkId netId;
  BufferId bufferId;
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

void BufferViewFilter::addBuffer(const BufferId &bufferuid) {
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
  NetworkId netId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  BufferId bufferuid = index.data(NetworkModel::BufferIdRole).value<BufferId>();

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
  BufferInfo::Type bufferType = (BufferInfo::Type) source_bufferIndex.data(NetworkModel::BufferTypeRole).toInt();
  
  if((mode & NoChannels) && bufferType == BufferInfo::ChannelBuffer)
    return false;
  if((mode & NoQueries) && bufferType == BufferInfo::QueryBuffer)
    return false;
  if((mode & NoServers) && bufferType == BufferInfo::StatusBuffer)
    return false;

//   bool isActive = source_bufferIndex.data(NetworkModel::BufferActiveRole).toBool();
//   if((mode & NoActive) && isActive)
//     return false;
//   if((mode & NoInactive) && !isActive)
//     return false;

  if((mode & FullCustom)) {
    BufferId bufferuid = source_bufferIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
    return buffers.contains(bufferuid);
  }
    
  return true;
}

bool BufferViewFilter::filterAcceptNetwork(const QModelIndex &source_index) const {
  NetworkId net = source_index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
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
  int lefttype = left.data(NetworkModel::BufferTypeRole).toInt();
  int righttype = right.data(NetworkModel::BufferTypeRole).toInt();

  if(lefttype != righttype)
    return lefttype < righttype;
  else
    return QSortFilterProxyModel::lessThan(left, right);
}

QVariant BufferViewFilter::data(const QModelIndex &index, int role) const {
  if(role == Qt::ForegroundRole)
    return foreground(index);
  else
    return QSortFilterProxyModel::data(index, role);
}

QVariant BufferViewFilter::foreground(const QModelIndex &index) const {
  UiSettings s("QtUi/Colors");
  QVariant inactiveActivity = s.value("inactiveActivityFG", QVariant(QColor(Qt::gray)));
  QVariant noActivity = s.value("noActivityFG", QVariant(QColor(Qt::black)));
  QVariant highlightActivity = s.value("highlightActivityFG", QVariant(QColor(Qt::magenta)));
  QVariant newMessageActivity = s.value("newMessageActivityFG", QVariant(QColor(Qt::green)));
  QVariant otherActivity = s.value("otherActivityFG", QVariant(QColor(Qt::darkGreen)));

  if(!index.data(NetworkModel::ItemActiveRole).toBool())
    return inactiveActivity.value<QColor>();

  Buffer::ActivityLevel activity = (Buffer::ActivityLevel)index.data(NetworkModel::BufferActivityRole).toInt();

  if(activity & Buffer::Highlight)
    return highlightActivity.value<QColor>();
  if(activity & Buffer::NewMessage)
    return newMessageActivity.value<QColor>();
  if(activity & Buffer::OtherActivity)
    return otherActivity.value<QColor>();
  
  return noActivity.value<QColor>();
  
  // FIXME:: make colors configurable;

}

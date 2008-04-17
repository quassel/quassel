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

#include "buffermodel.h"
#include "client.h"
#include "networkmodel.h"

#include "uisettings.h"

/*****************************************
* The Filter for the Tree View
*****************************************/
BufferViewFilter::BufferViewFilter(QAbstractItemModel *model, BufferViewConfig *config)
  : QSortFilterProxyModel(model),
    _config(0)
{
  setConfig(config);
  setSourceModel(model);
  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(source_rowsInserted(const QModelIndex &, int, int)));
			
  setDynamicSortFilter(true);
}

void BufferViewFilter::setConfig(BufferViewConfig *config) {
  if(_config == config)
    return;
  
  if(_config) {
    disconnect(_config, 0, this, 0);
  }

  _config = config;
  if(config) {
    connect(config, SIGNAL(bufferViewNameSet(const QString &)), this, SLOT(invalidate()));
    connect(config, SIGNAL(networkIdSet(const NetworkId &)), this, SLOT(invalidate()));
    connect(config, SIGNAL(addNewBuffersAutomaticallySet(bool)), this, SLOT(invalidate()));
    connect(config, SIGNAL(sortAlphabeticallySet(bool)), this, SLOT(invalidate()));
    connect(config, SIGNAL(hideInactiveBuffersSet(bool)), this, SLOT(invalidate()));
    connect(config, SIGNAL(allowedBufferTypesSet(int)), this, SLOT(invalidate()));
    connect(config, SIGNAL(minimumActivitySet(int)), this, SLOT(invalidate()));
    connect(config, SIGNAL(bufferListSet()), this, SLOT(invalidate()));
    connect(config, SIGNAL(bufferAdded(const BufferId &, int)), this, SLOT(invalidate()));
    connect(config, SIGNAL(bufferMoved(const BufferId &, int)), this, SLOT(invalidate()));
    connect(config, SIGNAL(bufferRemoved(const BufferId &)), this, SLOT(invalidate()));
  }
  invalidate();
}

Qt::ItemFlags BufferViewFilter::flags(const QModelIndex &index) const {
  Qt::ItemFlags flags = mapToSource(index).flags();
  if(_config && index == QModelIndex() || index.parent() == QModelIndex())
    flags |= Qt::ItemIsDropEnabled;
  return flags;
}

bool BufferViewFilter::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
  if(!config() || !NetworkModel::mimeContainsBufferList(data))
    return QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);

  NetworkId droppedNetworkId;
  if(parent.data(NetworkModel::ItemTypeRole) == NetworkModel::NetworkItemType)
    droppedNetworkId = parent.data(NetworkModel::NetworkIdRole).value<NetworkId>();

  QList< QPair<NetworkId, BufferId> > bufferList = NetworkModel::mimeDataToBufferList(data);
  BufferId bufferId;
  NetworkId networkId;
  int pos;
  for(int i = 0; i < bufferList.count(); i++) {
    networkId = bufferList[i].first;
    bufferId = bufferList[i].second;
    if(droppedNetworkId == networkId) {
      if(row < 0)
	row = 0;
      if(row < rowCount(parent)) {
	BufferId beforeBufferId = parent.child(row, 0).data(NetworkModel::BufferIdRole).value<BufferId>();
	pos = config()->bufferList().indexOf(beforeBufferId);
      } else {
	pos = config()->bufferList().count();
      }

      if(config()->bufferList().contains(bufferId)) {
	if(config()->bufferList().indexOf(bufferId) < pos)
	  pos--;
	config()->requestMoveBuffer(bufferId, pos);
      } else {
	config()->requestAddBuffer(bufferId, pos);
      }

    } else {
      addBuffer(bufferId);
    }
  }
  return true;
}

void BufferViewFilter::addBuffer(const BufferId &bufferId) {
  if(!config() || config()->bufferList().contains(bufferId))
    return;
  
  int pos = config()->bufferList().count();
  bool lt;
  for(int i = 0; i < config()->bufferList().count(); i++) {
    if(config() && config()->sortAlphabetically())
      lt = bufferIdLessThan(bufferId, config()->bufferList()[i]);
    else
      lt = bufferId < config()->bufferList()[i];
    
    if(lt) {
      pos = i;
      break;
    }
  }
  config()->requestAddBuffer(bufferId, pos);
}

void BufferViewFilter::removeBuffer(const QModelIndex &index) {
  if(!config())
    return;
  
  BufferId bufferId = data(index, NetworkModel::BufferIdRole).value<BufferId>();
  config()->requestRemoveBuffer(bufferId);
}


bool BufferViewFilter::filterAcceptBuffer(const QModelIndex &source_bufferIndex) const {
  BufferId bufferId = sourceModel()->data(source_bufferIndex, NetworkModel::BufferIdRole).value<BufferId>();
  Q_ASSERT(bufferId.isValid());
  if(!_config)
    return true;
  
  if(config()->networkId().isValid() && config()->networkId() != sourceModel()->data(source_bufferIndex, NetworkModel::NetworkIdRole).value<NetworkId>())
    return false;

  if(!(_config->allowedBufferTypes() & (BufferInfo::Type)source_bufferIndex.data(NetworkModel::BufferTypeRole).toInt()))
    return false;

  if(_config->hideInactiveBuffers() && !source_bufferIndex.data(NetworkModel::ItemActiveRole).toBool())
    return false;

  if(_config->minimumActivity() > source_bufferIndex.data(NetworkModel::BufferActivityRole).toInt()) {
    if(bufferId != Client::bufferModel()->standardSelectionModel()->currentIndex().data(NetworkModel::BufferIdRole).value<BufferId>())
      return false;
  }

  return _config->bufferList().contains(bufferId);
}

bool BufferViewFilter::filterAcceptNetwork(const QModelIndex &source_index) const {
  if(!config())
    return true;

  if(!config()->networkId().isValid()) {
    return true;
  } else {
    return config()->networkId() == sourceModel()->data(source_index, NetworkModel::NetworkIdRole).value<NetworkId>();
  }
}

bool BufferViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  QModelIndex child = sourceModel()->index(source_row, 0, source_parent);
  
  if(!child.isValid()) {
    qWarning() << "filterAcceptsRow has been called with an invalid Child";
    return false;
  }

  if(!source_parent.isValid())
    return filterAcceptNetwork(child);
  else
    return filterAcceptBuffer(child);
}

bool BufferViewFilter::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
  int itemType = sourceModel()->data(source_left, NetworkModel::ItemTypeRole).toInt();
  switch(itemType) {
  case NetworkModel::NetworkItemType:
    return networkLessThan(source_left, source_right);
  case NetworkModel::BufferItemType:
    return bufferLessThan(source_left, source_right);
  default:
    return QSortFilterProxyModel::lessThan(source_left, source_right);    
  }
}

bool BufferViewFilter::bufferLessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
  return QSortFilterProxyModel::lessThan(source_left, source_right);
  BufferId leftBufferId = sourceModel()->data(source_left, NetworkModel::BufferIdRole).value<BufferId>();
  BufferId rightBufferId = sourceModel()->data(source_right, NetworkModel::BufferIdRole).value<BufferId>();
  if(config()) {
    return config()->bufferList().indexOf(leftBufferId) < config()->bufferList().indexOf(rightBufferId);
  } else
    return bufferIdLessThan(leftBufferId, rightBufferId);
}

bool BufferViewFilter::networkLessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
  NetworkId leftNetworkId = sourceModel()->data(source_left, NetworkModel::NetworkIdRole).value<NetworkId>();
  NetworkId rightNetworkId = sourceModel()->data(source_right, NetworkModel::NetworkIdRole).value<NetworkId>();

  if(config() && config()->sortAlphabetically())
    return QSortFilterProxyModel::lessThan(source_left, source_right);
  else
    return leftNetworkId < rightNetworkId;
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
    return inactiveActivity;

  Buffer::ActivityLevel activity = (Buffer::ActivityLevel)index.data(NetworkModel::BufferActivityRole).toInt();

  if(activity & Buffer::Highlight)
    return highlightActivity;
  if(activity & Buffer::NewMessage)
    return newMessageActivity;
  if(activity & Buffer::OtherActivity)
    return otherActivity;

  return noActivity;

}

void BufferViewFilter::source_rowsInserted(const QModelIndex &parent, int start, int end) {
  if(parent.data(NetworkModel::ItemTypeRole) != NetworkModel::BufferItemType)
    return;

  if(!config() || !config()->addNewBuffersAutomatically())
    return;

  QModelIndex child;
  for(int row = start; row <= end; row++) {
    child = sourceModel()->index(row, 0, parent);
    addBuffer(sourceModel()->data(child, NetworkModel::BufferIdRole).value<BufferId>());
  }
}

// ******************************
//  Helper
// ******************************
bool bufferIdLessThan(const BufferId &left, const BufferId &right) {
  Q_CHECK_PTR(Client::networkModel());
  if(!Client::networkModel())
    return true;
  
  QModelIndex leftIndex = Client::networkModel()->bufferIndex(left);
  QModelIndex rightIndex = Client::networkModel()->bufferIndex(right);

  int leftType = Client::networkModel()->data(leftIndex, NetworkModel::BufferTypeRole).toInt();
  int rightType = Client::networkModel()->data(rightIndex, NetworkModel::BufferTypeRole).toInt();

  if(leftType != rightType)
    return leftType < rightType;
  else
    return QString::compare(Client::networkModel()->data(leftIndex, Qt::DisplayRole).toString(), Client::networkModel()->data(rightIndex, Qt::DisplayRole).toString(), Qt::CaseInsensitive) < 0;
}


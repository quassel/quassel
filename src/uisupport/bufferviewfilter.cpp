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

#include <QApplication>
#include <QPalette>
#include <QBrush>

#include "bufferinfo.h"
#include "buffermodel.h"
#include "buffersettings.h"
#include "client.h"
#include "iconloader.h"
#include "networkmodel.h"

#include "uisettings.h"

class CheckRemovalEvent : public QEvent {
public:
  CheckRemovalEvent(const QModelIndex &source_index) : QEvent(QEvent::User), index(source_index) {};
  QPersistentModelIndex index;
};

/*****************************************
* The Filter for the Tree View
*****************************************/
BufferViewFilter::BufferViewFilter(QAbstractItemModel *model, BufferViewConfig *config)
  : QSortFilterProxyModel(model),
    _config(0),
    _sortOrder(Qt::AscendingOrder),
    _userOfflineIcon(SmallIcon("user-offline")),
    _userAwayIcon(SmallIcon("user-away")),
    _userOnlineIcon(SmallIcon("user-online")),
    _editMode(false),
    _enableEditMode(tr("Edit Mode"), this)
{
  setConfig(config);
  setSourceModel(model);

  setDynamicSortFilter(true);

  loadColors();

  connect(this, SIGNAL(_dataChanged(const QModelIndex &, const QModelIndex &)),
	  this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

  _enableEditMode.setCheckable(true);
  _enableEditMode.setChecked(_editMode);
  connect(&_enableEditMode, SIGNAL(toggled(bool)), this, SLOT(enableEditMode(bool)));

  BufferSettings bufferSettings;
  _showUserStateIcons = bufferSettings.showUserStateIcons();
  bufferSettings.notify("ShowUserStateIcons", this, SLOT(showUserStateIconsChanged()));
}

void BufferViewFilter::loadColors() {
  UiSettings s("QtUiStyle/Colors");
  _FgColorInactiveActivity = s.value("inactiveActivityFG", QVariant(QColor(Qt::gray))).value<QColor>();
  _FgColorNoActivity = s.value("noActivityFG", QVariant(QColor(Qt::black))).value<QColor>();
  _FgColorHighlightActivity = s.value("highlightActivityFG", QVariant(QColor(Qt::magenta))).value<QColor>();
  _FgColorNewMessageActivity = s.value("newMessageActivityFG", QVariant(QColor(Qt::green))).value<QColor>();
  _FgColorOtherActivity = s.value("otherActivityFG", QVariant(QColor(Qt::darkGreen))).value<QColor>();
}

void BufferViewFilter::showUserStateIconsChanged() {
  BufferSettings bufferSettings;
  _showUserStateIcons = bufferSettings.showUserStateIcons();
}

void BufferViewFilter::setConfig(BufferViewConfig *config) {
  if(_config == config)
    return;

  if(_config) {
    disconnect(_config, 0, this, 0);
  }

  _config = config;

  if(!config) {
    invalidate();
    return;
  }

  if(config->isInitialized()) {
    configInitialized();
  } else {
    connect(config, SIGNAL(initDone()), this, SLOT(configInitialized()));
    invalidate();
  }
}

void BufferViewFilter::configInitialized() {
  if(!config())
    return;

  connect(config(), SIGNAL(bufferViewNameSet(const QString &)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(networkIdSet(const NetworkId &)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(addNewBuffersAutomaticallySet(bool)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(sortAlphabeticallySet(bool)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(hideInactiveBuffersSet(bool)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(allowedBufferTypesSet(int)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(minimumActivitySet(int)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(bufferListSet()), this, SLOT(invalidate()));
  connect(config(), SIGNAL(bufferAdded(const BufferId &, int)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(bufferMoved(const BufferId &, int)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(bufferRemoved(const BufferId &)), this, SLOT(invalidate()));
  connect(config(), SIGNAL(bufferPermanentlyRemoved(const BufferId &)), this, SLOT(invalidate()));

  disconnect(config(), SIGNAL(initDone()), this, SLOT(configInitialized()));

  invalidate();
  emit configChanged();
}

QList<QAction *> BufferViewFilter::actions(const QModelIndex &index) {
  Q_UNUSED(index)
  QList<QAction *> actionList;
  actionList << &_enableEditMode;
  return actionList;
}

void BufferViewFilter::enableEditMode(bool enable) {
  if(_editMode == enable) {
    return;
  }
  _editMode = enable;

  if(!config())
    return;

  if(enable == false) {
    int numBuffers = config()->bufferList().count();
    QSet<BufferId>::const_iterator iter;
    for(iter = _toAdd.constBegin(); iter != _toAdd.constEnd(); iter++) {
      if(config()->bufferList().contains(*iter))
	continue;
      config()->requestAddBuffer(*iter, numBuffers);
    }
    for(iter = _toTempRemove.constBegin(); iter != _toTempRemove.constEnd(); iter++) {
      if(config()->temporarilyRemovedBuffers().contains(*iter))
	 continue;
      config()->requestRemoveBuffer(*iter);
    }
    for(iter = _toRemove.constBegin(); iter != _toRemove.constEnd(); iter++) {
      if(config()->removedBuffers().contains(*iter))
	 continue;
      config()->requestRemoveBufferPermanently(*iter);
    }
  }
  _toAdd.clear();
  _toTempRemove.clear();
  _toRemove.clear();

  invalidate();
}


Qt::ItemFlags BufferViewFilter::flags(const QModelIndex &index) const {
  Qt::ItemFlags flags = mapToSource(index).flags();
  if(_config) {
    if(index == QModelIndex() || index.parent() == QModelIndex()) {
      flags |= Qt::ItemIsDropEnabled;
    } else if(_editMode) {
      flags |= Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
    }
  }
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
	if(_sortOrder == Qt::DescendingOrder)
	  pos++;
      } else {
	if(_sortOrder == Qt::AscendingOrder)
	  pos = config()->bufferList().count();
	else
	  pos = 0;
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

void BufferViewFilter::sort(int column, Qt::SortOrder order) {
  _sortOrder = order;
  QSortFilterProxyModel::sort(column, order);
}

void BufferViewFilter::addBuffer(const BufferId &bufferId) const {
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

bool BufferViewFilter::filterAcceptBuffer(const QModelIndex &source_bufferIndex) const {
  // no config -> "all buffers" -> accept everything
  if(!config())
    return true;

  BufferId bufferId = source_bufferIndex.data(NetworkModel::BufferIdRole).value<BufferId>();
  Q_ASSERT(bufferId.isValid());

  int activityLevel = source_bufferIndex.data(NetworkModel::BufferActivityRole).toInt();

  if(!config()->bufferList().contains(bufferId) && !_editMode) {
    // add the buffer if...
    if(config()->isInitialized() && !config()->removedBuffers().contains(bufferId) // it hasn't been manually removed and either
       && ((config()->addNewBuffersAutomatically() && !config()->temporarilyRemovedBuffers().contains(bufferId)) // is totally unknown to us (a new buffer)...
	   || (config()->temporarilyRemovedBuffers().contains(bufferId) && activityLevel > BufferInfo::OtherActivity))) { // or was just temporarily hidden and has a new message waiting for us.
      addBuffer(bufferId);
    }
    // note: adding the buffer to the valid list does not temper with the following filters ("show only channels" and stuff)
    return false;
  }

  if(config()->networkId().isValid() && config()->networkId() != source_bufferIndex.data(NetworkModel::NetworkIdRole).value<NetworkId>())
    return false;

  if(!(config()->allowedBufferTypes() & (BufferInfo::Type)source_bufferIndex.data(NetworkModel::BufferTypeRole).toInt()))
    return false;

  // the following dynamic filters may not trigger if the buffer is currently selected.
  if(bufferId == Client::bufferModel()->standardSelectionModel()->currentIndex().data(NetworkModel::BufferIdRole).value<BufferId>())
    return true;

  if(config()->hideInactiveBuffers() && !source_bufferIndex.data(NetworkModel::ItemActiveRole).toBool() && activityLevel <= BufferInfo::OtherActivity)
    return false;

  if(config()->minimumActivity() > activityLevel)
    return false;

  return true;
}

bool BufferViewFilter::filterAcceptNetwork(const QModelIndex &source_index) const {
  if(!config())
    return true;

  if(!config()->networkId().isValid()) {
    return true;
  } else {
    return config()->networkId() == source_index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
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
  int leftItemType = source_left.data(NetworkModel::ItemTypeRole).toInt();
  int rightItemType = source_right.data(NetworkModel::ItemTypeRole).toInt();
  int itemType = leftItemType & rightItemType;
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
  BufferId leftBufferId = source_left.data(NetworkModel::BufferIdRole).value<BufferId>();
  BufferId rightBufferId = source_right.data(NetworkModel::BufferIdRole).value<BufferId>();
  if(config()) {
    int leftPos = config()->bufferList().indexOf(leftBufferId);
    int rightPos = config()->bufferList().indexOf(rightBufferId);
    if(leftPos == -1 && rightPos == -1)
      return QSortFilterProxyModel::lessThan(source_left, source_right);
    if(leftPos == -1 || rightPos == -1)
      return !(leftPos < rightPos);
    return leftPos < rightPos;
  } else
    return bufferIdLessThan(leftBufferId, rightBufferId);
}

bool BufferViewFilter::networkLessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
  NetworkId leftNetworkId = source_left.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  NetworkId rightNetworkId = source_right.data(NetworkModel::NetworkIdRole).value<NetworkId>();

  if(config() && config()->sortAlphabetically())
    return QSortFilterProxyModel::lessThan(source_left, source_right);
  else
    return leftNetworkId < rightNetworkId;
}

QVariant BufferViewFilter::data(const QModelIndex &index, int role) const {
  switch(role) {
  case Qt::DecorationRole:
    return icon(index);
  case Qt::ForegroundRole:
    return foreground(index);
  case Qt::CheckStateRole:
    return checkedState(index);
  default:
    return QSortFilterProxyModel::data(index, role);
  }
}

QVariant BufferViewFilter::icon(const QModelIndex &index) const {
  if(!_showUserStateIcons || (config() && config()->disableDecoration()))
    return QVariant();

  if(index.column() != 0)
    return QVariant();

  if(index.data(NetworkModel::BufferTypeRole).toInt() != BufferInfo::QueryBuffer)
    return QVariant();

  if(!index.data(NetworkModel::ItemActiveRole).toBool())
    return _userOfflineIcon;

  if(index.data(NetworkModel::UserAwayRole).toBool())
    return _userAwayIcon;
  else
    return _userOnlineIcon;

  return QVariant();
}

QVariant BufferViewFilter::foreground(const QModelIndex &index) const {
  if(config() && config()->disableDecoration())
    return _FgColorNoActivity;

  BufferInfo::ActivityLevel activity = (BufferInfo::ActivityLevel)index.data(NetworkModel::BufferActivityRole).toInt();

  if(activity & BufferInfo::Highlight)
    return _FgColorHighlightActivity;
  if(activity & BufferInfo::NewMessage)
    return _FgColorNewMessageActivity;
  if(activity & BufferInfo::OtherActivity)
    return _FgColorOtherActivity;

  if(!index.data(NetworkModel::ItemActiveRole).toBool() || index.data(NetworkModel::UserAwayRole).toBool())
    return _FgColorInactiveActivity;

  return _FgColorNoActivity;
}

QVariant BufferViewFilter::checkedState(const QModelIndex &index) const {
  if(!_editMode || !config())
    return QVariant();

  BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
  if(_toAdd.contains(bufferId))
    return Qt::Checked;

  if(_toTempRemove.contains(bufferId))
    return Qt::PartiallyChecked;

  if(_toRemove.contains(bufferId))
    return Qt::Unchecked;

  if(config()->bufferList().contains(bufferId))
    return Qt::Checked;

  if(config()->temporarilyRemovedBuffers().contains(bufferId))
    return Qt::PartiallyChecked;

  return Qt::Unchecked;
}

bool BufferViewFilter::setData(const QModelIndex &index, const QVariant &value, int role) {
  switch(role) {
  case Qt::CheckStateRole:
    return setCheckedState(index, Qt::CheckState(value.toInt()));
  default:
    return QSortFilterProxyModel::setData(index, value, role);
  }
}

bool BufferViewFilter::setCheckedState(const QModelIndex &index, Qt::CheckState state) {
  BufferId bufferId = index.data(NetworkModel::BufferIdRole).value<BufferId>();
  if(!bufferId.isValid())
    return false;

  switch(state) {
  case Qt::Unchecked:
    _toAdd.remove(bufferId);
    _toTempRemove.remove(bufferId);
    _toRemove << bufferId;
    break;
  case Qt::PartiallyChecked:
    _toAdd.remove(bufferId);
    _toTempRemove << bufferId;
    _toRemove.remove(bufferId);
    break;
  case Qt::Checked:
    _toAdd << bufferId;
    _toTempRemove.remove(bufferId);
    _toRemove.remove(bufferId);
    break;
  default:
    return false;
  }
  emit dataChanged(index, index);
  return true;
}

void BufferViewFilter::checkPreviousCurrentForRemoval(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(current);
  if(previous.isValid())
    QCoreApplication::postEvent(this, new CheckRemovalEvent(previous));
}

void BufferViewFilter::customEvent(QEvent *event) {
  if(event->type() != QEvent::User)
    return;

  CheckRemovalEvent *removalEvent = static_cast<CheckRemovalEvent *>(event);
  checkItemForRemoval(removalEvent->index);

  event->accept();
}

void BufferViewFilter::checkItemsForRemoval(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QModelIndex source_topLeft = mapToSource(topLeft);
  QModelIndex source_bottomRight = mapToSource(bottomRight);
  emit _dataChanged(source_topLeft, source_bottomRight);
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

  int leftType = leftIndex.data(NetworkModel::BufferTypeRole).toInt();
  int rightType = rightIndex.data(NetworkModel::BufferTypeRole).toInt();

  if(leftType != rightType)
    return leftType < rightType;
  else
    return QString::compare(leftIndex.data(Qt::DisplayRole).toString(), rightIndex.data(Qt::DisplayRole).toString(), Qt::CaseInsensitive) < 0;
}


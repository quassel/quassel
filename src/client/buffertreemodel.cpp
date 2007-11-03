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

#include <QColor>  // FIXME Dependency on QtGui!

#include "buffertreemodel.h"

#include "mappedselectionmodel.h"
#include <QAbstractItemView>

#include "bufferinfo.h"
#include "client.h"
#include "signalproxy.h"

/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferTreeItem::BufferTreeItem(Buffer *buffer, TreeItem *parent)
  : TreeItem(parent),
    buf(buffer),
    activity(Buffer::NoActivity)
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  if(buf->bufferType() == Buffer::QueryType)
    flags |= Qt::ItemIsDropEnabled;
  setFlags(flags);
}

uint BufferTreeItem::id() const {
  return buf->bufferInfo().uid();
}

void BufferTreeItem::setActivity(const Buffer::ActivityLevel &level) {
  activity = level;
}

QString BufferTreeItem::text(int column) const {
  switch(column) {
    case 0:
      return buf->name();
    case 1:
      return buf->networkName();
    default:
      return QString();
  }
}

QColor BufferTreeItem::foreground(int column) const {
  Q_UNUSED(column)
  // for the time beeing we ignore the column :)
  if(activity & Buffer::Highlight) {
    return QColor(Qt::red);
  } else if(activity & Buffer::NewMessage) {
    return QColor(Qt::darkYellow);
  } else if(activity & Buffer::OtherActivity) {
    return QColor(Qt::darkGreen);
  } else {
    if(buf->isActive())
      return QColor(Qt::black);
    else
      return QColor(Qt::gray);
  }
}


QVariant BufferTreeItem::data(int column, int role) const {
  switch(role) {
  case Qt::DisplayRole:
    return text(column);
  case Qt::ForegroundRole:
    return foreground(column);
  case BufferTreeModel::BufferTypeRole:
    return int(buf->bufferType());
  case BufferTreeModel::BufferActiveRole:
    return buf->isActive();
  case BufferTreeModel::BufferUidRole:
    return buf->bufferInfo().uid();
  case BufferTreeModel::NetworkIdRole:
    return buf->bufferInfo().networkId();
    
  default:
    return TreeItem::data(column, role);
  }
}

/*****************************************
*  Network Items
*****************************************/
NetworkTreeItem::NetworkTreeItem(const uint &netid, const QString &network, TreeItem *parent)
  : TreeItem(parent),
    _networkId(netid),
    net(network)
{
  net = network;
  itemData << net << "";
  setFlags(Qt::ItemIsEnabled);
}

QVariant NetworkTreeItem::data(int column, int role) const {
  switch(role) {
  case BufferTreeModel::NetworkIdRole:
    return _networkId;
  default:
    return TreeItem::data(column, role);
  }
}

uint NetworkTreeItem::id() const {
  return _networkId;
}

/*****************************************
 * BufferTreeModel
 *****************************************/
BufferTreeModel::BufferTreeModel(QObject *parent)
  : TreeModel(BufferTreeModel::defaultHeader(), parent),
    _selectionModelSynchronizer(new SelectionModelSynchronizer(this)),
    _propertyMapper(new ModelPropertyMapper(this))
{
  // initialize the Property Mapper
  _propertyMapper->setModel(this);
  delete _propertyMapper->selectionModel();
  MappedSelectionModel *mappedSelectionModel = new MappedSelectionModel(this);
  _propertyMapper->setSelectionModel(mappedSelectionModel);
  synchronizeSelectionModel(mappedSelectionModel);
  
  connect(_selectionModelSynchronizer, SIGNAL(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)),
	  this, SLOT(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)));
}

QList<QVariant >BufferTreeModel::defaultHeader() {
  QList<QVariant> data;
  data << tr("Buffer") << tr("Network");
  return data;
}

void BufferTreeModel::synchronizeSelectionModel(MappedSelectionModel *selectionModel) {
  selectionModelSynchronizer()->addSelectionModel(selectionModel);
}

void BufferTreeModel::synchronizeView(QAbstractItemView *view) {
  MappedSelectionModel *mappedSelectionModel = new MappedSelectionModel(view->model());
  selectionModelSynchronizer()->addSelectionModel(mappedSelectionModel);
  Q_ASSERT(mappedSelectionModel);
  delete view->selectionModel();
  view->setSelectionModel(mappedSelectionModel);
}

void BufferTreeModel::mapProperty(int column, int role, QObject *target, const QByteArray &property) {
  propertyMapper()->addMapping(column, role, target, property);
}

bool BufferTreeModel::isBufferIndex(const QModelIndex &index) const {
  // not so purdy...
  return parent(index) != QModelIndex();
}

Buffer *BufferTreeModel::getBufferByIndex(const QModelIndex &index) const {
  BufferTreeItem *item = static_cast<BufferTreeItem *>(index.internalPointer());
  return item->buffer();
}

QModelIndex BufferTreeModel::getOrCreateNetworkItemIndex(Buffer *buffer) {
  QString net = buffer->networkName();
  uint netId = buffer->networkId();
  TreeItem *networkItem;

  if(!(networkItem = rootItem->childById(netId))) {
    int nextRow = rootItem->childCount();
    networkItem = new NetworkTreeItem(netId, net, rootItem);
    
    beginInsertRows(QModelIndex(), nextRow, nextRow);
    rootItem->appendChild(networkItem);
    endInsertRows();
  }

  Q_ASSERT(networkItem);
  return index(networkItem->row(), 0);
}

QModelIndex BufferTreeModel::getOrCreateBufferItemIndex(Buffer *buffer) {
  QModelIndex networkItemIndex = getOrCreateNetworkItemIndex(buffer);
  NetworkTreeItem *networkItem = static_cast<NetworkTreeItem*>(networkItemIndex.internalPointer());
  TreeItem *bufferItem;
  
  if(!(bufferItem = networkItem->childById(buffer->bufferInfo().uid()))) {
    int nextRow = networkItem->childCount();
    bufferItem = new BufferTreeItem(buffer, networkItem);
    
    beginInsertRows(networkItemIndex, nextRow, nextRow);
    networkItem->appendChild(bufferItem);
    endInsertRows();
  }

  Q_ASSERT(bufferItem);
  return index(bufferItem->row(), 0, networkItemIndex);
}

QStringList BufferTreeModel::mimeTypes() const {
  // mimetypes we accept for drops
  QStringList types;
  // comma separated list of colon separated pairs of networkid and bufferid
  // example: 0:1,0:2,1:4
  types << "application/Quassel/BufferItemList";
  return types;
}

bool BufferTreeModel::mimeContainsBufferList(const QMimeData *mimeData) {
  return mimeData->hasFormat("application/Quassel/BufferItemList");
}

QList< QPair<uint, uint> > BufferTreeModel::mimeDataToBufferList(const QMimeData *mimeData) {
  QList< QPair<uint, uint> > bufferList;

  if(!mimeContainsBufferList(mimeData))
    return bufferList;

  QStringList rawBufferList = QString::fromAscii(mimeData->data("application/Quassel/BufferItemList")).split(",");
  uint networkId, bufferUid;
  foreach(QString rawBuffer, rawBufferList) {
    if(!rawBuffer.contains(":"))
      continue;
    networkId = rawBuffer.section(":", 0, 0).toUInt();
    bufferUid = rawBuffer.section(":", 1, 1).toUInt();
    bufferList.append(qMakePair(networkId, bufferUid));
  }
  return bufferList;
}


QMimeData *BufferTreeModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *mimeData = new QMimeData();

  QStringList bufferlist;
  QString netid, uid, bufferid;
  foreach(QModelIndex index, indexes) {
    netid = QString::number(index.data(NetworkIdRole).toUInt());
    uid = QString::number(index.data(BufferUidRole).toUInt());
    bufferid = QString("%1:%2").arg(netid).arg(uid);
    if(!bufferlist.contains(bufferid))
      bufferlist << bufferid;
  }

  mimeData->setData("application/Quassel/BufferItemList", bufferlist.join(",").toAscii());

  return mimeData;
}

bool BufferTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
  Q_UNUSED(action)
  Q_UNUSED(row)
  Q_UNUSED(column)

  if(!mimeContainsBufferList(data))
    return false;

  // target must be a query
  Buffer::Type targetType = (Buffer::Type)parent.data(BufferTreeModel::BufferTypeRole).toInt();
  if(targetType != Buffer::QueryType)
    return false;

  QList< QPair<uint, uint> > bufferList = mimeDataToBufferList(data);

  // exactly one buffer has to be dropped
  if(bufferList.count() != 1)
    return false;

  uint netId = bufferList.first().first;
  uint bufferId = bufferList.first().second;

  // no self merges (would kill us)
  if(bufferId == parent.data(BufferUidRole).toUInt())
    return false; 
  
  Q_ASSERT(rootItem->childById(netId));
  Q_ASSERT(rootItem->childById(netId)->childById(bufferId));

  // source must be a query too
  Buffer::Type sourceType = (Buffer::Type)rootItem->childById(netId)->childById(bufferId)->data(0, BufferTypeRole).toInt();
  if(sourceType != Buffer::QueryType)
    return false;
    
  // TODO: warn user about buffermerge!
  qDebug() << "merging" << bufferId << parent.data(BufferUidRole).toInt();
  removeRow(parent.row(), parent.parent());
  
  return true;
}

void BufferTreeModel::bufferUpdated(Buffer *buffer) {
  QModelIndex itemindex = getOrCreateBufferItemIndex(buffer);
  emit invalidateFilter();
  emit dataChanged(itemindex, itemindex);
}

// This Slot indicates that the user has selected a different buffer in the gui
void BufferTreeModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  Q_UNUSED(command)
  if(isBufferIndex(index)) {
    currentBuffer = getBufferByIndex(index);
    bufferActivity(Buffer::NoActivity, currentBuffer);
    emit bufferSelected(currentBuffer);
    emit selectionChanged(index);
  }
}

void BufferTreeModel::bufferActivity(Buffer::ActivityLevel level, Buffer *buffer) {
  BufferTreeItem *bufferItem = static_cast<BufferTreeItem*>(getOrCreateBufferItemIndex(buffer).internalPointer());
  if(buffer != currentBuffer)
    bufferItem->setActivity(level);
  else
    bufferItem->setActivity(Buffer::NoActivity);
  bufferUpdated(buffer);
}

void BufferTreeModel::selectBuffer(Buffer *buffer) {
  QModelIndex index = getOrCreateBufferItemIndex(buffer);
  // SUPER UGLY!
  setCurrentIndex(index, 0);
}

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

#include "bufferinfo.h"
#include "client.h"
#include "signalproxy.h"

/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferTreeItem::BufferTreeItem(Buffer *buffer, TreeItem *parent) : TreeItem(parent) {
  buf = buffer;
  activity = Buffer::NoActivity;
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
      return buf->displayName();
    case 1:
      return buf->networkName();
    default:
      return QString();
  }
}

QColor BufferTreeItem::foreground(int /*column*/) const {
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
    case BufferTreeModel::BufferNameRole:
      return buf->bufferName();
    case BufferTreeModel::BufferTypeRole:
      return buf->bufferType();
    case BufferTreeModel::BufferActiveRole:
      return buf->isActive();
    case BufferTreeModel::BufferInfoRole:
      return buf->bufferInfo().uid();
    default:
      return QVariant();
  }
}

Qt::ItemFlags BufferTreeItem::flags() const {
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  if(buf->bufferType() == Buffer::QueryBuffer)
    flags |= Qt::ItemIsDropEnabled;

  return flags;
}

/*****************************************
*  Network Items
*****************************************/
NetworkTreeItem::NetworkTreeItem(const QString &network, TreeItem *parent) : TreeItem(parent) {
  net = network;
  itemData << net << "";
}

uint NetworkTreeItem::id() const {
  return qHash(net);
}

Qt::ItemFlags NetworkTreeItem::flags() const {
  return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
}

/*****************************************
 * BufferTreeModel
 *****************************************/
BufferTreeModel::BufferTreeModel(QObject *parent)
  : TreeModel(BufferTreeModel::defaultHeader(), parent)
{
  Client::signalProxy()->attachSignal(this, SIGNAL(fakeUserInput(BufferInfo, QString)), SIGNAL(sendInput(BufferInfo, QString)));
}

QList<QVariant >BufferTreeModel::defaultHeader() {
  QList<QVariant> data;
  data << tr("Buffer") << tr("Network");
  return data;
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
  TreeItem *networkItem;

  if(not(networkItem = rootItem->childById(qHash(net)))) {
    int nextRow = rootItem->childCount();
    networkItem = new NetworkTreeItem(net, rootItem);
    
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
  
  if(not(bufferItem = networkItem->childById(buffer->bufferInfo().uid()))) {
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
  QStringList types;
  types << "application/Quassel/BufferItem/row"
    << "application/Quassel/BufferItem/network"
    << "application/Quassel/BufferItem/bufferInfo";
  return types;
}

QMimeData *BufferTreeModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *mimeData = new QMimeData();

  QModelIndex index = indexes.first();
  
  mimeData->setData("application/Quassel/BufferItem/row", QByteArray::number(index.row()));
  mimeData->setData("application/Quassel/BufferItem/network", getBufferByIndex(index)->networkName().toUtf8());
  mimeData->setData("application/Quassel/BufferItem/bufferInfo", QByteArray::number(getBufferByIndex(index)->bufferInfo().uid()));
  return mimeData;
}

bool BufferTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex &parent) {
  foreach(QString mimeType, mimeTypes()) {
    if(!(data->hasFormat(mimeType)))
      return false; // whatever the drop is... it's not a buffer...
  }
  
  int sourcerow = data->data("application/Quassel/BufferItem/row").toInt();
  QString network = QString::fromUtf8(data->data("application/Quassel/BufferItem/network"));
  
  Q_ASSERT(rootItem->childById(qHash(network)));

  if(parent == QModelIndex()) // can't be a query...
    return false;
  
  Buffer *sourceBuffer = static_cast<BufferTreeItem *>(rootItem->childById(qHash(network))->child(sourcerow))->buffer();
  Buffer *targetBuffer = getBufferByIndex(parent);

  if(!(sourceBuffer->bufferType() & targetBuffer->bufferType() & Buffer::QueryBuffer)) // only queries can be merged
    return false;
  
  if(sourceBuffer == targetBuffer) // we won't merge with ourself :)
    return false;
    
  // TODO: warn user about buffermerge!
  qDebug() << "merging" << sourceBuffer->bufferName() << "with" << targetBuffer->bufferName();
  removeRow(parent.row(), BufferTreeModel::parent(parent));
  
  return true;
}

void BufferTreeModel::bufferUpdated(Buffer *buffer) {
  QModelIndex itemindex = getOrCreateBufferItemIndex(buffer);
  emit invalidateFilter();
  emit dataChanged(itemindex, itemindex);
}

// This Slot indicates that the user has selected a different buffer in the gui
void BufferTreeModel::changeCurrent(const QModelIndex &current, const QModelIndex &/*previous*/) {
  if(isBufferIndex(current)) {
    currentBuffer = getBufferByIndex(current);
    bufferActivity(Buffer::NoActivity, currentBuffer);
    emit bufferSelected(currentBuffer);
    emit selectionChanged(current);
  }
}

// we received a double click on a buffer, so we're going to join it
void BufferTreeModel::doubleClickReceived(const QModelIndex &clicked) {
  if(isBufferIndex(clicked)) {
    Buffer *buffer = getBufferByIndex(clicked);
    if(!buffer->isStatusBuffer()) 
      emit fakeUserInput(buffer->bufferInfo(), QString("/join " + buffer->bufferName()));
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
  //emit selectionChanged(index);
  changeCurrent(index, QModelIndex());
}

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

#include <QColor>  // FIXME Dependency on QtGui!

#include "buffertreemodel.h"

#include "mappedselectionmodel.h"
#include <QAbstractItemView>

#include "bufferinfo.h"
#include "client.h"
#include "signalproxy.h"
#include "networkinfo.h"
#include "ircchannel.h"
#include "ircuser.h"

/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferTreeItem::BufferTreeItem(Buffer *buffer, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "bufferName" << "topic" << "nickCount", parent),
    buf(buffer),
    activity(Buffer::NoActivity)
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  if(buf->bufferType() == Buffer::QueryType)
    flags |= Qt::ItemIsDropEnabled;
  setFlags(flags);
}

quint64 BufferTreeItem::id() const {
  return buf->bufferInfo().uid();
}

void BufferTreeItem::setActivity(const Buffer::ActivityLevel &level) {
  activity = level;
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
  case BufferTreeModel::BufferUidRole:
    return buf->bufferInfo().uid();
  case BufferTreeModel::NetworkIdRole:
    return buf->bufferInfo().networkId();
  case BufferTreeModel::BufferTypeRole:
    return int(buf->bufferType());
  case BufferTreeModel::BufferActiveRole:
    return buf->isActive();
  case Qt::ForegroundRole:
    return foreground(column);
  default:
    return PropertyMapItem::data(column, role);
  }
}

void BufferTreeItem::attachIrcChannel(IrcChannel *ircChannel) {
  if(!ircChannel)
    return;
  
  _ircChannel = ircChannel;

  connect(ircChannel, SIGNAL(topicSet(QString)),
	  this, SLOT(setTopic(QString)));
  connect(ircChannel, SIGNAL(ircUserJoined(IrcUser *)),
	  this, SLOT(join(IrcUser *)));
  connect(ircChannel, SIGNAL(ircUserParted(IrcUser *)),
	  this, SLOT(part(IrcUser *)));
}

QString BufferTreeItem::bufferName() const {
  return buf->name();
}

QString BufferTreeItem::topic() const {
  if(_ircChannel)
    return _ircChannel->topic();
  else
    return QString();
}

int BufferTreeItem::nickCount() const {
  if(_ircChannel)
    return _ircChannel->ircUsers().count();
  else
    return 0;
}

void BufferTreeItem::setTopic(const QString &topic) {
  Q_UNUSED(topic);
  emit dataChanged(1);
}

void BufferTreeItem::join(IrcUser *ircUser) {
  Q_UNUSED(ircUser);
  emit dataChanged(2);
}

void BufferTreeItem::part(IrcUser *ircUser) {
  Q_UNUSED(ircUser);
  emit dataChanged(2);
}

/*****************************************
*  Network Items
*****************************************/
NetworkTreeItem::NetworkTreeItem(const uint &netid, const QString &network, AbstractTreeItem *parent)
  : PropertyMapItem(QList<QString>() << "networkName" << "currentServer" << "nickCount", parent),
    _networkId(netid),
    _networkName(network)
{
  setFlags(Qt::ItemIsEnabled);
}

QVariant NetworkTreeItem::data(int column, int role) const {
  switch(role) {
  case BufferTreeModel::NetworkIdRole:
    return _networkId;
  case BufferTreeModel::ItemTypeRole:
    return BufferTreeModel::NetworkItem;
  default:
    return PropertyMapItem::data(column, role);
  }
}

quint64 NetworkTreeItem::id() const {
  return _networkId;
}

QString NetworkTreeItem::networkName() const {
  if(_networkInfo)
    return _networkInfo->networkName();
  else
    return _networkName;
}

QString NetworkTreeItem::currentServer() const {
  if(_networkInfo)
    return _networkInfo->currentServer();
  else
    return QString();
}

int NetworkTreeItem::nickCount() const {
  BufferTreeItem *bufferItem;
  int count = 0;
  for(int i = 0; i < childCount(); i++) {
    bufferItem = qobject_cast<BufferTreeItem *>(child(i));
    if(!bufferItem)
      continue;
    count += bufferItem->nickCount();
  }
  return count;
}

void NetworkTreeItem::attachNetworkInfo(NetworkInfo *networkInfo) {
  if(!networkInfo)
    return;
  
  _networkInfo = networkInfo;

  connect(networkInfo, SIGNAL(networkNameSet(QString)),
	  this, SLOT(setNetworkName(QString)));
  connect(networkInfo, SIGNAL(currentServerSet(QString)),
	  this, SLOT(setCurrentServer(QString)));
  connect(networkInfo, SIGNAL(ircChannelAdded(QString)),
	  this, SLOT(attachIrcChannel(QString)));
  // FIXME: connect this and that...
}

void NetworkTreeItem::attachIrcChannel(const QString &channelName) {
  IrcChannel *ircChannel = _networkInfo->ircChannel(channelName);
  if(!ircChannel) {
    qWarning() << "NetworkTreeItem::attachIrcChannel(): unkown Channel" << channelName;
    return;
  }
  
  BufferTreeItem *bufferItem;
  for(int i = 0; i < childCount(); i++) {
    bufferItem = qobject_cast<BufferTreeItem *>(child(i));
    if(bufferItem->bufferName() == ircChannel->name()) {
      bufferItem->attachIrcChannel(ircChannel);
      break;
    }
  }
}

void NetworkTreeItem::setNetworkName(const QString &networkName) {
  Q_UNUSED(networkName);
  emit dataChanged(0);
}

void NetworkTreeItem::setCurrentServer(const QString &serverName) {
  Q_UNUSED(serverName);
  emit dataChanged(1);
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
  data << tr("Buffer") << tr("Topic") << tr("Nick Count");
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
  // FIXME get rid of this
  Q_ASSERT(item->buffer() == Client::instance()->buffer(item->id()));
  return item->buffer();
}


// experimental stuff :)
QModelIndex BufferTreeModel::networkIndex(uint networkId) {
  return indexById(networkId);
}

NetworkTreeItem *BufferTreeModel::network(uint networkId) {
  return qobject_cast<NetworkTreeItem *>(rootItem->childById(networkId));
}

NetworkTreeItem *BufferTreeModel::newNetwork(uint networkId, const QString &networkName) {
  NetworkTreeItem *networkItem = network(networkId);

  if(networkItem == 0) {
    networkItem = new NetworkTreeItem(networkId, networkName, rootItem);
    appendChild(rootItem, networkItem);
  }

  Q_ASSERT(networkItem);
  return networkItem;
}

QModelIndex BufferTreeModel::bufferIndex(BufferInfo bufferInfo) {
  QModelIndex networkIdx = networkIndex(bufferInfo.networkId());
  if(!networkIdx.isValid())
    return QModelIndex();
  else
    return indexById(bufferInfo.uid(), networkIdx);
}

BufferTreeItem *BufferTreeModel::buffer(BufferInfo bufferInfo) {
  QModelIndex bufferIdx = bufferIndex(bufferInfo);
  if(bufferIdx.isValid())
    return static_cast<BufferTreeItem *>(bufferIdx.internalPointer());
  else
    return 0;
}

BufferTreeItem *BufferTreeModel::newBuffer(BufferInfo bufferInfo) {
  BufferTreeItem *bufferItem = buffer(bufferInfo);
  if(bufferItem == 0) {
    NetworkTreeItem *networkItem = newNetwork(bufferInfo.networkId(), bufferInfo.network());

    // FIXME: get rid of the buffer pointer
    Buffer *buffer = Client::instance()->buffer(bufferInfo.uid());
    bufferItem = new BufferTreeItem(buffer, networkItem);
    appendChild(networkItem, bufferItem);
  }

  Q_ASSERT(bufferItem);
  return bufferItem;
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

void BufferTreeModel::attachNetworkInfo(NetworkInfo *networkInfo) {
  NetworkTreeItem *networkItem = network(networkInfo->networkId());
  if(!networkItem) {
    qWarning() << "BufferTreeModel::attachNetworkInfo(): network is unknown!";
    return;
  }
  networkItem->attachNetworkInfo(networkInfo);
}

void BufferTreeModel::bufferUpdated(Buffer *buffer) {
  BufferTreeItem *bufferItem = newBuffer(buffer->bufferInfo());
  QModelIndex itemindex = indexByItem(bufferItem);
  emit dataChanged(itemindex, itemindex);
}

// This Slot indicates that the user has selected a different buffer in the gui
void BufferTreeModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  Q_UNUSED(command)
  Buffer *newCurrentBuffer;
  if(isBufferIndex(index) && currentBuffer != (newCurrentBuffer = getBufferByIndex(index))) {
    currentBuffer = newCurrentBuffer;
    bufferActivity(Buffer::NoActivity, currentBuffer);
    emit bufferSelected(currentBuffer);
    emit selectionChanged(index);
  }
}

void BufferTreeModel::bufferActivity(Buffer::ActivityLevel level, Buffer *buf) {
  BufferTreeItem *bufferItem = buffer(buf->bufferInfo());
  if(!bufferItem) {
    qWarning() << "BufferTreeModel::bufferActivity(): received Activity Info for uknown Buffer";
    return;
  }
  
  if(buf != currentBuffer)
    bufferItem->setActivity(level);
  else
    bufferItem->setActivity(Buffer::NoActivity);
  bufferUpdated(buf);
}

void BufferTreeModel::selectBuffer(Buffer *buffer) {
  QModelIndex index = bufferIndex(buffer->bufferInfo());
  if(!index.isValid()) {
    qWarning() << "BufferTreeModel::selectBuffer(): unknown Buffer has been selected.";
    return;
  }
  // SUPER UGLY!
  setCurrentIndex(index, 0);
}

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

#include <QColor>  // FIXME Dependency on QtGui!

#include "networkmodel.h"

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
BufferItem::BufferItem(Buffer *buffer, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "bufferName" << "topic" << "nickCount", parent),
    buf(buffer),
    activity(Buffer::NoActivity)
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  if(buf->bufferType() == Buffer::QueryType)
    flags |= Qt::ItemIsDropEnabled;
  setFlags(flags);
}

quint64 BufferItem::id() const {
  return buf->bufferInfo().uid();
}

void BufferItem::setActivity(const Buffer::ActivityLevel &level) {
  activity = level;
}

QColor BufferItem::foreground(int column) const {
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

QVariant BufferItem::data(int column, int role) const {
  switch(role) {
  case NetworkModel::ItemTypeRole:
    return NetworkModel::BufferItemType;
  case NetworkModel::BufferUidRole:
    return buf->bufferInfo().uid();
  case NetworkModel::NetworkIdRole:
    return buf->bufferInfo().networkId();
  case NetworkModel::BufferTypeRole:
    return int(buf->bufferType());
  case NetworkModel::BufferActiveRole:
    return buf->isActive();
  case Qt::ForegroundRole:
    return foreground(column);
  default:
    return PropertyMapItem::data(column, role);
  }
}

void BufferItem::attachIrcChannel(IrcChannel *ircChannel) {
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

QString BufferItem::bufferName() const {
  return buf->name();
}

QString BufferItem::topic() const {
  if(_ircChannel)
    return _ircChannel->topic();
  else
    return QString();
}

int BufferItem::nickCount() const {
  if(_ircChannel)
    return _ircChannel->ircUsers().count();
  else
    return 0;
}

void BufferItem::setTopic(const QString &topic) {
  Q_UNUSED(topic);
  emit dataChanged(1);
}

void BufferItem::join(IrcUser *ircUser) {
  emit newChild(new IrcUserItem(ircUser, this));
  emit dataChanged(2);
}

void BufferItem::part(IrcUser *ircUser) {
  Q_UNUSED(ircUser);
  emit dataChanged(2);
}

/*****************************************
*  Network Items
*****************************************/
NetworkItem::NetworkItem(const uint &netid, const QString &network, AbstractTreeItem *parent)
  : PropertyMapItem(QList<QString>() << "networkName" << "currentServer" << "nickCount", parent),
    _networkId(netid),
    _networkName(network)
{
  setFlags(Qt::ItemIsEnabled);
}

QVariant NetworkItem::data(int column, int role) const {
  switch(role) {
  case NetworkModel::NetworkIdRole:
    return _networkId;
  case NetworkModel::ItemTypeRole:
    return NetworkModel::NetworkItemType;
  default:
    return PropertyMapItem::data(column, role);
  }
}

quint64 NetworkItem::id() const {
  return _networkId;
}

QString NetworkItem::networkName() const {
  if(_networkInfo)
    return _networkInfo->networkName();
  else
    return _networkName;
}

QString NetworkItem::currentServer() const {
  if(_networkInfo)
    return _networkInfo->currentServer();
  else
    return QString();
}

int NetworkItem::nickCount() const {
  BufferItem *bufferItem;
  int count = 0;
  for(int i = 0; i < childCount(); i++) {
    bufferItem = qobject_cast<BufferItem *>(child(i));
    if(!bufferItem)
      continue;
    count += bufferItem->nickCount();
  }
  return count;
}

void NetworkItem::attachNetworkInfo(NetworkInfo *networkInfo) {
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

void NetworkItem::attachIrcChannel(const QString &channelName) {
  IrcChannel *ircChannel = _networkInfo->ircChannel(channelName);
  if(!ircChannel) {
    qWarning() << "NetworkItem::attachIrcChannel(): unkown Channel" << channelName;
    return;
  }
  
  BufferItem *bufferItem;
  for(int i = 0; i < childCount(); i++) {
    bufferItem = qobject_cast<BufferItem *>(child(i));
    if(bufferItem->bufferName() == ircChannel->name()) {
      bufferItem->attachIrcChannel(ircChannel);
      break;
    }
  }
}

void NetworkItem::setNetworkName(const QString &networkName) {
  Q_UNUSED(networkName);
  emit dataChanged(0);
}

void NetworkItem::setCurrentServer(const QString &serverName) {
  Q_UNUSED(serverName);
  emit dataChanged(1);
}

/*****************************************
*  Irc User Items
*****************************************/
IrcUserItem::IrcUserItem(IrcUser *ircUser, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "nickName", parent),
    _ircUser(ircUser)
{
  connect(ircUser, SIGNAL(destroyed()),
	  this, SLOT(ircUserDestroyed()));
  
  connect(ircUser, SIGNAL(nickSet(QString)),
	  this, SLOT(setNick(QString)));
}

QString IrcUserItem::nickName() {
  return _ircUser->nick();
}

void IrcUserItem::setNick(QString newNick) {
  Q_UNUSED(newNick);
  emit dataChanged(0);
}
void IrcUserItem::ircUserDestroyed() {
  deleteLater();
}


/*****************************************
 * NetworkModel
 *****************************************/
NetworkModel::NetworkModel(QObject *parent)
  : TreeModel(NetworkModel::defaultHeader(), parent)
{
}

QList<QVariant >NetworkModel::defaultHeader() {
  QList<QVariant> data;
  data << tr("Buffer") << tr("Topic") << tr("Nick Count");
  return data;
}

bool NetworkModel::isBufferIndex(const QModelIndex &index) const {
  return index.data(NetworkModel::ItemTypeRole) == NetworkModel::BufferItemType;
}

Buffer *NetworkModel::getBufferByIndex(const QModelIndex &index) const {
  BufferItem *item = static_cast<BufferItem *>(index.internalPointer());
  // FIXME get rid of this
  Q_ASSERT(item->buffer() == Client::instance()->buffer(item->id()));
  return item->buffer();
}


// experimental stuff :)
QModelIndex NetworkModel::networkIndex(uint networkId) {
  return indexById(networkId);
}

NetworkItem *NetworkModel::network(uint networkId) {
  return qobject_cast<NetworkItem *>(rootItem->childById(networkId));
}

NetworkItem *NetworkModel::newNetwork(uint networkId, const QString &networkName) {
  NetworkItem *networkItem = network(networkId);

  if(networkItem == 0) {
    networkItem = new NetworkItem(networkId, networkName, rootItem);
    appendChild(rootItem, networkItem);
  }

  Q_ASSERT(networkItem);
  return networkItem;
}

QModelIndex NetworkModel::bufferIndex(BufferInfo bufferInfo) {
  QModelIndex networkIdx = networkIndex(bufferInfo.networkId());
  if(!networkIdx.isValid())
    return QModelIndex();
  else
    return indexById(bufferInfo.uid(), networkIdx);
}

BufferItem *NetworkModel::buffer(BufferInfo bufferInfo) {
  QModelIndex bufferIdx = bufferIndex(bufferInfo);
  if(bufferIdx.isValid())
    return static_cast<BufferItem *>(bufferIdx.internalPointer());
  else
    return 0;
}

BufferItem *NetworkModel::newBuffer(BufferInfo bufferInfo) {
  BufferItem *bufferItem = buffer(bufferInfo);
  if(bufferItem == 0) {
    NetworkItem *networkItem = newNetwork(bufferInfo.networkId(), bufferInfo.network());

    // FIXME: get rid of the buffer pointer
    Buffer *buffer = Client::instance()->buffer(bufferInfo.uid());
    bufferItem = new BufferItem(buffer, networkItem);
    appendChild(networkItem, bufferItem);
  }

  Q_ASSERT(bufferItem);
  return bufferItem;
}

QStringList NetworkModel::mimeTypes() const {
  // mimetypes we accept for drops
  QStringList types;
  // comma separated list of colon separated pairs of networkid and bufferid
  // example: 0:1,0:2,1:4
  types << "application/Quassel/BufferItemList";
  return types;
}

bool NetworkModel::mimeContainsBufferList(const QMimeData *mimeData) {
  return mimeData->hasFormat("application/Quassel/BufferItemList");
}

QList< QPair<uint, uint> > NetworkModel::mimeDataToBufferList(const QMimeData *mimeData) {
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


QMimeData *NetworkModel::mimeData(const QModelIndexList &indexes) const {
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

bool NetworkModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
  Q_UNUSED(action)
  Q_UNUSED(row)
  Q_UNUSED(column)

  if(!mimeContainsBufferList(data))
    return false;

  // target must be a query
  Buffer::Type targetType = (Buffer::Type)parent.data(NetworkModel::BufferTypeRole).toInt();
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

void NetworkModel::attachNetworkInfo(NetworkInfo *networkInfo) {
  NetworkItem *networkItem = network(networkInfo->networkId());
  if(!networkItem) {
    qWarning() << "NetworkModel::attachNetworkInfo(): network is unknown!";
    return;
  }
  networkItem->attachNetworkInfo(networkInfo);
}

void NetworkModel::bufferUpdated(Buffer *buffer) {
  BufferItem *bufferItem = newBuffer(buffer->bufferInfo());
  QModelIndex itemindex = indexByItem(bufferItem);
  emit dataChanged(itemindex, itemindex);
}

void NetworkModel::bufferActivity(Buffer::ActivityLevel level, Buffer *buf) {
  BufferItem *bufferItem = buffer(buf->bufferInfo());
  if(!bufferItem) {
    qWarning() << "NetworkModel::bufferActivity(): received Activity Info for uknown Buffer";
    return;
  }
  bufferItem->setActivity(level);
  bufferUpdated(buf);
}


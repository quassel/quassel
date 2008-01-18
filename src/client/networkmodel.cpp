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

#include "networkmodel.h"

#include <QAbstractItemView>

#include "bufferinfo.h"
#include "client.h"
#include "signalproxy.h"
#include "network.h"
#include "ircchannel.h"
#include "ircuser.h"

#include "util.h" // get rid of this (needed for isChannelName)

/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferItem::BufferItem(BufferInfo bufferInfo, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "bufferName" << "topic" << "nickCount", parent),
    _bufferInfo(bufferInfo),
    _activity(NoActivity)
{
  // determine BufferType
  if(bufferInfo.buffer().isEmpty())
    _type = StatusType;
  else if(isChannelName(bufferInfo.buffer()))
    _type = ChannelType;
  else
    _type = QueryType;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  if(bufferType() == QueryType)
    flags |= Qt::ItemIsDropEnabled;
  setFlags(flags);
}


const BufferInfo &BufferItem::bufferInfo() const {
  return _bufferInfo;
}

quint64 BufferItem::id() const {
  return bufferInfo().uid();
}

bool BufferItem::isStatusBuffer() const {
   return bufferType() == StatusType;
}

BufferItem::Type BufferItem::bufferType() const {
  return _type;
}

bool BufferItem::isActive() const {
  if(bufferType() == ChannelType)
    return _ircChannel;
  else
    return qobject_cast<NetworkItem *>(parent())->isActive();
}

BufferItem::ActivityLevel BufferItem::activity() const {
  return _activity;
}

void BufferItem::setActivity(const ActivityLevel &level) {
  _activity = level;
}

void BufferItem::addActivity(const ActivityLevel &level) {
  _activity |= level;
}

QVariant BufferItem::data(int column, int role) const {
  switch(role) {
  case NetworkModel::ItemTypeRole:
    return NetworkModel::BufferItemType;
  case NetworkModel::BufferIdRole:
    return bufferInfo().uid();
  case NetworkModel::NetworkIdRole:
    return bufferInfo().networkId();
  case NetworkModel::BufferTypeRole:
    return int(bufferType());
  case NetworkModel::ItemActiveRole:
    return isActive();
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
  connect(ircChannel, SIGNAL(destroyed()),
	  this, SLOT(ircChannelDestroyed()));
  connect(ircChannel, SIGNAL(ircUserModesSet(IrcUser *, QString)),
	  this, SLOT(userModeChanged(IrcUser *)));
  connect(ircChannel, SIGNAL(ircUserModeAdded(IrcUser *, QString)),
	  this, SLOT(userModeChanged(IrcUser *)));
  connect(ircChannel, SIGNAL(ircUserModeRemoved(IrcUser *, QString)),
	  this, SLOT(userModeChanged(IrcUser *)));
  
  emit dataChanged();
}

void BufferItem::ircChannelDestroyed() {
  emit dataChanged();
  for(int i = 0; i < childCount(); i++) {
    emit childDestroyed(i);
    removeChild(i);
  }
}

QString BufferItem::bufferName() const {
  if(bufferType() == StatusType)
    return tr("Status Buffer");
  else
    return bufferInfo().buffer();
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
  if(!ircUser)
    return;

  addUserToCategory(ircUser);
  emit dataChanged(2);
}

void BufferItem::addUserToCategory(IrcUser *ircUser) {
  Q_ASSERT(_ircChannel);
  
  UserCategoryItem *categoryItem;
  int categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
  if(!(categoryItem = qobject_cast<UserCategoryItem *>(childById(qHash(categoryId))))) {
    categoryItem = new UserCategoryItem(categoryId, this);
    emit newChild(categoryItem);
  }
  
  categoryItem->addUser(ircUser);
}

void BufferItem::part(IrcUser *ircUser) {
  Q_UNUSED(ircUser);
  emit dataChanged(2);
}

void BufferItem::removeUserFromCategory(IrcUser *ircUser) {
  UserCategoryItem *categoryItem = 0;
  IrcUserItem *userItem;
  for(int i = 0; i < childCount(); i++) {
    categoryItem = qobject_cast<UserCategoryItem *>(child(i));
    if((userItem = qobject_cast<IrcUserItem *>(categoryItem->childById((quint64)ircUser)))) {
      userItem->deleteLater();
      return;
    }
  }
}

void BufferItem::userModeChanged(IrcUser *ircUser) {
  removeUserFromCategory(ircUser);
  addUserToCategory(ircUser);
}

/*****************************************
*  Network Items
*****************************************/
NetworkItem::NetworkItem(const NetworkId &netid, AbstractTreeItem *parent)
  : PropertyMapItem(QList<QString>() << "networkName" << "currentServer" << "nickCount", parent),
    _networkId(netid)
{
  setFlags(Qt::ItemIsEnabled);
}

QVariant NetworkItem::data(int column, int role) const {
  switch(role) {
  case NetworkModel::NetworkIdRole:
    return _networkId;
  case NetworkModel::ItemTypeRole:
    return NetworkModel::NetworkItemType;
  case NetworkModel::ItemActiveRole:
    return isActive();
  default:
    return PropertyMapItem::data(column, role);
  }
}

quint64 NetworkItem::id() const {
  return _networkId;
}

bool NetworkItem::isActive() const {
  return _network;
}

QString NetworkItem::networkName() const {
  if(_network)
    return _network->networkName();
  else
    return QString();
}

QString NetworkItem::currentServer() const {
  if(_network)
    return _network->currentServer();
  else
    return QString();
}

int NetworkItem::nickCount() const {
  if(_network)
    return _network->ircUsers().count();
  else
    return 0;
}

void NetworkItem::attachNetwork(Network *network) {
  if(!network)
    return;
  
  _network = network;

  connect(network, SIGNAL(networkNameSet(QString)),
	  this, SLOT(setNetworkName(QString)));
  connect(network, SIGNAL(currentServerSet(QString)),
	  this, SLOT(setCurrentServer(QString)));
  connect(network, SIGNAL(ircChannelAdded(QString)),
	  this, SLOT(attachIrcChannel(QString)));
  // FIXME: connect this and that...

  emit dataChanged();
}

void NetworkItem::attachIrcChannel(const QString &channelName) {
  IrcChannel *ircChannel = _network->ircChannel(channelName);
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
*  User Category Items (like @vh etc.)
*****************************************/
// we hardcode this even though we have PREFIX in network... but that wouldn't help with mapping modes to
// category strings anyway.
const QList<UserCategoryItem::Category> UserCategoryItem::categories = QList<UserCategoryItem::Category>() << UserCategoryItem::Category('q', "Owners")
													   << UserCategoryItem::Category('a', "Admins")
													   << UserCategoryItem::Category('a', "Admins")
													   << UserCategoryItem::Category('o', "Operators")
													   << UserCategoryItem::Category('h', "Half-Ops")
													   << UserCategoryItem::Category('v', "Voiced");

UserCategoryItem::UserCategoryItem(int category, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "categoryId", parent),
    _category(category)
{
  connect(this, SIGNAL(childDestroyed(int)),
	  this, SLOT(checkNoChilds()));
}

QString UserCategoryItem::categoryId() {
  if(_category < categories.count())
    return categories[_category].displayString;
  else
    return QString("Users");
}

void UserCategoryItem::checkNoChilds() {
  if(childCount() == 0)
    deleteLater();
}

quint64 UserCategoryItem::id() const {
  return qHash(_category);
}

void UserCategoryItem::addUser(IrcUser *ircUser) {
  emit newChild(new IrcUserItem(ircUser, this));
}

int UserCategoryItem::categoryFromModes(const QString &modes) {
  for(int i = 0; i < categories.count(); i++) {
    if(modes.contains(categories[i].mode))
      return i;
  }
  return categories.count();
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

IrcUser *IrcUserItem::ircUser() {
  return _ircUser;
}

quint64 IrcUserItem::id() const {
  return (quint64)_ircUser;
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

/*
Buffer *NetworkModel::getBufferByIndex(const QModelIndex &index) const {
  BufferItem *item = static_cast<BufferItem *>(index.internalPointer());
  return Client::instance()->buffer(item->id());
}
*/


// experimental stuff :)
QModelIndex NetworkModel::networkIndex(NetworkId networkId) {
  return indexById(networkId);
}

NetworkItem *NetworkModel::existsNetworkItem(NetworkId networkId) {
  return qobject_cast<NetworkItem *>(rootItem->childById(networkId));
}

NetworkItem *NetworkModel::networkItem(NetworkId networkId) {
  NetworkItem *netItem = existsNetworkItem(networkId);

  if(netItem == 0) {
    netItem = new NetworkItem(networkId, rootItem);
    appendChild(rootItem, netItem);
  }

  Q_ASSERT(netItem);
  return netItem;
}

QModelIndex NetworkModel::bufferIndex(BufferId bufferId) {
  AbstractTreeItem *netItem, *bufferItem;
  for(int i = 0; i < rootItem->childCount(); i++) {
    netItem = rootItem->child(i);
    if((bufferItem = netItem->childById(bufferId))) {
      return indexById(bufferItem->id(), networkIndex(netItem->id()));
    }
  }
  return QModelIndex();
}

BufferItem *NetworkModel::existsBufferItem(const BufferInfo &bufferInfo) {
  QModelIndex bufferIdx = bufferIndex(bufferInfo.uid());
  if(bufferIdx.isValid())
    return static_cast<BufferItem *>(bufferIdx.internalPointer());
  else
    return 0;
}

BufferItem *NetworkModel::bufferItem(const BufferInfo &bufferInfo) {
  BufferItem *bufItem = existsBufferItem(bufferInfo);
  if(bufItem == 0) {
    NetworkItem *netItem = networkItem(bufferInfo.networkId());
    bufItem = new BufferItem(bufferInfo, netItem);
    appendChild(netItem, bufItem);
  }

  Q_ASSERT(bufItem);
  return bufItem;
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

QList< QPair<NetworkId, BufferId> > NetworkModel::mimeDataToBufferList(const QMimeData *mimeData) {
  QList< QPair<NetworkId, BufferId> > bufferList;

  if(!mimeContainsBufferList(mimeData))
    return bufferList;

  QStringList rawBufferList = QString::fromAscii(mimeData->data("application/Quassel/BufferItemList")).split(",");
  NetworkId networkId;
  BufferId bufferUid;
  foreach(QString rawBuffer, rawBufferList) {
    if(!rawBuffer.contains(":"))
      continue;
    networkId = rawBuffer.section(":", 0, 0).toInt();
    bufferUid = rawBuffer.section(":", 1, 1).toInt();
    bufferList.append(qMakePair(networkId, bufferUid));
  }
  return bufferList;
}


QMimeData *NetworkModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *mimeData = new QMimeData();

  QStringList bufferlist;
  QString netid, uid, bufferid;
  foreach(QModelIndex index, indexes) {
    netid = QString::number(index.data(NetworkIdRole).value<NetworkId>());
    uid = QString::number(index.data(BufferIdRole).value<BufferId>());
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
  BufferItem::Type targetType = (BufferItem::Type)parent.data(NetworkModel::BufferTypeRole).toInt();
  if(targetType != BufferItem::QueryType)
    return false;

  QList< QPair<NetworkId, BufferId> > bufferList = mimeDataToBufferList(data);

  // exactly one buffer has to be dropped
  if(bufferList.count() != 1)
    return false;

  NetworkId netId = bufferList.first().first;
  BufferId bufferId = bufferList.first().second;

  // no self merges (would kill us)
  if(bufferId == parent.data(BufferIdRole).value<BufferId>())
    return false; 
  
  Q_ASSERT(rootItem->childById(netId));
  Q_ASSERT(rootItem->childById(netId)->childById(bufferId));

  // source must be a query too
  BufferItem::Type sourceType = (BufferItem::Type)rootItem->childById(netId)->childById(bufferId)->data(0, BufferTypeRole).toInt();
  if(sourceType != BufferItem::QueryType)
    return false;
    
  // TODO: warn user about buffermerge!
  qDebug() << "merging" << bufferId << parent.data(BufferIdRole).value<BufferId>();
  removeRow(parent.row(), parent.parent());
  
  return true;
}

void NetworkModel::attachNetwork(Network *net) {
  NetworkItem *netItem = networkItem(net->networkId());
  netItem->attachNetwork(net);
}

void NetworkModel::bufferUpdated(BufferInfo bufferInfo) {
  BufferItem *bufItem = bufferItem(bufferInfo);
  QModelIndex itemindex = indexByItem(bufItem);
  emit dataChanged(itemindex, itemindex);
}

void NetworkModel::bufferActivity(BufferItem::ActivityLevel level, BufferInfo bufferInfo) {
//   BufferItem *bufferItem = buffer(buf->bufferInfo());
//   if(!bufferItem) {
//     qWarning() << "NetworkModel::bufferActivity(): received Activity Info for uknown Buffer";
//     return;
//   }
//   bufferItem->setActivity(level);
//   bufferUpdated(buf);
}


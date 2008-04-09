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

#include "buffersettings.h"

#include "util.h" // get rid of this (needed for isChannelName)


/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferItem::BufferItem(BufferInfo bufferInfo, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "bufferName" << "topic" << "nickCount", parent),
    _bufferInfo(bufferInfo),
    _bufferName(bufferInfo.bufferName()),
    _activity(Buffer::NoActivity)
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
  if(bufferType() == BufferInfo::QueryBuffer)
    flags |= Qt::ItemIsDropEnabled;

  if(bufferType() == BufferInfo::StatusBuffer) {
    NetworkItem *networkItem = qobject_cast<NetworkItem *>(parent);
    connect(networkItem, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
  }
  setFlags(flags);
}

quint64 BufferItem::id() const {
  return qHash(bufferInfo().bufferId());
}

// bool BufferItem::isStatusBuffer() const {
//   return bufferType() == BufferInfo::StatusBuffer;
// }

bool BufferItem::isActive() const {
  if(bufferType() == BufferInfo::ChannelBuffer)
    return _ircChannel;
  else
    return qobject_cast<NetworkItem *>(parent())->isActive();
}

bool BufferItem::setActivityLevel(Buffer::ActivityLevel level) {
  _activity = level;
  emit dataChanged();
  return true;
}

void BufferItem::updateActivityLevel(Buffer::ActivityLevel level) {
  Buffer::ActivityLevel oldActivity = _activity;
  _activity |= level;
  if(oldActivity != _activity)
    emit dataChanged();
}

QVariant BufferItem::data(int column, int role) const {
  switch(role) {
  case NetworkModel::ItemTypeRole:
    return NetworkModel::BufferItemType;
  case NetworkModel::BufferIdRole:
    return qVariantFromValue(bufferInfo().bufferId());
  case NetworkModel::NetworkIdRole:
    return qVariantFromValue(bufferInfo().networkId());
  case NetworkModel::BufferInfoRole:
    return qVariantFromValue(bufferInfo());
  case NetworkModel::BufferTypeRole:
    return int(bufferType());
  case NetworkModel::ItemActiveRole:
    return isActive();
  case NetworkModel::BufferActivityRole:
    return (int)activityLevel();
  default:
    return PropertyMapItem::data(column, role);
  }
}

bool BufferItem::setData(int column, const QVariant &value, int role) {
  switch(role) {
  case NetworkModel::BufferActivityRole:
    return setActivityLevel((Buffer::ActivityLevel)value.toInt());
  default:
    return PropertyMapItem::setData(column, value, role);
  }
  return true;
}


void BufferItem::attachIrcChannel(IrcChannel *ircChannel) {
  if(!ircChannel)
    return;
  
  _ircChannel = ircChannel;

  connect(ircChannel, SIGNAL(topicSet(QString)),
	  this, SLOT(setTopic(QString)));
  connect(ircChannel, SIGNAL(ircUsersJoined(QList<IrcUser *>)),
	  this, SLOT(join(QList<IrcUser *>)));
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

  if(!ircChannel->ircUsers().isEmpty()) {
    qWarning() << "Channel" << ircChannel->name() << "has already users which is quite surprising :)";
    join(ircChannel->ircUsers());
  }
  
  emit dataChanged();
}

void BufferItem::ircChannelDestroyed() {
  emit dataChanged();
  removeAllChilds();
}

QString BufferItem::bufferName() const {
  if(bufferType() == BufferInfo::StatusBuffer)
    return tr("Status Buffer");
  else
    return _bufferName;
}

void BufferItem::setBufferName(const QString &name) {
  _bufferName = name;
  // as long as we need those bufferInfos, we have to update that one aswell.
  // pretty ugly though :/
  _bufferInfo = BufferInfo(_bufferInfo.bufferId(), _bufferInfo.networkId(), _bufferInfo.type(), _bufferInfo.groupId(), name);
  emit dataChanged(0);
}

QString BufferItem::topic() const {
  if(_ircChannel)
    return _ircChannel->topic();
  else
    return QString();
}

void BufferItem::ircUserDestroyed() {
  // PRIVATE
  IrcUser *ircUser = static_cast<IrcUser *>(sender());
  removeUserFromCategory(ircUser);
  emit dataChanged(2);
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

void BufferItem::join(const QList<IrcUser *> &ircUsers) {
  addUsersToCategory(ircUsers);

  foreach(IrcUser *ircUser, ircUsers) {
    if(!ircUser)
      continue;
    connect(ircUser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
  }
  
  emit dataChanged(2);
}

void BufferItem::addUserToCategory(IrcUser *ircUser) {
  addUsersToCategory(QList<IrcUser *>() << ircUser);
}

void BufferItem::addUsersToCategory(const QList<IrcUser *> &ircUsers) {
  Q_ASSERT(_ircChannel);

  QHash<UserCategoryItem *, QList<IrcUser *> > categories;
  foreach(IrcUser *ircUser, ircUsers) {
    UserCategoryItem *categoryItem;
    int categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
    if(!(categoryItem = qobject_cast<UserCategoryItem *>(childById(qHash(categoryId))))) {
      categoryItem = new UserCategoryItem(categoryId, this);
      categories[categoryItem] = QList<IrcUser *>();
      newChild(categoryItem);
    }
    categories[categoryItem] << ircUser;
  }

  QHash<UserCategoryItem *, QList<IrcUser *> >::const_iterator catIter = categories.constBegin();
  while(catIter != categories.constEnd()) {
    catIter.key()->addUsers(catIter.value());
    catIter++;
  }
}

void BufferItem::part(IrcUser *ircUser) {
  if(!ircUser) {
    qWarning() << bufferName() << "BufferItem::part(): unknown User" << ircUser;
    return;
  }

  disconnect(ircUser, 0, this, 0);
  removeUserFromCategory(ircUser);
  emit dataChanged(2);
}

void BufferItem::removeUserFromCategory(IrcUser *ircUser) {
  if(!_ircChannel) {
    // If we parted the channel there might still be some ircUsers connected.
    // in that case we just ignore the call
    Q_ASSERT(childCount() == 0);
    return;
  }

  bool success = false;
  UserCategoryItem *categoryItem = 0;
  for(int i = 0; i < childCount(); i++) {
    categoryItem = qobject_cast<UserCategoryItem *>(child(i));
    if(success = categoryItem->removeUser(ircUser)) {
      if(categoryItem->childCount() == 0)
	removeChild(i);
      break;
    }
  }

  if(!success) {
    qDebug() << "didn't find User:" << ircUser << qHash(ircUser);
    qDebug() << "==== Childlist for Item:" << this << id() << bufferName() << "====";
    for(int i = 0; i < childCount(); i++) {
      categoryItem = qobject_cast<UserCategoryItem *>(child(i));
      categoryItem->dumpChildList();
    }
    qDebug() << "==== End Of Childlist for Item:" << this << id() << bufferName() << "====";
  }
  Q_ASSERT(success);
}

void BufferItem::userModeChanged(IrcUser *ircUser) {
  Q_ASSERT(_ircChannel);

  UserCategoryItem *categoryItem;
  int categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
  if((categoryItem = qobject_cast<UserCategoryItem *>(childById(qHash(categoryId)))) && categoryItem->childById(qHash(ircUser)))
    return; // already in the right category;
  
  removeUserFromCategory(ircUser);
  addUserToCategory(ircUser);
}

QString BufferItem::toolTip(int column) const {
  Q_UNUSED(column);
  QStringList toolTip;

  switch(bufferType()) {
    case BufferInfo::StatusBuffer: {
      QString netName = Client::network(bufferInfo().networkId())->networkName();
      toolTip.append(tr("<b>Status buffer of %1</b>").arg(netName));
      break;
    }
    case BufferInfo::ChannelBuffer:
      toolTip.append(tr("<b>Channel %1</b>").arg(bufferName()));
      if(isActive()) {
        //TODO: add channel modes 
        toolTip.append(tr("<b>Users:</b> %1").arg(nickCount()));

        BufferSettings s;
        bool showTopic = s.value("DisplayTopicInTooltip", QVariant(false)).toBool();
        if(showTopic) {
          QString _topic = topic();
          if(_topic != "") {
            _topic.replace(QString("<"), QString("&lt;"));
            _topic.replace(QString(">"), QString("&gt;"));
            toolTip.append(QString("<font size='-2'>&nbsp;</font>"));
            toolTip.append(tr("<b>Topic:</b> %1").arg(_topic));
          }
        }
      } else {
        toolTip.append(tr("Not active <br /> Double-click to join"));
      }
      break;
    case BufferInfo::QueryBuffer:
      toolTip.append(tr("<b>Query with %1</b>").arg(bufferName()));
      if(topic() != "") toolTip.append(tr("Away Message: %1").arg(topic()));
      break;
    default: //this should not happen
      toolTip.append(tr("%1 - %2").arg(bufferInfo().bufferId().toInt()).arg(bufferName()));
      break;
  }

  return tr("<p> %1 </p>").arg(toolTip.join("<br />"));
}

/*
void BufferItem::setLastMsgInsert(QDateTime msgDate) {
  if(msgDate.isValid() && msgDate > _lastMsgInsert)
    _lastMsgInsert = msgDate;
}
*/
/*
// FIXME emit dataChanged()
bool BufferItem::setLastSeen() {
  if(_lastSeen > _lastMsgInsert)
    return false;
  
  _lastSeen = _lastMsgInsert;
  BufferSettings(bufferInfo().bufferId()).setLastSeen(_lastSeen);
  return true;
}

QDateTime BufferItem::lastSeen() {
  return _lastSeen;
}
*/
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
    return qVariantFromValue(_networkId);
  case NetworkModel::ItemTypeRole:
    return NetworkModel::NetworkItemType;
  case NetworkModel::ItemActiveRole:
    return isActive();
  default:
    return PropertyMapItem::data(column, role);
  }
}

quint64 NetworkItem::id() const {
  return qHash(_networkId);
}

bool NetworkItem::isActive() const {
  if(_network)
    return _network->isConnected();
  else
    return false;
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
  connect(network, SIGNAL(connectedSet(bool)),
	  this, SIGNAL(dataChanged()));

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
    if(bufferItem->bufferName().toLower() == ircChannel->name().toLower()) {
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


QString NetworkItem::toolTip(int column) const {
  Q_UNUSED(column);

  QStringList toolTip(QString("<b>%1</b>").arg(networkName()));
  toolTip.append(QString("Server: %1").arg(currentServer()));
  toolTip.append(QString("Users: %1").arg(nickCount()));

  return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}


/*****************************************
*  User Category Items (like @vh etc.)
*****************************************/
// we hardcode this even though we have PREFIX in network... but that wouldn't help with mapping modes to
// category strings anyway.
// TODO make this translateable depending on the number of users in a category
//      -> we can't set the real string here, because tr() needs to get the actual number as second param
//      -> tr("%n User(s)", n) needs to be used somewhere where we do know the user number n

const QList<QChar> UserCategoryItem::categories = QList<QChar>() << 'q' << 'a' << 'o' << 'h' << 'v';

UserCategoryItem::UserCategoryItem(int category, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "categoryName", parent),
    _category(category)
{

}

// caching this makes no sense, since we display the user number dynamically
QString UserCategoryItem::categoryName() const {
  int n = childCount();
  switch(_category) {
    case 0: return tr("%n Owner(s)", 0, n);
    case 1: return tr("%n Admin(s)", 0, n);
    case 2: return tr("%n Operator(s)", 0, n);
    case 3: return tr("%n Half-Op(s)", 0, n);
    case 4: return tr("%n Voiced", 0, n);
    default: return tr("%n User(s)", 0, n);
  }
}

quint64 UserCategoryItem::id() const {
  return qHash(_category);
}

void UserCategoryItem::addUsers(const QList<IrcUser *> &ircUsers) {
  QList<AbstractTreeItem *> userItems;
  foreach(IrcUser *ircUser, ircUsers)
    userItems << new IrcUserItem(ircUser, this);
  newChilds(userItems);
}

bool UserCategoryItem::removeUser(IrcUser *ircUser) {
  return removeChildById(qHash(ircUser));
}

int UserCategoryItem::categoryFromModes(const QString &modes) {
  for(int i = 0; i < categories.count(); i++) {
    if(modes.contains(categories[i]))
      return i;
  }
  return categories.count();
}

QVariant UserCategoryItem::data(int column, int role) const {
  switch(role) {
  case TreeModel::SortRole:
    return _category;
  case NetworkModel::ItemActiveRole:
    return true;
  case NetworkModel::ItemTypeRole:
    return NetworkModel::UserCategoryItemType;
  case NetworkModel::BufferIdRole:
    return parent()->data(column, role);
  case NetworkModel::NetworkIdRole:
    return parent()->data(column, role);
  case NetworkModel::BufferInfoRole:
    return parent()->data(column, role);
  default:
    return PropertyMapItem::data(column, role);
  }
}

     
/*****************************************
*  Irc User Items
*****************************************/
IrcUserItem::IrcUserItem(IrcUser *ircUser, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "nickName", parent),
    _ircUser(ircUser),
    _id(qHash(ircUser))
{
  // we don't need to handle the ircUser's destroyed signal since it's automatically removed
  // by the IrcChannel::ircUserParted();
  
  connect(ircUser, SIGNAL(nickSet(QString)),
	  this, SLOT(setNick(QString)));
  connect(ircUser, SIGNAL(awaySet(bool)),
          this, SLOT(setAway(bool)));
}

QString IrcUserItem::nickName() const {
  if(_ircUser)
    return _ircUser->nick();
  else
    return QString();
}

bool IrcUserItem::isActive() const {
  if(_ircUser)
    return !_ircUser->isAway();
  else
    return false;
}

IrcUser *IrcUserItem::ircUser() {
  return _ircUser;
}

quint64 IrcUserItem::id() const {
  return _id;
}

QVariant IrcUserItem::data(int column, int role) const {
  switch(role) {
  case NetworkModel::ItemActiveRole:
    return isActive();
  case NetworkModel::ItemTypeRole:
    return NetworkModel::IrcUserItemType;
  case NetworkModel::BufferIdRole:
    return parent()->data(column, role);
  case NetworkModel::NetworkIdRole:
    return parent()->data(column, role);
  case NetworkModel::BufferInfoRole:
    return parent()->data(column, role);
  default:
    return PropertyMapItem::data(column, role);
  }
}

QString IrcUserItem::toolTip(int column) const {
  Q_UNUSED(column);
  QStringList toolTip(QString("<b>%1</b>").arg(nickName()));
  if(_ircUser->userModes() != "") toolTip[0].append(QString(" (%1)").arg(_ircUser->userModes()));
  if(_ircUser->isAway()) toolTip[0].append(" is away");
  if(!_ircUser->awayMessage().isEmpty()) toolTip[0].append(QString(" (%1)").arg(_ircUser->awayMessage()));
  if(!_ircUser->realName().isEmpty()) toolTip.append(_ircUser->realName());
  if(!_ircUser->ircOperator().isEmpty()) toolTip.append(_ircUser->ircOperator());
  toolTip.append(_ircUser->hostmask().remove(0, _ircUser->hostmask().indexOf("!")+1));

  if(_ircUser->idleTime().isValid()) {
    QDateTime now = QDateTime::currentDateTime();
    QDateTime idle = _ircUser->idleTime();
    int idleTime = idle.secsTo(now);
    toolTip.append(tr("idling since %1").arg(secondsToString(idleTime)));
  }
  if(_ircUser->loginTime().isValid()) {
    toolTip.append(tr("login time: %1").arg(_ircUser->loginTime().toString()));
  }

  if(!_ircUser->server().isEmpty()) toolTip.append(tr("server: %1").arg(_ircUser->server()));

  return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}

void IrcUserItem::setNick(QString newNick) {
  Q_UNUSED(newNick);
  emit dataChanged(0);
}

void IrcUserItem::setAway(bool away) {
  Q_UNUSED(away);
  emit dataChanged(0);
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
  return indexById(qHash(networkId));
}

NetworkItem *NetworkModel::existsNetworkItem(NetworkId networkId) {
  return qobject_cast<NetworkItem *>(rootItem->childById(networkId.toInt()));
}

NetworkItem *NetworkModel::networkItem(NetworkId networkId) {
  NetworkItem *netItem = existsNetworkItem(networkId);

  if(netItem == 0) {
    netItem = new NetworkItem(networkId, rootItem);
    rootItem->newChild(netItem);
  }

  Q_ASSERT(netItem);
  return netItem;
}

void NetworkModel::networkRemoved(const NetworkId &networkId) {
  rootItem->removeChildById(qHash(networkId));
}

QModelIndex NetworkModel::bufferIndex(BufferId bufferId) {
  AbstractTreeItem *netItem, *bufferItem;
  for(int i = 0; i < rootItem->childCount(); i++) {
    netItem = rootItem->child(i);
    if((bufferItem = netItem->childById(qHash(bufferId)))) {
      return indexByItem(bufferItem);
    }
  }
  return QModelIndex();
}

BufferItem *NetworkModel::existsBufferItem(const BufferInfo &bufferInfo) {
  QModelIndex bufferIdx = bufferIndex(bufferInfo.bufferId());
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
    netItem->newChild(bufItem);
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
    netid = QString::number(index.data(NetworkIdRole).value<NetworkId>().toInt());
    uid = QString::number(index.data(BufferIdRole).value<BufferId>().toInt());
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
  BufferInfo::Type targetType = (BufferInfo::Type)parent.data(NetworkModel::BufferTypeRole).toInt();
  if(targetType != BufferInfo::QueryBuffer)
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
  
  Q_ASSERT(rootItem->childById(qHash(netId)));
  Q_ASSERT(rootItem->childById(qHash(netId))->childById(qHash(bufferId)));

  // source must be a query too
  BufferInfo::Type sourceType = (BufferInfo::Type)rootItem->childById(qHash(netId))->childById(qHash(bufferId))->data(0, BufferTypeRole).toInt();
  if(sourceType != BufferInfo::QueryBuffer)
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

void NetworkModel::removeBuffer(BufferId bufferId) {
  const int numNetworks = rootItem->childCount();
  if(numNetworks == 0)
    return;

  for(int i = 0; i < numNetworks; i++) {
    if(rootItem->child(i)->removeChildById(qHash(bufferId)))
      break;
  }
}

/*
void NetworkModel::updateBufferActivity(const Message &msg) {
  BufferItem *buff = bufferItem(msg.bufferInfo());
  Q_ASSERT(buff);

  buff->setLastMsgInsert(msg.timestamp());

  if(buff->lastSeen() >= msg.timestamp())
    return;

  BufferItem::ActivityLevel level = BufferItem::OtherActivity;
  if(msg.type() == Message::Plain || msg.type() == Message::Notice)
    level |= BufferItem::NewMessage;

  if(msg.flags() & Message::Highlight) 
      level |= BufferItem::Highlight;

  bufferItem(msg.bufferInfo())->updateActivity(level);
}
*/

void NetworkModel::setBufferActivity(const BufferInfo &info, Buffer::ActivityLevel level) {
  BufferItem *buff = bufferItem(info);
  Q_ASSERT(buff);

  buff->setActivityLevel(level);
}

const Network *NetworkModel::networkByIndex(const QModelIndex &index) const {
  QVariant netVariant = index.data(NetworkIdRole);
  if(!netVariant.isValid())
    return 0;

  NetworkId networkId = netVariant.value<NetworkId>();
  return Client::network(networkId);
}


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

#include "buffersettings.h"

#include "util.h" // get rid of this (needed for isChannelName)

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

BufferItem *NetworkItem::bufferItem(const BufferInfo &bufferInfo) {
  BufferItem *bufferItem = qobject_cast<BufferItem *>(childById(qHash(bufferInfo.bufferId())));
  if(bufferItem)
    return bufferItem;
  
  switch(bufferInfo.type()) {
  case BufferInfo::StatusBuffer:
    bufferItem = new StatusBufferItem(bufferInfo, this);
    break;
  case BufferInfo::ChannelBuffer:
    bufferItem = new ChannelBufferItem(bufferInfo, this);
    break;
  case BufferInfo::QueryBuffer:
    bufferItem = new QueryBufferItem(bufferInfo, this);
    break;
  default:
    bufferItem = new BufferItem(bufferInfo, this);
  }

  newChild(bufferItem);
  return bufferItem;
}

void NetworkItem::attachNetwork(Network *network) {
  if(!network)
    return;
  
  _network = network;

  connect(network, SIGNAL(networkNameSet(QString)),
	  this, SLOT(setNetworkName(QString)));
  connect(network, SIGNAL(currentServerSet(QString)),
	  this, SLOT(setCurrentServer(QString)));
  connect(network, SIGNAL(ircChannelAdded(IrcChannel *)),
	  this, SLOT(attachIrcChannel(IrcChannel *)));
  connect(network, SIGNAL(ircUserAdded(IrcUser *)),
	  this, SLOT(attachIrcUser(IrcUser *)));
  connect(network, SIGNAL(connectedSet(bool)),
	  this, SIGNAL(dataChanged()));
  connect(network, SIGNAL(destroyed()),
	  this, SIGNAL(dataChanged()));
  
  emit dataChanged();
}

void NetworkItem::attachIrcChannel(IrcChannel *ircChannel) {
  ChannelBufferItem *channelItem;
  for(int i = 0; i < childCount(); i++) {
    channelItem = qobject_cast<ChannelBufferItem *>(child(i));
    if(!channelItem)
      continue;

    if(channelItem->bufferName().toLower() == ircChannel->name().toLower()) {
      channelItem->attachIrcChannel(ircChannel);
      break;
    }
  }
}

void NetworkItem::attachIrcUser(IrcUser *ircUser) {
  QueryBufferItem *queryItem = 0;
  for(int i = 0; i < childCount(); i++) {
    queryItem = qobject_cast<QueryBufferItem *>(child(i));
    if(!queryItem)
      continue;

    if(queryItem->bufferName().toLower() == ircUser->nick().toLower()) {
      queryItem->attachIrcUser(ircUser);
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

  if(_network) {
    toolTip.append(QString("Lag: %1 msecs").arg(_network->latency()));
  }

  return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}

/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferItem::BufferItem(const BufferInfo &bufferInfo, AbstractTreeItem *parent)
  : PropertyMapItem(QStringList() << "bufferName" << "topic" << "nickCount", parent),
    _bufferInfo(bufferInfo),
    _activity(Buffer::NoActivity)
{
  setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
}

void BufferItem::setActivityLevel(Buffer::ActivityLevel level) {
  if(_activity != level) {
    _activity = level;
    emit dataChanged();
  }
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
  qDebug() << "BufferItem::setData(int column, const QVariant &value, int role):" << this << column << value << role;
  switch(role) {
  case NetworkModel::BufferActivityRole:
    setActivityLevel((Buffer::ActivityLevel)value.toInt());
    return true;
  default:
    return PropertyMapItem::setData(column, value, role);
  }
  return true;
}

void BufferItem::setBufferName(const QString &name) {
  _bufferInfo = BufferInfo(_bufferInfo.bufferId(), _bufferInfo.networkId(), _bufferInfo.type(), _bufferInfo.groupId(), name);
  emit dataChanged(0);
}

QString BufferItem::toolTip(int column) const {
  Q_UNUSED(column);
  return tr("<p> %1 - %2 </p>").arg(bufferInfo().bufferId().toInt()).arg(bufferName());
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
*  StatusBufferItem
*****************************************/
StatusBufferItem::StatusBufferItem(const BufferInfo &bufferInfo, NetworkItem *parent)
  : BufferItem(bufferInfo, parent)
{
  Q_ASSERT(parent);
  connect(parent, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
}

QString StatusBufferItem::toolTip(int column) const {
  Q_UNUSED(column);
  QStringList toolTip;

  QString netName = Client::network(bufferInfo().networkId())->networkName();
  toolTip.append(tr("<b>Status buffer of %1</b>").arg(netName));

  return tr("<p> %1 </p>").arg(toolTip.join("<br />"));
}

/*****************************************
*  QueryBufferItem
*****************************************/
QueryBufferItem::QueryBufferItem(const BufferInfo &bufferInfo, NetworkItem *parent)
  : BufferItem(bufferInfo, parent),
    _ircUser(0)
{
  setFlags(flags() | Qt::ItemIsDropEnabled);

  const Network *net = Client::network(bufferInfo.networkId());
  if(!net)
    return;

  IrcUser *ircUser = net->ircUser(bufferInfo.bufferName());
  if(ircUser)
    attachIrcUser(ircUser);

}

bool QueryBufferItem::isActive() const {
  if(_ircUser)
    return !_ircUser->isAway();
  else
    return false;
}

QString QueryBufferItem::toolTip(int column) const {
  // pretty much code duplication of IrcUserItem::toolTip() but inheritance won't solve this... 
  Q_UNUSED(column);
  QStringList toolTip;

  toolTip.append(tr("<b>Query with %1</b>").arg(bufferName()));

  if(_ircUser) {
    if(_ircUser->userModes() != "") toolTip[0].append(QString(" (%1)").arg(_ircUser->userModes()));
    if(_ircUser->isAway()) {
      toolTip[0].append(QString(" (away%1)").arg(!_ircUser->awayMessage().isEmpty() ? (QString(" ") + _ircUser->awayMessage()) : QString()));
    }
    if(!_ircUser->realName().isEmpty()) toolTip.append(_ircUser->realName());
    if(!_ircUser->ircOperator().isEmpty()) toolTip.append(QString("%1 %2").arg(_ircUser->nick()).arg(_ircUser->ircOperator()));
    if(!_ircUser->suserHost().isEmpty()) toolTip.append(_ircUser->suserHost());
    if(!_ircUser->whoisServiceReply().isEmpty()) toolTip.append(_ircUser->whoisServiceReply());
    
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
  }

  return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}

void QueryBufferItem::attachIrcUser(IrcUser *ircUser) {
  _ircUser = ircUser;
  connect(_ircUser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
  connect(_ircUser, SIGNAL(awaySet(bool)), this, SIGNAL(dataChanged()));
  emit dataChanged();
}

void QueryBufferItem::ircUserDestroyed() {
  _ircUser = 0;
  emit dataChanged();
}

/*****************************************
*  ChannelBufferItem
*****************************************/
ChannelBufferItem::ChannelBufferItem(const BufferInfo &bufferInfo, AbstractTreeItem *parent)
  : BufferItem(bufferInfo, parent),
    _ircChannel(0)
{
  const Network *net = Client::network(bufferInfo.networkId());
  if(!net)
    return;

  IrcChannel *ircChannel = net->ircChannel(bufferInfo.bufferName());
  if(ircChannel)
    attachIrcChannel(ircChannel);
}

QString ChannelBufferItem::toolTip(int column) const {
  Q_UNUSED(column);
  QStringList toolTip;

  toolTip.append(tr("<b>Channel %1</b>").arg(bufferName()));
  if(isActive()) {
    //TODO: add channel modes 
    toolTip.append(tr("<b>Users:</b> %1").arg(nickCount()));
    if(_ircChannel) {
      QString channelMode = _ircChannel->channelModeString(); // channelModeString is compiled on the fly -> thus cache the result
      if(!channelMode.isEmpty())
	toolTip.append(tr("<b>Mode:</b> %1").arg(channelMode));
    }
    
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

  return tr("<p> %1 </p>").arg(toolTip.join("<br />"));  
}

void ChannelBufferItem::attachIrcChannel(IrcChannel *ircChannel) {
  Q_ASSERT(!_ircChannel && ircChannel);
  
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

  if(!ircChannel->ircUsers().isEmpty())
    join(ircChannel->ircUsers());
  
  emit dataChanged();
}

void ChannelBufferItem::ircChannelDestroyed() {
  Q_CHECK_PTR(_ircChannel);
  disconnect(_ircChannel, 0, this, 0);
  _ircChannel = 0;
  emit dataChanged();
  removeAllChilds();
}

void ChannelBufferItem::ircUserDestroyed() {
  // PRIVATE
  IrcUser *ircUser = static_cast<IrcUser *>(sender());
  removeUserFromCategory(ircUser);
  emit dataChanged(2);
}

void ChannelBufferItem::join(const QList<IrcUser *> &ircUsers) {
  addUsersToCategory(ircUsers);

  foreach(IrcUser *ircUser, ircUsers) {
    if(!ircUser)
      continue;
    connect(ircUser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
  }
  
  emit dataChanged(2);
}

void ChannelBufferItem::addUserToCategory(IrcUser *ircUser) {
  addUsersToCategory(QList<IrcUser *>() << ircUser);
}

void ChannelBufferItem::addUsersToCategory(const QList<IrcUser *> &ircUsers) {
  Q_ASSERT(_ircChannel);

  QHash<UserCategoryItem *, QList<IrcUser *> > categories;

  int categoryId = -1;
  UserCategoryItem *categoryItem = 0;
  
  foreach(IrcUser *ircUser, ircUsers) {
    categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
    categoryItem = qobject_cast<UserCategoryItem *>(childById(qHash(categoryId)));
    if(!categoryItem) {
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

void ChannelBufferItem::part(IrcUser *ircUser) {
  if(!ircUser) {
    qWarning() << bufferName() << "ChannelBufferItem::part(): unknown User" << ircUser;
    return;
  }

  disconnect(ircUser, 0, this, 0);
  removeUserFromCategory(ircUser);
  emit dataChanged(2);
}

void ChannelBufferItem::removeUserFromCategory(IrcUser *ircUser) {
  if(!_ircChannel) {
    // If we parted the channel there might still be some ircUsers connected.
    // in that case we just ignore the call
    Q_ASSERT(childCount() == 0);
    return;
  }

  UserCategoryItem *categoryItem = 0;
  for(int i = 0; i < childCount(); i++) {
    categoryItem = qobject_cast<UserCategoryItem *>(child(i));
    if(categoryItem->removeUser(ircUser)) {
      if(categoryItem->childCount() == 0)
	removeChild(i);
      break;
    }
  }
}

void ChannelBufferItem::userModeChanged(IrcUser *ircUser) {
  Q_ASSERT(_ircChannel);

  int categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
  UserCategoryItem *categoryItem = qobject_cast<UserCategoryItem *>(childById(qHash(categoryId)));
    
  if(categoryItem) {
    if(categoryItem->childById(qHash(ircUser)))
      return; // already in the right category;
  } else {
    categoryItem = new UserCategoryItem(categoryId, this);
    newChild(categoryItem);
  }

  // find the item that needs reparenting
  IrcUserItem *ircUserItem = 0;
  for(int i = 0; i < childCount(); i++) {
    UserCategoryItem *categoryItem = qobject_cast<UserCategoryItem *>(child(i));
    IrcUserItem *userItem = qobject_cast<IrcUserItem *>(categoryItem->childById(qHash(ircUser)));
    if(userItem) {
      ircUserItem = userItem;
      break;
    }
  }

  if(!ircUserItem) {
    qWarning() << "ChannelBufferItem::userModeChanged(IrcUser *): unable to determine old category of" << ircUser;
    return;
  }
  ircUserItem->reParent(categoryItem);
}

/*****************************************
*  User Category Items (like @vh etc.)
*****************************************/
// we hardcode this even though we have PREFIX in network... but that wouldn't help with mapping modes to
// category strings anyway.
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
  emit dataChanged(0);
}

bool UserCategoryItem::removeUser(IrcUser *ircUser) {
  bool success = removeChildById(qHash(ircUser));
  if(success)
    emit dataChanged(0);
  return success;
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
  
  connect(ircUser, SIGNAL(nickSet(QString)), this, SIGNAL(dataChanged()));
  connect(ircUser, SIGNAL(awaySet(bool)), this, SIGNAL(dataChanged()));
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
  if(_ircUser->isAway()) {
    toolTip[0].append(" is away");
    if(!_ircUser->awayMessage().isEmpty())
      toolTip[0].append(QString(" (%1)").arg(_ircUser->awayMessage()));
  }
  if(!_ircUser->realName().isEmpty()) toolTip.append(_ircUser->realName());
  if(!_ircUser->ircOperator().isEmpty()) toolTip.append(QString("%1 %2").arg(nickName()).arg(_ircUser->ircOperator()));
  if(!_ircUser->suserHost().isEmpty()) toolTip.append(_ircUser->suserHost());
  if(!_ircUser->whoisServiceReply().isEmpty()) toolTip.append(_ircUser->whoisServiceReply());

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
  NetworkItem *netItem = networkItem(bufferInfo.networkId());
  return netItem->bufferItem(bufferInfo);
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


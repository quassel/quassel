/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "networkmodel.h"

#include <QAbstractItemView>
#include <QMimeData>
#include <QTextDocument>        // for Qt::escape()

#include "buffermodel.h"
#include "buffersettings.h"
#include "client.h"
#include "clientignorelistmanager.h"
#include "clientsettings.h"
#include "ircchannel.h"
#include "network.h"
#include "signalproxy.h"

/*****************************************
*  Network Items
*****************************************/
NetworkItem::NetworkItem(const NetworkId &netid, AbstractTreeItem *parent)
    : PropertyMapItem(QList<QString>() << "networkName" << "currentServer" << "nickCount", parent),
    _networkId(netid),
    _statusBufferItem(0)
{
    // DO NOT EMIT dataChanged() DIRECTLY IN NetworkItem
    // use networkDataChanged() instead. Otherwise you will end up in a infinite loop
    // as we "sync" the dataChanged() signals of NetworkItem and StatusBufferItem
    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    connect(this, SIGNAL(networkDataChanged(int)), this, SIGNAL(dataChanged(int)));
    connect(this, SIGNAL(beginRemoveChilds(int, int)), this, SLOT(onBeginRemoveChilds(int, int)));
}


QVariant NetworkItem::data(int column, int role) const
{
    switch (role) {
    case NetworkModel::BufferIdRole:
    case NetworkModel::BufferInfoRole:
    case NetworkModel::BufferTypeRole:
    case NetworkModel::BufferActivityRole:
        if (_statusBufferItem)
            return _statusBufferItem->data(column, role);
        else
            return QVariant();
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


// FIXME shouldn't we check the bufferItemCache here?
BufferItem *NetworkItem::findBufferItem(BufferId bufferId)
{
    BufferItem *bufferItem = 0;

    for (int i = 0; i < childCount(); i++) {
        bufferItem = qobject_cast<BufferItem *>(child(i));
        if (!bufferItem)
            continue;
        if (bufferItem->bufferId() == bufferId)
            return bufferItem;
    }
    return 0;
}


BufferItem *NetworkItem::bufferItem(const BufferInfo &bufferInfo)
{
    BufferItem *bufferItem = findBufferItem(bufferInfo);
    if (bufferItem)
        return bufferItem;

    switch (bufferInfo.type()) {
    case BufferInfo::StatusBuffer:
        _statusBufferItem = new StatusBufferItem(bufferInfo, this);
        bufferItem = _statusBufferItem;
        disconnect(this, SIGNAL(networkDataChanged(int)), this, SIGNAL(dataChanged(int)));
        connect(this, SIGNAL(networkDataChanged(int)), bufferItem, SIGNAL(dataChanged(int)));
        connect(bufferItem, SIGNAL(dataChanged(int)), this, SIGNAL(dataChanged(int)));
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

    // postprocess... this is necessary because Qt doesn't seem to like adding children which already have children on their own
    switch (bufferInfo.type()) {
    case BufferInfo::ChannelBuffer:
    {
        ChannelBufferItem *channelBufferItem = static_cast<ChannelBufferItem *>(bufferItem);
        if (_network) {
            IrcChannel *ircChannel = _network->ircChannel(bufferInfo.bufferName());
            if (ircChannel)
                channelBufferItem->attachIrcChannel(ircChannel);
        }
    }
    break;
    default:
        break;
    }

    return bufferItem;
}


void NetworkItem::attachNetwork(Network *network)
{
    if (!network)
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
        this, SIGNAL(networkDataChanged()));
    connect(network, SIGNAL(destroyed()),
        this, SIGNAL(networkDataChanged()));

    emit networkDataChanged();
}


void NetworkItem::attachIrcChannel(IrcChannel *ircChannel)
{
    ChannelBufferItem *channelItem;
    for (int i = 0; i < childCount(); i++) {
        channelItem = qobject_cast<ChannelBufferItem *>(child(i));
        if (!channelItem)
            continue;

        if (channelItem->bufferName().toLower() == ircChannel->name().toLower()) {
            channelItem->attachIrcChannel(ircChannel);
            return;
        }
    }
}


void NetworkItem::attachIrcUser(IrcUser *ircUser)
{
    QueryBufferItem *queryItem = 0;
    for (int i = 0; i < childCount(); i++) {
        queryItem = qobject_cast<QueryBufferItem *>(child(i));
        if (!queryItem)
            continue;

        if (queryItem->bufferName().toLower() == ircUser->nick().toLower()) {
            queryItem->setIrcUser(ircUser);
            break;
        }
    }
}


void NetworkItem::setNetworkName(const QString &networkName)
{
    Q_UNUSED(networkName);
    emit networkDataChanged(0);
}


void NetworkItem::setCurrentServer(const QString &serverName)
{
    Q_UNUSED(serverName);
    emit networkDataChanged(1);
}


QString NetworkItem::toolTip(int column) const
{
    Q_UNUSED(column);

    QStringList toolTip(QString("<b>%1</b>").arg(Qt::escape(networkName())));
    toolTip.append(tr("Server: %1").arg(Qt::escape(currentServer())));
    toolTip.append(tr("Users: %1").arg(nickCount()));

    if (_network) {
        toolTip.append(tr("Lag: %1 msecs").arg(_network->latency()));
    }

    return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}


void NetworkItem::onBeginRemoveChilds(int start, int end)
{
    for (int i = start; i <= end; i++) {
        StatusBufferItem *statusBufferItem = qobject_cast<StatusBufferItem *>(child(i));
        if (statusBufferItem) {
            _statusBufferItem = 0;
            break;
        }
    }
}


/*****************************************
*  Fancy Buffer Items
*****************************************/
BufferItem::BufferItem(const BufferInfo &bufferInfo, AbstractTreeItem *parent)
    : PropertyMapItem(QStringList() << "bufferName" << "topic" << "nickCount", parent),
    _bufferInfo(bufferInfo),
    _activity(BufferInfo::NoActivity)
{
    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
}


void BufferItem::setActivityLevel(BufferInfo::ActivityLevel level)
{
    if (_activity != level) {
        _activity = level;
        emit dataChanged();
    }
}


void BufferItem::clearActivityLevel()
{
    _activity = BufferInfo::NoActivity;
    _firstUnreadMsgId = MsgId();

    // FIXME remove with core proto v11
    if (!(Client::coreFeatures() & Quassel::SynchronizedMarkerLine)) {
        _markerLineMsgId = _lastSeenMsgId;
    }

    emit dataChanged();
}


void BufferItem::updateActivityLevel(const Message &msg)
{
    if (isCurrentBuffer()) {
        return;
    }

    if (msg.flags() & Message::Self)    // don't update activity for our own messages
        return;

    if (Client::ignoreListManager()
        && Client::ignoreListManager()->match(msg, qobject_cast<NetworkItem *>(parent())->networkName()))
        return;

    if (msg.msgId() <= lastSeenMsgId())
        return;

    bool stateChanged = false;
    if (!firstUnreadMsgId().isValid() || msg.msgId() < firstUnreadMsgId()) {
        stateChanged = true;
        _firstUnreadMsgId = msg.msgId();
    }

    BufferInfo::ActivityLevel oldLevel = activityLevel();

    _activity |= BufferInfo::OtherActivity;
    if (msg.type() & (Message::Plain | Message::Notice | Message::Action))
        _activity |= BufferInfo::NewMessage;

    if (msg.flags() & Message::Highlight)
        _activity |= BufferInfo::Highlight;

    stateChanged |= (oldLevel != _activity);

    if (stateChanged)
        emit dataChanged();
}


QVariant BufferItem::data(int column, int role) const
{
    switch (role) {
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
    case NetworkModel::BufferFirstUnreadMsgIdRole:
        return qVariantFromValue(firstUnreadMsgId());
    case NetworkModel::MarkerLineMsgIdRole:
        return qVariantFromValue(markerLineMsgId());
    default:
        return PropertyMapItem::data(column, role);
    }
}


bool BufferItem::setData(int column, const QVariant &value, int role)
{
    switch (role) {
    case NetworkModel::BufferActivityRole:
        setActivityLevel((BufferInfo::ActivityLevel)value.toInt());
        return true;
    default:
        return PropertyMapItem::setData(column, value, role);
    }
    return true;
}


void BufferItem::setBufferName(const QString &name)
{
    _bufferInfo = BufferInfo(_bufferInfo.bufferId(), _bufferInfo.networkId(), _bufferInfo.type(), _bufferInfo.groupId(), name);
    emit dataChanged(0);
}


void BufferItem::setLastSeenMsgId(MsgId msgId)
{
    _lastSeenMsgId = msgId;

    // FIXME remove with core protocol v11
    if (!(Client::coreFeatures() & Quassel::SynchronizedMarkerLine)) {
        if (!isCurrentBuffer())
            _markerLineMsgId = msgId;
    }

    setActivityLevel(BufferInfo::NoActivity);
}


void BufferItem::setMarkerLineMsgId(MsgId msgId)
{
    _markerLineMsgId = msgId;
    emit dataChanged();
}


bool BufferItem::isCurrentBuffer() const
{
    return _bufferInfo.bufferId() == Client::bufferModel()->currentIndex().data(NetworkModel::BufferIdRole).value<BufferId>();
}


QString BufferItem::toolTip(int column) const
{
    Q_UNUSED(column);
    return tr("<p> %1 - %2 </p>").arg(bufferInfo().bufferId().toInt()).arg(bufferName());
}


/*****************************************
*  StatusBufferItem
*****************************************/
StatusBufferItem::StatusBufferItem(const BufferInfo &bufferInfo, NetworkItem *parent)
    : BufferItem(bufferInfo, parent)
{
}


QString StatusBufferItem::toolTip(int column) const
{
    NetworkItem *networkItem = qobject_cast<NetworkItem *>(parent());
    if (networkItem)
        return networkItem->toolTip(column);
    else
        return QString();
}


/*****************************************
*  QueryBufferItem
*****************************************/
QueryBufferItem::QueryBufferItem(const BufferInfo &bufferInfo, NetworkItem *parent)
    : BufferItem(bufferInfo, parent),
    _ircUser(0)
{
    setFlags(flags() | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

    const Network *net = Client::network(bufferInfo.networkId());
    if (!net)
        return;

    IrcUser *ircUser = net->ircUser(bufferInfo.bufferName());
    setIrcUser(ircUser);
}


QVariant QueryBufferItem::data(int column, int role) const
{
    switch (role) {
    case Qt::EditRole:
        return BufferItem::data(column, Qt::DisplayRole);
    case NetworkModel::IrcUserRole:
        return QVariant::fromValue<QObject *>(_ircUser);
    case NetworkModel::UserAwayRole:
        return (bool)_ircUser ? _ircUser->isAway() : false;
    default:
        return BufferItem::data(column, role);
    }
}


bool QueryBufferItem::setData(int column, const QVariant &value, int role)
{
    if (column != 0)
        return BufferItem::setData(column, value, role);

    switch (role) {
    case Qt::EditRole:
    {
        QString newName = value.toString();
        if (!newName.isEmpty()) {
            Client::renameBuffer(bufferId(), newName);
            return true;
        }
        else {
            return false;
        }
    }
    break;
    default:
        return BufferItem::setData(column, value, role);
    }
}


void QueryBufferItem::setBufferName(const QString &name)
{
    BufferItem::setBufferName(name);
    NetworkId netId = data(0, NetworkModel::NetworkIdRole).value<NetworkId>();
    const Network *net = Client::network(netId);
    if (net)
        setIrcUser(net->ircUser(name));
}


QString QueryBufferItem::toolTip(int column) const
{
    // pretty much code duplication of IrcUserItem::toolTip() but inheritance won't solve this...
    Q_UNUSED(column);
    QStringList toolTip;

    toolTip.append(tr("<b>Query with %1</b>").arg(bufferName()));

    if (_ircUser) {
        if (_ircUser->userModes() != "") toolTip[0].append(QString(" (+%1)").arg(_ircUser->userModes()));
        if (_ircUser->isAway()) {
            toolTip[0].append(QString(" (away%1)").arg(!_ircUser->awayMessage().isEmpty() ? (QString(" ") + _ircUser->awayMessage()) : QString()));
        }
        if (!_ircUser->realName().isEmpty()) toolTip.append(_ircUser->realName());
        if (!_ircUser->ircOperator().isEmpty()) toolTip.append(QString("%1 %2").arg(_ircUser->nick()).arg(_ircUser->ircOperator()));
        if (!_ircUser->suserHost().isEmpty()) toolTip.append(_ircUser->suserHost());
        if (!_ircUser->whoisServiceReply().isEmpty()) toolTip.append(_ircUser->whoisServiceReply());

        toolTip.append(_ircUser->hostmask().remove(0, _ircUser->hostmask().indexOf("!")+1));

        if (_ircUser->idleTime().isValid()) {
            QDateTime now = QDateTime::currentDateTime();
            QDateTime idle = _ircUser->idleTime();
            int idleTime = idle.secsTo(now);
            toolTip.append(tr("idling since %1").arg(secondsToString(idleTime)));
        }
        if (_ircUser->loginTime().isValid()) {
            toolTip.append(tr("login time: %1").arg(_ircUser->loginTime().toString()));
        }

        if (!_ircUser->server().isEmpty()) toolTip.append(tr("server: %1").arg(_ircUser->server()));
    }

    return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}


void QueryBufferItem::setIrcUser(IrcUser *ircUser)
{
    if (_ircUser == ircUser)
        return;

    if (_ircUser) {
        disconnect(_ircUser, 0, this, 0);
    }

    if (ircUser) {
        connect(ircUser, SIGNAL(quited()), this, SLOT(removeIrcUser()));
        connect(ircUser, SIGNAL(awaySet(bool)), this, SIGNAL(dataChanged()));
    }

    _ircUser = ircUser;
    emit dataChanged();
}


void QueryBufferItem::removeIrcUser()
{
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
}


QVariant ChannelBufferItem::data(int column, int role) const
{
    switch (role) {
    case NetworkModel::IrcChannelRole:
        return QVariant::fromValue<QObject *>(_ircChannel);
    default:
        return BufferItem::data(column, role);
    }
}


QString ChannelBufferItem::toolTip(int column) const
{
    Q_UNUSED(column);
    QStringList toolTip;

    toolTip.append(tr("<b>Channel %1</b>").arg(Qt::escape(bufferName())));
    if (isActive()) {
        //TODO: add channel modes
        toolTip.append(tr("<b>Users:</b> %1").arg(nickCount()));
        if (_ircChannel) {
            QString channelMode = _ircChannel->channelModeString(); // channelModeString is compiled on the fly -> thus cache the result
            if (!channelMode.isEmpty())
                toolTip.append(tr("<b>Mode:</b> %1").arg(channelMode));
        }

        ItemViewSettings s;
        bool showTopic = s.displayTopicInTooltip();
        if (showTopic) {
            QString _topic = topic();
            if (_topic != "") {
                _topic = stripFormatCodes(_topic);
                _topic = Qt::escape(_topic);
                toolTip.append(QString("<font size='-2'>&nbsp;</font>"));
                toolTip.append(tr("<b>Topic:</b> %1").arg(_topic));
            }
        }
    }
    else {
        toolTip.append(tr("Not active <br /> Double-click to join"));
    }

    return tr("<p> %1 </p>").arg(toolTip.join("<br />"));
}


void ChannelBufferItem::attachIrcChannel(IrcChannel *ircChannel)
{
    Q_ASSERT(!_ircChannel && ircChannel);

    _ircChannel = ircChannel;

    connect(ircChannel, SIGNAL(topicSet(QString)),
        this, SLOT(setTopic(QString)));
    connect(ircChannel, SIGNAL(ircUsersJoined(QList<IrcUser *> )),
        this, SLOT(join(QList<IrcUser *> )));
    connect(ircChannel, SIGNAL(ircUserParted(IrcUser *)),
        this, SLOT(part(IrcUser *)));
    connect(ircChannel, SIGNAL(parted()),
        this, SLOT(ircChannelParted()));
    connect(ircChannel, SIGNAL(ircUserModesSet(IrcUser *, QString)),
        this, SLOT(userModeChanged(IrcUser *)));
    connect(ircChannel, SIGNAL(ircUserModeAdded(IrcUser *, QString)),
        this, SLOT(userModeChanged(IrcUser *)));
    connect(ircChannel, SIGNAL(ircUserModeRemoved(IrcUser *, QString)),
        this, SLOT(userModeChanged(IrcUser *)));

    if (!ircChannel->ircUsers().isEmpty())
        join(ircChannel->ircUsers());

    emit dataChanged();
}


void ChannelBufferItem::ircChannelParted()
{
    Q_CHECK_PTR(_ircChannel);
    disconnect(_ircChannel, 0, this, 0);
    _ircChannel = 0;
    emit dataChanged();
    removeAllChilds();
}


void ChannelBufferItem::join(const QList<IrcUser *> &ircUsers)
{
    addUsersToCategory(ircUsers);
    emit dataChanged(2);
}


UserCategoryItem *ChannelBufferItem::findCategoryItem(int categoryId)
{
    UserCategoryItem *categoryItem = 0;

    for (int i = 0; i < childCount(); i++) {
        categoryItem = qobject_cast<UserCategoryItem *>(child(i));
        if (!categoryItem)
            continue;
        if (categoryItem->categoryId() == categoryId)
            return categoryItem;
    }
    return 0;
}


void ChannelBufferItem::addUserToCategory(IrcUser *ircUser)
{
    addUsersToCategory(QList<IrcUser *>() << ircUser);
}


void ChannelBufferItem::addUsersToCategory(const QList<IrcUser *> &ircUsers)
{
    Q_ASSERT(_ircChannel);

    QHash<UserCategoryItem *, QList<IrcUser *> > categories;

    int categoryId = -1;
    UserCategoryItem *categoryItem = 0;

    foreach(IrcUser *ircUser, ircUsers) {
        categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
        categoryItem = findCategoryItem(categoryId);
        if (!categoryItem) {
            categoryItem = new UserCategoryItem(categoryId, this);
            categories[categoryItem] = QList<IrcUser *>();
            newChild(categoryItem);
        }
        categories[categoryItem] << ircUser;
    }

    QHash<UserCategoryItem *, QList<IrcUser *> >::const_iterator catIter = categories.constBegin();
    while (catIter != categories.constEnd()) {
        catIter.key()->addUsers(catIter.value());
        catIter++;
    }
}


void ChannelBufferItem::part(IrcUser *ircUser)
{
    if (!ircUser) {
        qWarning() << bufferName() << "ChannelBufferItem::part(): unknown User" << ircUser;
        return;
    }

    disconnect(ircUser, 0, this, 0);
    removeUserFromCategory(ircUser);
    emit dataChanged(2);
}


void ChannelBufferItem::removeUserFromCategory(IrcUser *ircUser)
{
    if (!_ircChannel) {
        // If we parted the channel there might still be some ircUsers connected.
        // in that case we just ignore the call
        Q_ASSERT(childCount() == 0);
        return;
    }

    UserCategoryItem *categoryItem = 0;
    for (int i = 0; i < childCount(); i++) {
        categoryItem = qobject_cast<UserCategoryItem *>(child(i));
        if (categoryItem->removeUser(ircUser)) {
            if (categoryItem->childCount() == 0)
                removeChild(i);
            break;
        }
    }
}


void ChannelBufferItem::userModeChanged(IrcUser *ircUser)
{
    Q_ASSERT(_ircChannel);

    int categoryId = UserCategoryItem::categoryFromModes(_ircChannel->userModes(ircUser));
    UserCategoryItem *categoryItem = findCategoryItem(categoryId);

    if (categoryItem) {
        if (categoryItem->findIrcUser(ircUser)) {
            return; // already in the right category;
        }
    }
    else {
        categoryItem = new UserCategoryItem(categoryId, this);
        newChild(categoryItem);
    }

    // find the item that needs reparenting
    IrcUserItem *ircUserItem = 0;
    for (int i = 0; i < childCount(); i++) {
        UserCategoryItem *oldCategoryItem = qobject_cast<UserCategoryItem *>(child(i));
        Q_ASSERT(oldCategoryItem);
        IrcUserItem *userItem = oldCategoryItem->findIrcUser(ircUser);
        if (userItem) {
            ircUserItem = userItem;
            break;
        }
    }

    if (!ircUserItem) {
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
    setFlags(Qt::ItemIsEnabled);
    setTreeItemFlags(AbstractTreeItem::DeleteOnLastChildRemoved);
    setObjectName(parent->data(0, Qt::DisplayRole).toString() + "/" + QString::number(category));
}


// caching this makes no sense, since we display the user number dynamically
QString UserCategoryItem::categoryName() const
{
    int n = childCount();
    switch (_category) {
    case 0:
        return tr("%n Owner(s)", 0, n);
    case 1:
        return tr("%n Admin(s)", 0, n);
    case 2:
        return tr("%n Operator(s)", 0, n);
    case 3:
        return tr("%n Half-Op(s)", 0, n);
    case 4:
        return tr("%n Voiced", 0, n);
    default:
        return tr("%n User(s)", 0, n);
    }
}


IrcUserItem *UserCategoryItem::findIrcUser(IrcUser *ircUser)
{
    IrcUserItem *userItem = 0;

    for (int i = 0; i < childCount(); i++) {
        userItem = qobject_cast<IrcUserItem *>(child(i));
        if (!userItem)
            continue;
        if (userItem->ircUser() == ircUser)
            return userItem;
    }
    return 0;
}


void UserCategoryItem::addUsers(const QList<IrcUser *> &ircUsers)
{
    QList<AbstractTreeItem *> userItems;
    foreach(IrcUser *ircUser, ircUsers)
    userItems << new IrcUserItem(ircUser, this);
    newChilds(userItems);
    emit dataChanged(0);
}


bool UserCategoryItem::removeUser(IrcUser *ircUser)
{
    IrcUserItem *userItem = findIrcUser(ircUser);
    bool success = (bool)userItem;
    if (success) {
        removeChild(userItem);
        emit dataChanged(0);
    }
    return success;
}


int UserCategoryItem::categoryFromModes(const QString &modes)
{
    for (int i = 0; i < categories.count(); i++) {
        if (modes.contains(categories[i]))
            return i;
    }
    return categories.count();
}


QVariant UserCategoryItem::data(int column, int role) const
{
    switch (role) {
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
    _ircUser(ircUser)
{
    setObjectName(ircUser->nick());
    connect(ircUser, SIGNAL(quited()), this, SLOT(ircUserQuited()));
    connect(ircUser, SIGNAL(nickSet(QString)), this, SIGNAL(dataChanged()));
    connect(ircUser, SIGNAL(awaySet(bool)), this, SIGNAL(dataChanged()));
}


QVariant IrcUserItem::data(int column, int role) const
{
    switch (role) {
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
    case NetworkModel::IrcChannelRole:
        return parent()->data(column, role);
    case NetworkModel::IrcUserRole:
        return QVariant::fromValue<QObject *>(_ircUser.data());
    case NetworkModel::UserAwayRole:
        return (bool)_ircUser ? _ircUser->isAway() : false;
    default:
        return PropertyMapItem::data(column, role);
    }
}


QString IrcUserItem::toolTip(int column) const
{
    Q_UNUSED(column);
    QStringList toolTip(QString("<b>%1</b>").arg(nickName()));
    if (_ircUser->userModes() != "") toolTip[0].append(QString(" (%1)").arg(_ircUser->userModes()));
    if (_ircUser->isAway()) {
        toolTip[0].append(tr(" is away"));
        if (!_ircUser->awayMessage().isEmpty())
            toolTip[0].append(QString(" (%1)").arg(_ircUser->awayMessage()));
    }
    if (!_ircUser->realName().isEmpty()) toolTip.append(_ircUser->realName());
    if (!_ircUser->ircOperator().isEmpty()) toolTip.append(QString("%1 %2").arg(nickName()).arg(_ircUser->ircOperator()));
    if (!_ircUser->suserHost().isEmpty()) toolTip.append(_ircUser->suserHost());
    if (!_ircUser->whoisServiceReply().isEmpty()) toolTip.append(_ircUser->whoisServiceReply());

    toolTip.append(_ircUser->hostmask().remove(0, _ircUser->hostmask().indexOf("!")+1));

    if (_ircUser->idleTime().isValid()) {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime idle = _ircUser->idleTime();
        int idleTime = idle.secsTo(now);
        toolTip.append(tr("idling since %1").arg(secondsToString(idleTime)));
    }
    if (_ircUser->loginTime().isValid()) {
        toolTip.append(tr("login time: %1").arg(_ircUser->loginTime().toString()));
    }

    if (!_ircUser->server().isEmpty()) toolTip.append(tr("server: %1").arg(_ircUser->server()));

    return QString("<p> %1 </p>").arg(toolTip.join("<br />"));
}


/*****************************************
 * NetworkModel
 *****************************************/
NetworkModel::NetworkModel(QObject *parent)
    : TreeModel(NetworkModel::defaultHeader(), parent)
{
    connect(this, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
        this, SLOT(checkForNewBuffers(const QModelIndex &, int, int)));
    connect(this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
        this, SLOT(checkForRemovedBuffers(const QModelIndex &, int, int)));

    BufferSettings defaultSettings;
    defaultSettings.notify("UserNoticesTarget", this, SLOT(messageRedirectionSettingsChanged()));
    defaultSettings.notify("ServerNoticesTarget", this, SLOT(messageRedirectionSettingsChanged()));
    defaultSettings.notify("ErrorMsgsTarget", this, SLOT(messageRedirectionSettingsChanged()));
    messageRedirectionSettingsChanged();
}


QList<QVariant> NetworkModel::defaultHeader()
{
    QList<QVariant> data;
    data << tr("Chat") << tr("Topic") << tr("Nick Count");
    return data;
}


bool NetworkModel::isBufferIndex(const QModelIndex &index) const
{
    return index.data(NetworkModel::ItemTypeRole) == NetworkModel::BufferItemType;
}


int NetworkModel::networkRow(NetworkId networkId) const
{
    NetworkItem *netItem = 0;
    for (int i = 0; i < rootItem->childCount(); i++) {
        netItem = qobject_cast<NetworkItem *>(rootItem->child(i));
        if (!netItem)
            continue;
        if (netItem->networkId() == networkId)
            return i;
    }
    return -1;
}


QModelIndex NetworkModel::networkIndex(NetworkId networkId)
{
    int netRow = networkRow(networkId);
    if (netRow == -1)
        return QModelIndex();
    else
        return indexByItem(qobject_cast<NetworkItem *>(rootItem->child(netRow)));
}


NetworkItem *NetworkModel::findNetworkItem(NetworkId networkId) const
{
    int netRow = networkRow(networkId);
    if (netRow == -1)
        return 0;
    else
        return qobject_cast<NetworkItem *>(rootItem->child(netRow));
}


NetworkItem *NetworkModel::networkItem(NetworkId networkId)
{
    NetworkItem *netItem = findNetworkItem(networkId);

    if (netItem == 0) {
        netItem = new NetworkItem(networkId, rootItem);
        rootItem->newChild(netItem);
    }
    return netItem;
}


void NetworkModel::networkRemoved(const NetworkId &networkId)
{
    int netRow = networkRow(networkId);
    if (netRow != -1) {
        rootItem->removeChild(netRow);
    }
}


QModelIndex NetworkModel::bufferIndex(BufferId bufferId)
{
    if (!_bufferItemCache.contains(bufferId))
        return QModelIndex();

    return indexByItem(_bufferItemCache[bufferId]);
}


BufferItem *NetworkModel::findBufferItem(BufferId bufferId) const
{
    if (_bufferItemCache.contains(bufferId))
        return _bufferItemCache[bufferId];
    else
        return 0;
}


BufferItem *NetworkModel::bufferItem(const BufferInfo &bufferInfo)
{
    if (_bufferItemCache.contains(bufferInfo.bufferId()))
        return _bufferItemCache[bufferInfo.bufferId()];

    NetworkItem *netItem = networkItem(bufferInfo.networkId());
    return netItem->bufferItem(bufferInfo);
}


QStringList NetworkModel::mimeTypes() const
{
    // mimetypes we accept for drops
    QStringList types;
    // comma separated list of colon separated pairs of networkid and bufferid
    // example: 0:1,0:2,1:4
    types << "application/Quassel/BufferItemList";
    return types;
}


bool NetworkModel::mimeContainsBufferList(const QMimeData *mimeData)
{
    return mimeData->hasFormat("application/Quassel/BufferItemList");
}


QList<QPair<NetworkId, BufferId> > NetworkModel::mimeDataToBufferList(const QMimeData *mimeData)
{
    QList<QPair<NetworkId, BufferId> > bufferList;

    if (!mimeContainsBufferList(mimeData))
        return bufferList;

    QStringList rawBufferList = QString::fromAscii(mimeData->data("application/Quassel/BufferItemList")).split(",");
    NetworkId networkId;
    BufferId bufferUid;
    foreach(QString rawBuffer, rawBufferList) {
        if (!rawBuffer.contains(":"))
            continue;
        networkId = rawBuffer.section(":", 0, 0).toInt();
        bufferUid = rawBuffer.section(":", 1, 1).toInt();
        bufferList.append(qMakePair(networkId, bufferUid));
    }
    return bufferList;
}


QMimeData *NetworkModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();

    QStringList bufferlist;
    QString netid, uid, bufferid;
    foreach(QModelIndex index, indexes) {
        netid = QString::number(index.data(NetworkIdRole).value<NetworkId>().toInt());
        uid = QString::number(index.data(BufferIdRole).value<BufferId>().toInt());
        bufferid = QString("%1:%2").arg(netid).arg(uid);
        if (!bufferlist.contains(bufferid))
            bufferlist << bufferid;
    }

    mimeData->setData("application/Quassel/BufferItemList", bufferlist.join(",").toAscii());

    return mimeData;
}


void NetworkModel::attachNetwork(Network *net)
{
    NetworkItem *netItem = networkItem(net->networkId());
    netItem->attachNetwork(net);
}


void NetworkModel::bufferUpdated(BufferInfo bufferInfo)
{
    BufferItem *bufItem = bufferItem(bufferInfo);
    QModelIndex itemindex = indexByItem(bufItem);
    emit dataChanged(itemindex, itemindex);
}


void NetworkModel::removeBuffer(BufferId bufferId)
{
    BufferItem *buffItem = findBufferItem(bufferId);
    if (!buffItem)
        return;

    buffItem->parent()->removeChild(buffItem);
}


MsgId NetworkModel::lastSeenMsgId(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return MsgId();

    return _bufferItemCache[bufferId]->lastSeenMsgId();
}


MsgId NetworkModel::markerLineMsgId(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return MsgId();

    return _bufferItemCache[bufferId]->markerLineMsgId();
}


// FIXME we always seem to use this (expensive) non-const version
MsgId NetworkModel::lastSeenMsgId(const BufferId &bufferId)
{
    BufferItem *bufferItem = findBufferItem(bufferId);
    if (!bufferItem) {
        qDebug() << "NetworkModel::lastSeenMsgId(): buffer is unknown:" << bufferId;
        Client::purgeKnownBufferIds();
        return MsgId();
    }
    return bufferItem->lastSeenMsgId();
}


void NetworkModel::setLastSeenMsgId(const BufferId &bufferId, const MsgId &msgId)
{
    BufferItem *bufferItem = findBufferItem(bufferId);
    if (!bufferItem) {
        qDebug() << "NetworkModel::setLastSeenMsgId(): buffer is unknown:" << bufferId;
        Client::purgeKnownBufferIds();
        return;
    }
    bufferItem->setLastSeenMsgId(msgId);
    emit lastSeenMsgSet(bufferId, msgId);
}


void NetworkModel::setMarkerLineMsgId(const BufferId &bufferId, const MsgId &msgId)
{
    BufferItem *bufferItem = findBufferItem(bufferId);
    if (!bufferItem) {
        qDebug() << "NetworkModel::setMarkerLineMsgId(): buffer is unknown:" << bufferId;
        Client::purgeKnownBufferIds();
        return;
    }
    bufferItem->setMarkerLineMsgId(msgId);
    emit markerLineSet(bufferId, msgId);
}


void NetworkModel::updateBufferActivity(Message &msg)
{
    int redirectionTarget = 0;
    switch (msg.type()) {
    case Message::Notice:
        if (bufferType(msg.bufferId()) != BufferInfo::ChannelBuffer) {
            msg.setFlags(msg.flags() | Message::Redirected);
            if (msg.flags() & Message::ServerMsg) {
                // server notice
                redirectionTarget = _serverNoticesTarget;
            }
            else {
                redirectionTarget = _userNoticesTarget;
            }
        }
        break;
    case Message::Error:
        msg.setFlags(msg.flags() | Message::Redirected);
        redirectionTarget = _errorMsgsTarget;
        break;
    // Update IrcUser's last activity
    case Message::Plain:
    case Message::Action:
        if (bufferType(msg.bufferId()) == BufferInfo::ChannelBuffer) {
            const Network *net = Client::network(msg.bufferInfo().networkId());
            IrcUser *user = net ? net->ircUser(nickFromMask(msg.sender())) : 0;
            if (user)
                user->setLastChannelActivity(msg.bufferId(), msg.timestamp());
        }
        break;
    default:
        break;
    }

    if (msg.flags() & Message::Redirected) {
        if (redirectionTarget & BufferSettings::DefaultBuffer)
            updateBufferActivity(bufferItem(msg.bufferInfo()), msg);

        if (redirectionTarget & BufferSettings::StatusBuffer) {
            const NetworkItem *netItem = findNetworkItem(msg.bufferInfo().networkId());
            if (netItem) {
                updateBufferActivity(netItem->statusBufferItem(), msg);
            }
        }
    }
    else {
        updateBufferActivity(bufferItem(msg.bufferInfo()), msg);
    }
}


void NetworkModel::updateBufferActivity(BufferItem *bufferItem, const Message &msg)
{
    if (!bufferItem)
        return;

    bufferItem->updateActivityLevel(msg);
    if (bufferItem->isCurrentBuffer())
        emit requestSetLastSeenMsg(bufferItem->bufferId(), msg.msgId());
}


void NetworkModel::setBufferActivity(const BufferId &bufferId, BufferInfo::ActivityLevel level)
{
    BufferItem *bufferItem = findBufferItem(bufferId);
    if (!bufferItem) {
        qDebug() << "NetworkModel::setBufferActivity(): buffer is unknown:" << bufferId;
        return;
    }
    bufferItem->setActivityLevel(level);
}


void NetworkModel::clearBufferActivity(const BufferId &bufferId)
{
    BufferItem *bufferItem = findBufferItem(bufferId);
    if (!bufferItem) {
        qDebug() << "NetworkModel::clearBufferActivity(): buffer is unknown:" << bufferId;
        return;
    }
    bufferItem->clearActivityLevel();
}


const Network *NetworkModel::networkByIndex(const QModelIndex &index) const
{
    QVariant netVariant = index.data(NetworkIdRole);
    if (!netVariant.isValid())
        return 0;

    NetworkId networkId = netVariant.value<NetworkId>();
    return Client::network(networkId);
}


void NetworkModel::checkForRemovedBuffers(const QModelIndex &parent, int start, int end)
{
    if (parent.data(ItemTypeRole) != NetworkItemType)
        return;

    for (int row = start; row <= end; row++) {
        _bufferItemCache.remove(parent.child(row, 0).data(BufferIdRole).value<BufferId>());
    }
}


void NetworkModel::checkForNewBuffers(const QModelIndex &parent, int start, int end)
{
    if (parent.data(ItemTypeRole) != NetworkItemType)
        return;

    for (int row = start; row <= end; row++) {
        QModelIndex child = parent.child(row, 0);
        _bufferItemCache[child.data(BufferIdRole).value < BufferId > ()] = static_cast<BufferItem *>(child.internalPointer());
    }
}


QString NetworkModel::bufferName(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return QString();

    return _bufferItemCache[bufferId]->bufferName();
}


BufferInfo::Type NetworkModel::bufferType(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return BufferInfo::InvalidBuffer;

    return _bufferItemCache[bufferId]->bufferType();
}


BufferInfo NetworkModel::bufferInfo(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return BufferInfo();

    return _bufferItemCache[bufferId]->bufferInfo();
}


NetworkId NetworkModel::networkId(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return NetworkId();

    NetworkItem *netItem = qobject_cast<NetworkItem *>(_bufferItemCache[bufferId]->parent());
    if (netItem)
        return netItem->networkId();
    else
        return NetworkId();
}


QString NetworkModel::networkName(BufferId bufferId) const
{
    if (!_bufferItemCache.contains(bufferId))
        return QString();

    NetworkItem *netItem = qobject_cast<NetworkItem *>(_bufferItemCache[bufferId]->parent());
    if (netItem)
        return netItem->networkName();
    else
        return QString();
}


BufferId NetworkModel::bufferId(NetworkId networkId, const QString &bufferName, Qt::CaseSensitivity cs) const
{
    const NetworkItem *netItem = findNetworkItem(networkId);
    if (!netItem)
        return BufferId();

    for (int i = 0; i < netItem->childCount(); i++) {
        BufferItem *bufferItem = qobject_cast<BufferItem *>(netItem->child(i));
        if (bufferItem && !bufferItem->bufferName().compare(bufferName, cs))
            return bufferItem->bufferId();
    }
    return BufferId();
}


void NetworkModel::sortBufferIds(QList<BufferId> &bufferIds) const
{
    QList<BufferItem *> bufferItems;
    foreach(BufferId bufferId, bufferIds) {
        if (_bufferItemCache.contains(bufferId))
            bufferItems << _bufferItemCache[bufferId];
    }

    qSort(bufferItems.begin(), bufferItems.end(), bufferItemLessThan);

    bufferIds.clear();
    foreach(BufferItem *bufferItem, bufferItems) {
        bufferIds << bufferItem->bufferId();
    }
}


QList<BufferId> NetworkModel::allBufferIdsSorted() const
{
    QList<BufferId> bufferIds = allBufferIds();
    sortBufferIds(bufferIds);
    return bufferIds;
}


bool NetworkModel::bufferItemLessThan(const BufferItem *left, const BufferItem *right)
{
    int leftType = left->bufferType();
    int rightType = right->bufferType();

    if (leftType != rightType)
        return leftType < rightType;
    else
        return QString::compare(left->bufferName(), right->bufferName(), Qt::CaseInsensitive) < 0;
}


void NetworkModel::messageRedirectionSettingsChanged()
{
    BufferSettings bufferSettings;

    _userNoticesTarget = bufferSettings.userNoticesTarget();
    _serverNoticesTarget = bufferSettings.serverNoticesTarget();
    _errorMsgsTarget = bufferSettings.errorMsgsTarget();
}

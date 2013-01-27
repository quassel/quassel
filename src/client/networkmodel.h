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

#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include "bufferinfo.h"
#include "clientsettings.h"
#include "message.h"
#include "network.h"
#include "treemodel.h"

class BufferItem;
class StatusBufferItem;

/*****************************************
 *  Network Items
 *****************************************/
class NetworkItem : public PropertyMapItem
{
    Q_OBJECT
    Q_PROPERTY(QString networkName READ networkName)
    Q_PROPERTY(QString currentServer READ currentServer)
    Q_PROPERTY(int nickCount READ nickCount)

public :
        NetworkItem(const NetworkId &netid, AbstractTreeItem *parent = 0);

    virtual QVariant data(int column, int row) const;

    inline bool isActive() const { return (bool)_network ? _network->isConnected() : false; }

    inline const NetworkId &networkId() const { return _networkId; }
    inline QString networkName() const { return (bool)_network ? _network->networkName() : QString(); }
    inline QString currentServer() const { return (bool)_network ? _network->currentServer() : QString(); }
    inline int nickCount() const { return (bool)_network ? _network->ircUsers().count() : 0; }

    virtual QString toolTip(int column) const;

    BufferItem *findBufferItem(BufferId bufferId);
    inline BufferItem *findBufferItem(const BufferInfo &bufferInfo) { return findBufferItem(bufferInfo.bufferId()); }
    BufferItem *bufferItem(const BufferInfo &bufferInfo);
    inline StatusBufferItem *statusBufferItem() const { return _statusBufferItem; }

public slots:
    void setNetworkName(const QString &networkName);
    void setCurrentServer(const QString &serverName);

    void attachNetwork(Network *network);
    void attachIrcChannel(IrcChannel *channel);
    void attachIrcUser(IrcUser *ircUser);

signals:
    void networkDataChanged(int column = -1);

private slots:
    void onBeginRemoveChilds(int start, int end);

private:
    NetworkId _networkId;
    StatusBufferItem *_statusBufferItem;

    QPointer<Network> _network;
};


/*****************************************
 *  Fancy Buffer Items
 *****************************************/
class BufferItem : public PropertyMapItem
{
    Q_OBJECT
    Q_PROPERTY(QString bufferName READ bufferName WRITE setBufferName)
    Q_PROPERTY(QString topic READ topic)
    Q_PROPERTY(int nickCount READ nickCount)

public :
        BufferItem(const BufferInfo &bufferInfo, AbstractTreeItem *parent = 0);

    inline const BufferInfo &bufferInfo() const { return _bufferInfo; }
    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role);

    inline BufferId bufferId() const { return _bufferInfo.bufferId(); }
    inline BufferInfo::Type bufferType() const { return _bufferInfo.type(); }

    virtual void setBufferName(const QString &name);
    virtual inline QString bufferName() const { return _bufferInfo.bufferName(); }
    virtual inline QString topic() const { return QString(); }
    virtual inline int nickCount() const { return 0; }

    virtual inline bool isActive() const { return qobject_cast<NetworkItem *>(parent())->isActive(); }

    inline MsgId lastSeenMsgId() const { return _lastSeenMsgId; }
    inline MsgId markerLineMsgId() const { return _markerLineMsgId; }
    void setLastSeenMsgId(MsgId msgId);
    void setMarkerLineMsgId(MsgId msgId);

    inline BufferInfo::ActivityLevel activityLevel() const { return _activity; }
    void setActivityLevel(BufferInfo::ActivityLevel level);
    void clearActivityLevel();
    void updateActivityLevel(const Message &msg);

    inline const MsgId &firstUnreadMsgId() const { return _firstUnreadMsgId; }

    bool isCurrentBuffer() const;
    virtual QString toolTip(int column) const;

public slots:
    virtual inline void setTopic(const QString &) { emit dataChanged(1); }

private:
    BufferInfo _bufferInfo;
    BufferInfo::ActivityLevel _activity;
    MsgId _lastSeenMsgId;
    MsgId _markerLineMsgId;
    MsgId _firstUnreadMsgId;
};


/*****************************************
*  StatusBufferItem
*****************************************/
class StatusBufferItem : public BufferItem
{
    Q_OBJECT

public:
    StatusBufferItem(const BufferInfo &bufferInfo, NetworkItem *parent);

    virtual QString toolTip(int column) const;
    virtual inline QString bufferName() const { return tr("Status Buffer"); }
};


/*****************************************
*  QueryBufferItem
*****************************************/
class QueryBufferItem : public BufferItem
{
    Q_OBJECT

public:
    QueryBufferItem(const BufferInfo &bufferInfo, NetworkItem *parent);

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role);

    virtual inline bool isActive() const { return (bool)_ircUser; }
    virtual QString toolTip(int column) const;

    virtual void setBufferName(const QString &name);

public slots:
    void setIrcUser(IrcUser *ircUser);
    void removeIrcUser();

private:
    IrcUser *_ircUser;
};


/*****************************************
*  ChannelBufferItem
*****************************************/
class UserCategoryItem;

class ChannelBufferItem : public BufferItem
{
    Q_OBJECT

public:
    ChannelBufferItem(const BufferInfo &bufferInfo, AbstractTreeItem *parent);

    virtual QVariant data(int column, int role) const;
    virtual inline bool isActive() const { return (bool)_ircChannel; }
    virtual QString toolTip(int column) const;

    virtual inline QString topic() const { return (bool)_ircChannel ? _ircChannel->topic() : QString(); }
    virtual inline int nickCount() const { return (bool)_ircChannel ? _ircChannel->ircUsers().count() : 0; }

    void attachIrcChannel(IrcChannel *ircChannel);

public slots:
    void join(const QList<IrcUser *> &ircUsers);
    void part(IrcUser *ircUser);

    UserCategoryItem *findCategoryItem(int categoryId);
    void addUserToCategory(IrcUser *ircUser);
    void addUsersToCategory(const QList<IrcUser *> &ircUser);
    void removeUserFromCategory(IrcUser *ircUser);
    void userModeChanged(IrcUser *ircUser);

private slots:
    void ircChannelParted();

private:
    IrcChannel *_ircChannel;
};


/*****************************************
*  User Category Items (like @vh etc.)
*****************************************/
class IrcUserItem;
class UserCategoryItem : public PropertyMapItem
{
    Q_OBJECT
    Q_PROPERTY(QString categoryName READ categoryName)

public :
        UserCategoryItem(int category, AbstractTreeItem *parent);

    QString categoryName() const;
    inline int categoryId() const { return _category; }
    virtual QVariant data(int column, int role) const;

    IrcUserItem *findIrcUser(IrcUser *ircUser);
    void addUsers(const QList<IrcUser *> &ircUser);
    bool removeUser(IrcUser *ircUser);

    static int categoryFromModes(const QString &modes);

private:
    int _category;

    static const QList<QChar> categories;
};


/*****************************************
*  Irc User Items
*****************************************/
class IrcUserItem : public PropertyMapItem
{
    Q_OBJECT
    Q_PROPERTY(QString nickName READ nickName)

public :
        IrcUserItem(IrcUser *ircUser, AbstractTreeItem *parent);

    inline QString nickName() const { return _ircUser ? _ircUser->nick() : QString(); }
    inline bool isActive() const { return _ircUser ? !_ircUser->isAway() : false; }

    inline IrcUser *ircUser() { return _ircUser; }
    virtual QVariant data(int column, int role) const;
    virtual QString toolTip(int column) const;

private slots:
    inline void ircUserQuited() { parent()->removeChild(this); }

private:
    QPointer<IrcUser> _ircUser;
};


/*****************************************
 * NetworkModel
 *****************************************/
class NetworkModel : public TreeModel
{
    Q_OBJECT

public:
    enum Role {
        BufferTypeRole = TreeModel::UserRole,
        ItemActiveRole,
        BufferActivityRole,
        BufferIdRole,
        NetworkIdRole,
        BufferInfoRole,
        ItemTypeRole,
        UserAwayRole,
        IrcUserRole,
        IrcChannelRole,
        BufferFirstUnreadMsgIdRole,
        MarkerLineMsgIdRole,
    };

    enum ItemType {
        NetworkItemType = 0x01,
        BufferItemType = 0x02,
        UserCategoryItemType = 0x04,
        IrcUserItemType = 0x08
    };
    Q_DECLARE_FLAGS(ItemTypes, ItemType)

    NetworkModel(QObject *parent = 0);
    static QList<QVariant> defaultHeader();

    static bool mimeContainsBufferList(const QMimeData *mimeData);
    static QList<QPair<NetworkId, BufferId> > mimeDataToBufferList(const QMimeData *mimeData);

    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &) const;

    void attachNetwork(Network *network);

    bool isBufferIndex(const QModelIndex &) const;
    //Buffer *getBufferByIndex(const QModelIndex &) const;
    QModelIndex networkIndex(NetworkId networkId);
    QModelIndex bufferIndex(BufferId bufferId);

    const Network *networkByIndex(const QModelIndex &index) const;

    BufferInfo::ActivityLevel bufferActivity(const BufferInfo &buffer) const;

    //! Finds a buffer with a given name in a given network
    /** This performs a linear search through all BufferItems, hence it is expensive.
     *  @param networkId  The network which we search in
     *  @param bufferName The bufferName we look for
     *  @return The id of the buffer if found, an invalid one else
     */
    BufferId bufferId(NetworkId networkId, const QString &bufferName, Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    QString bufferName(BufferId bufferId) const;
    BufferInfo::Type bufferType(BufferId bufferId) const;
    BufferInfo bufferInfo(BufferId bufferId) const;
    MsgId lastSeenMsgId(BufferId bufferId) const;
    MsgId markerLineMsgId(BufferId bufferId) const;
    NetworkId networkId(BufferId bufferId) const;
    QString networkName(BufferId bufferId) const;

    inline QList<BufferId> allBufferIds() const { return _bufferItemCache.keys(); }
    QList<BufferId> allBufferIdsSorted() const;
    void sortBufferIds(QList<BufferId> &bufferIds) const;

public slots:
    void bufferUpdated(BufferInfo bufferInfo);
    void removeBuffer(BufferId bufferId);
    MsgId lastSeenMsgId(const BufferId &bufferId);
    void setLastSeenMsgId(const BufferId &bufferId, const MsgId &msgId);
    void setMarkerLineMsgId(const BufferId &bufferId, const MsgId &msgId);
    void setBufferActivity(const BufferId &bufferId, BufferInfo::ActivityLevel activity);
    void clearBufferActivity(const BufferId &bufferId);
    void updateBufferActivity(Message &msg);
    void networkRemoved(const NetworkId &networkId);

signals:
    void requestSetLastSeenMsg(BufferId buffer, MsgId msg);
    void lastSeenMsgSet(BufferId buffer, MsgId msg);
    void markerLineSet(BufferId buffer, MsgId msg);

private slots:
    void checkForRemovedBuffers(const QModelIndex &parent, int start, int end);
    void checkForNewBuffers(const QModelIndex &parent, int start, int end);
    void messageRedirectionSettingsChanged();

private:
    int networkRow(NetworkId networkId) const;
    NetworkItem *findNetworkItem(NetworkId networkId) const;
    NetworkItem *networkItem(NetworkId networkId);
    inline BufferItem *findBufferItem(const BufferInfo &bufferInfo) const { return findBufferItem(bufferInfo.bufferId()); }
    BufferItem *findBufferItem(BufferId bufferId) const;
    BufferItem *bufferItem(const BufferInfo &bufferInfo);

    void updateBufferActivity(BufferItem *bufferItem, const Message &msg);

    static bool bufferItemLessThan(const BufferItem *left, const BufferItem *right);

    QHash<BufferId, BufferItem *> _bufferItemCache;

    int _userNoticesTarget;
    int _serverNoticesTarget;
    int _errorMsgsTarget;
};


Q_DECLARE_OPERATORS_FOR_FLAGS(NetworkModel::ItemTypes)

#endif // NETWORKMODEL_H

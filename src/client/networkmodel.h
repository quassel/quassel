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

#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QtCore>

#include "treemodel.h"
#include "buffer.h"

#include <QPointer>

class BufferInfo;

#include "selectionmodelsynchronizer.h"
#include "modelpropertymapper.h"
class MappedSelectionModel;
class QAbstractItemView;
class Network;
class IrcChannel;
class IrcUser;

/*****************************************
 *  Fancy Buffer Items
 *****************************************/
class BufferItem : public PropertyMapItem {
  Q_OBJECT
  Q_PROPERTY(QString bufferName READ bufferName)
  Q_PROPERTY(QString topic READ topic)
  Q_PROPERTY(int nickCount READ nickCount)

public:
  BufferItem(BufferInfo bufferInfo, AbstractTreeItem *parent = 0);

  const BufferInfo &bufferInfo() const;
  virtual quint64 id() const;
  virtual QVariant data(int column, int role) const;

  void attachIrcChannel(IrcChannel *ircChannel);

  QString bufferName() const;
  QString topic() const;
  int nickCount() const;

  enum Type {
    StatusType,
    ChannelType,
    QueryType
  };
  
  bool isStatusBuffer() const;
  Type bufferType() const;

  bool isActive() const;
  
  enum Activity {
    NoActivity = 0x00,
    OtherActivity = 0x01,
    NewMessage = 0x02,
    Highlight = 0x40
  };
  Q_DECLARE_FLAGS(ActivityLevel, Activity)

  ActivityLevel activity() const;
  void setActivity(const ActivityLevel &level);
  void addActivity(const ActivityLevel &level);

public slots:
  void setTopic(const QString &topic);
  void join(IrcUser *ircUser);
  void part(IrcUser *ircUser);

private slots:
  void ircChannelDestroyed();
  
private:
  BufferInfo _bufferInfo;
  ActivityLevel _activity;
  Type _type;

  QPointer<IrcChannel> _ircChannel;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(BufferItem::ActivityLevel)

/*****************************************
 *  Network Items
 *****************************************/
class NetworkItem : public PropertyMapItem {
  Q_OBJECT
  Q_PROPERTY(QString networkName READ networkName)
  Q_PROPERTY(QString currentServer READ currentServer)
  Q_PROPERTY(int nickCount READ nickCount)
    
public:
  NetworkItem(const uint &netid, const QString &, AbstractTreeItem *parent = 0);

  virtual QVariant data(int column, int row) const;
  virtual quint64 id() const;

  bool isActive() const;
  
  QString networkName() const;
  QString currentServer() const;
  int nickCount() const;
  
public slots:
  void setNetworkName(const QString &networkName);
  void setCurrentServer(const QString &serverName);

  void attachNetwork(Network *network);
  void attachIrcChannel(const QString &channelName);
  
private:
  uint _networkId;
  QString _networkName;

  QPointer<Network> _network;
};

/*****************************************
*  Irc User Items
*****************************************/
class IrcUserItem : public PropertyMapItem {
  Q_OBJECT
  Q_PROPERTY(QString nickName READ nickName)
    
public:
  IrcUserItem(IrcUser *ircUser, AbstractTreeItem *parent);

  QString nickName();

private slots:
  void setNick(QString newNick);
  void ircUserDestroyed();

private:
  IrcUser *_ircUser;
};


/*****************************************
 * NetworkModel
 *****************************************/
class NetworkModel : public TreeModel {
  Q_OBJECT

public:
  enum myRoles {
    BufferTypeRole = Qt::UserRole,
    ItemActiveRole,
    BufferIdRole,
    NetworkIdRole,
    ItemTypeRole
  };

  enum itemTypes {
    AbstractItemType,
    SimpleItemType,
    NetworkItemType,
    BufferItemType,
    NickItemType
  };
    
  NetworkModel(QObject *parent = 0);
  static QList<QVariant> defaultHeader();

  static bool mimeContainsBufferList(const QMimeData *mimeData);
  static QList< QPair<uint, uint> > mimeDataToBufferList(const QMimeData *mimeData);

  virtual QStringList mimeTypes() const;
  virtual QMimeData *mimeData(const QModelIndexList &) const;
  virtual bool dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &);

  void attachNetwork(Network *network);

  bool isBufferIndex(const QModelIndex &) const;
  Buffer *getBufferByIndex(const QModelIndex &) const;
  QModelIndex bufferIndex(BufferId bufferId);

public slots:
  void bufferUpdated(BufferInfo bufferInfo);
  void bufferActivity(BufferItem::ActivityLevel, BufferInfo bufferInfo);

private:
  QModelIndex networkIndex(uint networkId);
  NetworkItem *network(uint networkId);
  NetworkItem *newNetwork(uint networkId, const QString &networkName);
  
  BufferItem *buffer(BufferInfo bufferInfo);
  BufferItem *newBuffer(BufferInfo bufferInfo);

};

#endif // NETWORKMODEL_H

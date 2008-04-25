/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#ifndef MESSAGEMODEL_H_
#define MESSAGEMODEL_H_

#include <QAbstractItemModel>

class Message;
class MessageItem;
class MsgId;

class MessageModel : public QAbstractItemModel {
  Q_OBJECT

  public:
    enum MessageRoles {
      MsgIdRole = Qt::UserRole,
      BufferIdRole,
      TypeRole,
      FlagsRole,
      TimestampRole,
      UserRole
    };

    MessageModel(QObject *parent);
    virtual ~MessageModel();

    inline QModelIndex index(int row, int column, const QModelIndex &/*parent*/ = QModelIndex()) const { return createIndex(row, column); }
    inline QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    inline int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return _messageList.count(); }
    inline int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return 3; }

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    //virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    void insertMessage(const Message &);
    void insertMessages(const QList<Message> &);

  protected:
    virtual MessageItem *createMessageItem(const Message &) = 0;

  private:
    QList<MessageItem *> _messageList;

    int indexForId(MsgId);

};

class MessageItem {
      
  public:
    MessageItem(const Message &);
    virtual ~MessageItem();


    virtual QVariant data(int column, int role) const = 0;
    virtual bool setData(int column, const QVariant &value, int role) = 0;

};

#endif

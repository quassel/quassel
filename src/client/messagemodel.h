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
#include <QDateTime>

#include "message.h"
#include "types.h"

class MessageModelItem;
struct MsgId;

class MessageModel : public QAbstractItemModel {
  Q_OBJECT

public:
  enum MessageRole {
    DisplayRole = Qt::DisplayRole,
    EditRole = Qt::EditRole,
    MsgIdRole = Qt::UserRole,
    BufferIdRole,
    TypeRole,
    FlagsRole,
    TimestampRole,
    FormatRole,
    ColumnTypeRole,
    UserRole
  };

  enum ColumnType {
    TimestampColumn, SenderColumn, ContentsColumn, UserColumnType
  };

  MessageModel(QObject *parent);

  inline QModelIndex index(int row, int column, const QModelIndex &/*parent*/ = QModelIndex()) const { return createIndex(row, column); }
  inline QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
  inline int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return _messageList.count(); }
  inline int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return 3; }

  virtual QVariant data(const QModelIndex &index, int role) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

  //virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  bool insertMessage(const Message &, bool fakeMsg = false);
  void insertMessages(const QList<Message> &);

  void clear();

protected:
  virtual MessageModelItem *createMessageModelItem(const Message &) = 0;
  virtual void customEvent(QEvent *event);

private slots:
  void changeOfDay();

private:
  void insertMessageGroup(const QList<Message> &);
  int insertMessagesGracefully(const QList<Message> &); // inserts as many contiguous msgs as possible. returns numer of inserted msgs.
  int indexForId(MsgId);

  QList<MessageModelItem *> _messageList;
  QList<Message> _messageBuffer;
  QTimer _dayChangeTimer;
  QDateTime _nextDayChange;
};

// **************************************************
//  MessageModelItem
// **************************************************
class MessageModelItem {
public:
  //! Creates a MessageModelItem from a Message object.
  /** This baseclass implementation takes care of all Message data *except* the stylable strings.
   *  Subclasses need to provide Qt::DisplayRole at least, which should describe the plaintext
   *  strings without formattings (e.g. for searching purposes).
   */
  MessageModelItem(const Message &);
  inline virtual ~MessageModelItem() {}

  virtual QVariant data(int column, int role) const;
  virtual bool setData(int column, const QVariant &value, int role) = 0;

  inline const QDateTime &timeStamp() const { return _timestamp; }
  inline MsgId msgId() const { return _msgId; }
  inline BufferId bufferId() const { return _bufferId; }
  inline Message::Type msgType() const { return _type; }
  inline Message::Flags msgFlags() const { return _flags; }
  
  // For sorting
  bool operator<(const MessageModelItem &) const;
  bool operator==(const MessageModelItem &) const;
  bool operator>(const MessageModelItem &) const;
  static bool lessThan(const MessageModelItem *m1, const MessageModelItem *m2);

private:
  QDateTime _timestamp;
  MsgId _msgId;
  BufferId _bufferId;
  Message::Type _type;
  Message::Flags _flags;
};

QDebug operator<<(QDebug dbg, const MessageModelItem &msgItem);

#endif

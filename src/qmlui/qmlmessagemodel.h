/***************************************************************************
 *   Copyright (C) 2005-2011 by the Quassel Project                        *
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

#ifndef QMLMESSAGEMODEL_H_
#define QMLMESSAGEMODEL_H_

#include <QSortFilterProxyModel>

#include "messagemodel.h"
#include "qmlmessagemodelitem.h"

class QmlMessageModel : public MessageModel {
  Q_OBJECT

public:
  enum QmlMessageModelRole {
    ChatLineDataRole = MessageModel::UserRole,
    UserRole
  };

  QmlMessageModel(QObject *parent = 0);
  virtual QVariant data(const QModelIndex &index, int role) const;

  virtual inline const MessageModelItem *messageItemAt(int i) const { return &_messageList[i]; }
protected:

  virtual inline int messageCount() const { return _messageList.count(); }
  virtual inline bool messagesIsEmpty() const { return _messageList.isEmpty(); }
  virtual inline MessageModelItem *messageItemAt(int i) { return &_messageList[i]; }
  virtual inline const MessageModelItem *firstMessageItem() const { return &_messageList.first(); }
  virtual inline MessageModelItem *firstMessageItem() { return &_messageList.first(); }
  virtual inline const MessageModelItem *lastMessageItem() const { return &_messageList.last(); }
  virtual inline MessageModelItem *lastMessageItem() { return &_messageList.last(); }
  virtual inline void insertMessage__(int pos, const Message &msg) { _messageList.insert(pos, QmlMessageModelItem(msg)); }
  virtual void insertMessages__(int pos, const QList<Message> &);
  virtual inline void removeMessageAt(int i) { _messageList.removeAt(i); }
  virtual inline void removeAllMessages() { _messageList.clear(); }
  virtual Message takeMessageAt(int i);


private:
  QList<QmlMessageModelItem> _messageList;
};

#endif

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

#ifndef CHATLINEMODEL_H_
#define CHATLINEMODEL_H_

#include "messagemodel.h"

#include <QList>
#include "chatlinemodelitem.h"

class ChatLineModel : public MessageModel
{
    Q_OBJECT

public:
    enum ChatLineRole {
        WrapListRole = MessageModel::UserRole,
        MsgLabelRole,
        SelectedBackgroundRole
    };

    ChatLineModel(QObject *parent = 0);

    typedef ChatLineModelItem::Word Word;
    typedef ChatLineModelItem::WrapList WrapList;
    virtual inline const MessageModelItem *messageItemAt(int i) const { return &_messageList[i]; }
protected:
//   virtual MessageModelItem *createMessageModelItem(const Message &);

    virtual inline int messageCount() const { return _messageList.count(); }
    virtual inline bool messagesIsEmpty() const { return _messageList.isEmpty(); }
    virtual inline MessageModelItem *messageItemAt(int i) { return &_messageList[i]; }
    virtual inline const MessageModelItem *firstMessageItem() const { return &_messageList.first(); }
    virtual inline MessageModelItem *firstMessageItem() { return &_messageList.first(); }
    virtual inline const MessageModelItem *lastMessageItem() const { return &_messageList.last(); }
    virtual inline MessageModelItem *lastMessageItem() { return &_messageList.last(); }
    virtual inline void insertMessage__(int pos, const Message &msg) { _messageList.insert(pos, ChatLineModelItem(msg)); }
    virtual void insertMessages__(int pos, const QList<Message> &);
    virtual inline void removeMessageAt(int i) { _messageList.removeAt(i); }
    virtual inline void removeAllMessages() { _messageList.clear(); }
    virtual Message takeMessageAt(int i);

protected slots:
    virtual void styleChanged();

private:
    QList<ChatLineModelItem> _messageList;
};


QDataStream &operator<<(QDataStream &out, const ChatLineModel::WrapList);
QDataStream &operator>>(QDataStream &in, ChatLineModel::WrapList &);

Q_DECLARE_METATYPE(ChatLineModel::WrapList)

#endif

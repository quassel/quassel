/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <QList>

#include "chatlinemodelitem.h"
#include "messagemodel.h"

class ChatLineModel : public MessageModel
{
    Q_OBJECT

public:
    enum ChatLineRole
    {
        WrapListRole = MessageModel::UserRole,
        MsgLabelRole,
        SelectedBackgroundRole
    };

    ChatLineModel(QObject* parent = nullptr);

    using Word = ChatLineModelItem::Word;
    using WrapList = ChatLineModelItem::WrapList;
    inline const MessageModelItem* messageItemAt(int i) const override { return &_messageList[i]; }

protected:
    //   virtual MessageModelItem *createMessageModelItem(const Message &);

    inline int messageCount() const override { return _messageList.count(); }
    inline bool messagesIsEmpty() const override { return _messageList.isEmpty(); }
    inline MessageModelItem* messageItemAt(int i) override { return &_messageList[i]; }
    inline const MessageModelItem* firstMessageItem() const override { return &_messageList.first(); }
    inline MessageModelItem* firstMessageItem() override { return &_messageList.first(); }
    inline const MessageModelItem* lastMessageItem() const override { return &_messageList.last(); }
    inline MessageModelItem* lastMessageItem() override { return &_messageList.last(); }
    inline void insertMessage__(int pos, const Message& msg) override { _messageList.insert(pos, ChatLineModelItem(msg)); }
    void insertMessages__(int pos, const QList<Message>&) override;
    inline void removeMessageAt(int i) override { _messageList.removeAt(i); }
    inline void removeAllMessages() override { _messageList.clear(); }
    Message takeMessageAt(int i) override;

protected slots:
    virtual void styleChanged();

private:
    QList<ChatLineModelItem> _messageList;
};

QDataStream& operator<<(QDataStream& out, const ChatLineModel::WrapList);
QDataStream& operator>>(QDataStream& in, ChatLineModel::WrapList&);

Q_DECLARE_METATYPE(ChatLineModel::WrapList)

#endif

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

#include "chatlinemodel.h"
#include "qtui.h"
#include "qtuistyle.h"

ChatLineModel::ChatLineModel(QObject *parent)
    : MessageModel(parent)
{
    qRegisterMetaType<WrapList>("ChatLineModel::WrapList");
    qRegisterMetaTypeStreamOperators<WrapList>("ChatLineModel::WrapList");

    connect(QtUi::style(), SIGNAL(changed()), SLOT(styleChanged()));
}


// MessageModelItem *ChatLineModel::createMessageModelItem(const Message &msg) {
//   return new ChatLineModelItem(msg);
// }

void ChatLineModel::insertMessages__(int pos, const QList<Message> &messages)
{
    for (int i = 0; i < messages.count(); i++) {
        _messageList.insert(pos, ChatLineModelItem(messages[i]));
        pos++;
    }
}


Message ChatLineModel::takeMessageAt(int i)
{
    Message msg = _messageList[i].message();
    _messageList.removeAt(i);
    return msg;
}


void ChatLineModel::styleChanged()
{
    foreach(ChatLineModelItem item, _messageList) {
        item.invalidateWrapList();
    }
    emit dataChanged(index(0, 0), index(rowCount()-1, columnCount()-1));
}


QDataStream &operator<<(QDataStream &out, const ChatLineModel::WrapList wplist)
{
    out << wplist.count();
    ChatLineModel::WrapList::const_iterator it = wplist.begin();
    while (it != wplist.end()) {
        out << (*it).start << (*it).width << (*it).trailing;
        ++it;
    }
    return out;
}


QDataStream &operator>>(QDataStream &in, ChatLineModel::WrapList &wplist)
{
    quint16 cnt;
    in >> cnt;
    wplist.resize(cnt);
    for (quint16 i = 0; i < cnt; i++) {
        in >> wplist[i].start >> wplist[i].width >> wplist[i].trailing;
    }
    return in;
}

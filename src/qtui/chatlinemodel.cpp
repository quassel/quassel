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

#include "chatlinemodel.h"

#include "chatlinemodelitem.h"

ChatLineModel::ChatLineModel(QObject *parent)
  : MessageModel(parent)
{
  qRegisterMetaType<WrapList>("ChatLineModel::WrapList");
  qRegisterMetaTypeStreamOperators<WrapList>("ChatLineModel::WrapList");
}

MessageModelItem *ChatLineModel::createMessageModelItem(const Message &msg) {
  return new ChatLineModelItem(msg);
}


QDataStream &operator<<(QDataStream &out, const ChatLineModel::WrapList wplist) {
  out << wplist.count();
  ChatLineModel::WrapList::const_iterator it = wplist.begin();
  while(it != wplist.end()) {
    out << (*it).start << (*it).width << (*it).trailing;
    ++it;
  }
  return out;
}

QDataStream &operator>>(QDataStream &in, ChatLineModel::WrapList &wplist) {
  quint16 cnt;
  in >> cnt;
  wplist.resize(cnt);
  for(quint16 i = 0; i < cnt; i++) {
    in >> wplist[i].start >> wplist[i].width >> wplist[i].trailing;
  }
  return in;
}

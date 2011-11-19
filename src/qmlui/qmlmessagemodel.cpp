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

#include <QtDeclarative>

#include "qmlmessagemodel.h"
#include "qmlchatline.h"

QmlMessageModel::QmlMessageModel(QObject *parent)
  : MessageModel(parent)
{
  QmlChatLine::registerTypes();

  QHash<int, QByteArray> roles;
  roles[RenderDataRole] = "renderDataRole";
  setRoleNames(roles);
}

void QmlMessageModel::insertMessages__(int pos, const QList<Message> &messages) {
  for(int i = 0; i < messages.count(); i++) {
    _messageList.insert(pos, QmlMessageModelItem(messages[i]));
    pos++;
  }
}

Message QmlMessageModel::takeMessageAt(int i) {
  Message msg = _messageList[i].message();
  _messageList.removeAt(i);
  return msg;
}

QVariant QmlMessageModel::data(const QModelIndex &index, int role) const {
  int row = index.row();
  if(row < 0 || row >= messageCount())
    return QVariant();

  return messageItemAt(row)->data(index.column(), role);
}

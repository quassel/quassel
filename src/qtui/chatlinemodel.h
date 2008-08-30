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

#ifndef CHATLINEMODEL_H_
#define CHATLINEMODEL_H_

#include "messagemodel.h"

class ChatLineModel : public MessageModel {
  Q_OBJECT

public:
  enum ChatLineRole {
    WrapListRole = MessageModel::UserRole
  };

  ChatLineModel(QObject *parent = 0);

  /// Used to store information about words to be used for wrapping
  struct Word {
    quint16 start;
    qreal width;
    qreal trailing;
  };
  typedef QVector<Word> WrapList;

protected:
  virtual MessageModelItem *createMessageModelItem(const Message &);

};

QDataStream &operator<<(QDataStream &out, const ChatLineModel::WrapList);
QDataStream &operator>>(QDataStream &in, ChatLineModel::WrapList &);

Q_DECLARE_METATYPE(ChatLineModel::WrapList)

#endif


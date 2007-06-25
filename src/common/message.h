/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel IRC Development Team             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <QtCore>
#include "global.h"

struct Message {

  /** The different types a message can have for display */
  enum Type { Plain, Notice, Action, Nick, Mode, Join, Part, Quit, Kick, Kill, Server, Info, Error };
  enum Flags { None = 0, Self = 1, PrivMsg = 2, Highlight = 4 };

  uint msgId;
  BufferId buffer;
  QString target;
  QString text;
  QString sender;
  Type type;
  quint8 flags;
  QDateTime timeStamp;

  Message(QString _target, Type _type = Plain, QString _text = "", QString _sender = "", quint8 _flags = None)
  : target(_target), text(_text), sender(_sender), type(_type), flags(_flags) { timeStamp = QDateTime::currentDateTime().toUTC(); }

  Message(BufferId _buffer = BufferId(), Type _type = Plain, QString _text = "", QString _sender = "", quint8 _flags = None)
  : buffer(_buffer), text(_text), sender(_sender), type(_type), flags(_flags) { timeStamp = QDateTime::currentDateTime().toUTC(); }

  Message(QDateTime _ts, BufferId _buffer = BufferId(), Type _type = Plain, QString _text = "", QString _sender = "", quint8 _flags = None)
  : buffer(_buffer), text(_text), sender(_sender), type(_type), flags(_flags), timeStamp(_ts) {}

};

QDataStream &operator<<(QDataStream &out, const Message &msg);
QDataStream &operator>>(QDataStream &in, Message &msg);

Q_DECLARE_METATYPE(Message);

#endif

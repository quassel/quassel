/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

struct Message {

  /** The different types a message can have for display */
  enum Type { Plain, Notice, Action, Nick, Mode, Join, Part, Quit, Kick, Kill, Server, Info, Error };
  enum Flags { None = 0, Self = 1, PrivMsg = 2, Highlight = 4 };

  Type type;
  quint8 flags;
  QString target;
  QString sender;
  QString text;
  QDateTime timeStamp;

  Message(QString _target = "", Type _type = Plain, QString _text = "", QString _sender = "", quint8 _flags = None)
  : target(_target), text(_text), sender(_sender), type(_type), flags(_flags) { timeStamp = QDateTime::currentDateTime().toUTC(); }

  static Message plain(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message notice(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message action(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message nick(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message mode(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message join(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message part(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message quit(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message kick(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message kill(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message server(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message info(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
  static Message error(QString _target, QString _text, QString _sender = "", quint8 _flags = None);
};

QDataStream &operator<<(QDataStream &out, const Message &msg);
QDataStream &operator>>(QDataStream &in, Message &msg);

Q_DECLARE_METATYPE(Message);

#endif

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

#include "message.h"
#include <QDataStream>

Message Message::plain(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Plain, _text, _sender, _flags);
}

Message Message::notice(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Notice, _text, _sender, _flags);
}

Message Message::action(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Action, _text, _sender, _flags);
}

Message Message::kick(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Kick, _text, _sender, _flags);
}

Message Message::join(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Join, _text, _sender, _flags);
}

Message Message::part(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Part, _text, _sender, _flags);
}

Message Message::nick(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Nick, _text, _sender, _flags);
}

Message Message::mode(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Mode, _text, _sender, _flags);
}

Message Message::quit(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Quit, _text, _sender, _flags);
}

Message Message::kill(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Kill, _text, _sender, _flags);
}

Message Message::server(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Server, _text, _sender, _flags);
}

Message Message::info(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Info, _text, _sender, _flags);
}

Message Message::error(QString _target, QString _text, QString _sender, Flags _flags) {
  return Message(_target, Error, _text, _sender, _flags);
}

QDataStream &operator<<(QDataStream &out, const Message &msg) {
  out << (quint32)msg.timeStamp.toTime_t() << (quint8)msg.type << (quint8)msg.flags
      << msg.target.toUtf8() << msg.sender.toUtf8() << msg.text.toUtf8();
  return out;
}

QDataStream &operator>>(QDataStream &in, Message &msg) {
  quint8 t, f;
  quint32 ts;
  QByteArray s, m, targ;
  in >> ts >> t >> f >> targ >> s >> m;
  msg.type = (Message::Type)t;
  msg.flags = (Message::Flags)f;
  msg.timeStamp = QDateTime::fromTime_t(ts);
  msg.target = QString::fromUtf8(targ);
  msg.sender = QString::fromUtf8(s);
  msg.text = QString::fromUtf8(m);
  return in;
}

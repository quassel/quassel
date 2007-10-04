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

Message::Message(BufferId __buffer, Type __type, QString __text, QString __sender, quint8 __flags)
  : _buffer(__buffer), _text(__text), _sender(__sender), _type(__type), _flags(__flags) {
  _timeStamp = QDateTime::currentDateTime().toUTC();
}

Message::Message(QDateTime __ts, BufferId __buffer, Type __type, QString __text, QString __sender, quint8 __flags)
  : _timeStamp(__ts), _buffer(__buffer), _text(__text), _sender(__sender), _type(__type), _flags(__flags) {

}

MsgId Message::msgId() const {
  return _msgId;
}

void Message::setMsgId(MsgId _id) {
  _msgId = _id;
}

BufferId Message::buffer() const {
  return _buffer;
}

QString Message::text() const {
  return _text;
}

QString Message::sender() const {
  return _sender;
}

Message::Type Message::type() const {
   return _type;
}

quint8 Message::flags() const {
   return _flags;
}

QDateTime Message::timeStamp() const {
  return _timeStamp;
}


QDataStream &operator<<(QDataStream &out, const Message &msg) {
  out << (quint32)msg.timeStamp().toTime_t() << (quint8)msg.type() << (quint8)msg.flags()
      << msg.buffer() << msg.sender().toUtf8() << msg.text().toUtf8();
  return out;
}

QDataStream &operator>>(QDataStream &in, Message &msg) {
  quint8 t, f;
  quint32 ts;
  QByteArray s, m;
  BufferId buf;
  in >> ts >> t >> f >> buf >> s >> m;
  msg._type = (Message::Type)t;
  msg._flags = (quint8)f;
  msg._buffer = buf;
  msg._timeStamp = QDateTime::fromTime_t(ts);
  msg._sender = QString::fromUtf8(s);
  msg._text = QString::fromUtf8(m);
  return in;
}


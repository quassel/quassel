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
#include "util.h"
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

QString Message::mircToInternal(QString mirc) {
  mirc.replace('%', "%%");      // escape % just to be sure
  mirc.replace('\x02', "%B");
  mirc.replace('\x03', "%C");
  mirc.replace('\x0f', "%O");
  mirc.replace('\x12', "%R");
  mirc.replace('\x16', "%R");
  mirc.replace('\x1d', "%S");
  mirc.replace('\x1f', "%U");
  return mirc;
}

void Message::format() {
  if(!_formattedText.isNull()) return; // already done
  QString user = userFromMask(sender());
  QString host = hostFromMask(sender());
  QString nick = nickFromMask(sender());
  QString txt = mircToInternal(text());
  QString networkName = buffer().network();
  QString bufferName = buffer().buffer();

  _formattedTimeStamp = tr("%DT[%1]").arg(timeStamp().toLocalTime().toString("hh:mm:ss"));

  QString s, t;
  switch(type()) {
    case Message::Plain:
      s = tr("%DS<%1>").arg(nick); t = tr("%D0%1").arg(txt); break;
    case Message::Server:
      s = tr("%Ds*"); t = tr("%Ds%1").arg(txt); break;
    case Message::Error:
      s = tr("%De*"); t = tr("%De%1").arg(txt); break;
    case Message::Join:
      s = tr("%Dj-->"); t = tr("%Dj%DN%DU%1%DU%DN %DH(%2@%3)%DH has joined %DC%DU%4%DU%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      s = tr("%Dp<--"); t = tr("%Dp%DN%DU%1%DU%DN %DH(%2@%3)%DH has left %DC%DU%4%DU%DC").arg(nick, user, host, bufferName);
      if(!txt.isEmpty()) t = QString("%1 (%2)").arg(t).arg(txt);
      break;
    case Message::Quit:
      s = tr("%Dq<--"); t = tr("%Dq%DN%DU%1%DU%DN %DH(%2@%3)%DH has quit").arg(nick, user, host);
      if(!txt.isEmpty()) t = QString("%1 (%2)").arg(t).arg(txt);
      break;
    case Message::Kick:
    { s = tr("%Dk<-*");
    QString victim = txt.section(" ", 0, 0);
        //if(victim == ui.ownNick->currentText()) victim = tr("you");
    QString kickmsg = txt.section(" ", 1);
    t = tr("%Dk%DN%DU%1%DU%DN has kicked %DN%DU%2%DU%DN from %DC%DU%3%DU%DC").arg(nick).arg(victim).arg(bufferName);
    if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
    }
    break;
    case Message::Nick:
      s = tr("%Dr<->");
      if(nick == text()) t = tr("%DrYou are now known as %DN%1%DN").arg(text());
      else t = tr("%Dr%DN%1%DN is now known as %DN%DU%2%DU%DN").arg(nick, text());
      break;
    case Message::Mode:
      s = tr("%Dm***");
      if(nick.isEmpty()) t = tr("%DmUser mode: %DM%1%DM").arg(text());
      else t = tr("%DmMode %DM%1%DM by %DN%DU%2%DU%DN").arg(text(), nick);
      break;
    case Message::Action:
      s = tr("%Da-*-");
      t = tr("%Da%DN%DU%1%DU%DN %2").arg(nick).arg(text());
      break;
    default:
      s = tr("%De%1").arg(sender());
      t = tr("%De[%1]").arg(text());
  }
  _formattedSender = s;
  _formattedText = t;
}

QString Message::formattedTimeStamp() {
  format();
  return _formattedTimeStamp;
}

QString Message::formattedSender() {
  format();
  return _formattedSender;
}

QString Message::formattedText() {
  format();
  return _formattedText;
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


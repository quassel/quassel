/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "message.h"

#include "util.h"

#include <QDataStream>

Message::Message(BufferInfo bufferInfo, Type type, QString text, QString sender, quint8 flags)
  : _timestamp(QDateTime::currentDateTime().toUTC()),
    _bufferInfo(bufferInfo),
    _text(text),
    _sender(sender),
    _type(type),
    _flags(flags)
{
}

Message::Message(QDateTime ts,BufferInfo bufferInfo, Type type, QString text, QString sender, quint8 flags)
  : _timestamp(ts),
    _bufferInfo(bufferInfo),
    _text(text),
    _sender(sender),
    _type(type),
    _flags(flags)
{
}

void Message::setFlags(quint8 flags) {
  _flags = flags;
}

QString Message::mircToInternal(QString mirc) {
  mirc.replace('%', "%%");      // escape % just to be sure
  mirc.replace('\x02', "%B");
  mirc.replace('\x0f', "%O");
  mirc.replace('\x12', "%R");
  mirc.replace('\x16', "%R");
  mirc.replace('\x1d', "%S");
  mirc.replace('\x1f', "%U");

  // Now we bring the color codes (\x03) in a sane format that can be parsed more easily later.
  // %Dcfxx is foreground, %Dcbxx is background color, where xx is a 2 digit dec number denoting the color code.
  // %Dc- turns color off.
  // Note: We use the "mirc standard" as described in <http://www.mirc.co.uk/help/color.txt>.
  //       This means that we don't accept something like \x03,5 (even though others, like WeeChat, do).
  int pos = 0;
  for(;;) {
    pos = mirc.indexOf('\x03', pos);
    if(pos < 0) break; // no more mirc color codes
    QString ins, num;
    int l = mirc.length();
    int i = pos + 1;
    // check for fg color
    if(i < l && mirc[i].isDigit()) {
      num = mirc[i++];
      if(i < l && mirc[i].isDigit()) num.append(mirc[i++]);
      else num.prepend('0');
      ins = QString("%Dcf%1").arg(num);

      if(i+1 < l && mirc[i] == ',' && mirc[i+1].isDigit()) {
        i++;
        num = mirc[i++];
        if(i < l && mirc[i].isDigit()) num.append(mirc[i++]);
        else num.prepend('0');
        ins += QString("%Dcb%1").arg(num);
      }
    } else {
      ins = "%Dc-";
    }
    mirc.replace(pos, i-pos, ins);
  }
  return mirc;
}

void Message::format() {
  if(!_formattedText.isNull())
    return; // already done
  
  QString user = userFromMask(sender());
  QString host = hostFromMask(sender());
  QString nick = nickFromMask(sender());
  QString txt = mircToInternal(text());
  QString bufferName = bufferInfo().bufferName();

  _formattedTimestamp = tr("%DT[%1]").arg(timestamp().toLocalTime().toString("hh:mm:ss"));

  QString s, t;
  switch(type()) {
    case Message::Plain:
      s = tr("%DS<%1>").arg(nick); t = tr("%D0%1").arg(txt); break;
    case Message::Notice:
      s = tr("%Dn[%1]").arg(nick); t = tr("%Dn%1").arg(txt); break;
    case Message::Server:
      s = tr("%Ds*"); t = tr("%Ds%1").arg(txt); break;
    case Message::Error:
      s = tr("%De*"); t = tr("%De%1").arg(txt); break;
    case Message::Join:
      s = tr("%Dj-->"); t = tr("%Dj%DN%1%DN %DH(%2@%3)%DH has joined %DC%4%DC").arg(nick, user, host, bufferName); break;
    case Message::Part:
      s = tr("%Dp<--"); t = tr("%Dp%DN%1%DN %DH(%2@%3)%DH has left %DC%4%DC").arg(nick, user, host, bufferName);
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
    t = tr("%Dk%DN%1%DN has kicked %DN%2%DN from %DC%3%DC").arg(nick).arg(victim).arg(bufferName);
    if(!kickmsg.isEmpty()) t = QString("%1 (%2)").arg(t).arg(kickmsg);
    }
    break;
    case Message::Nick:
      s = tr("%Dr<->");
      if(nick == text()) t = tr("%DrYou are now known as %DN%1%DN").arg(txt);
      else t = tr("%Dr%DN%1%DN is now known as %DN%2%DN").arg(nick, txt);
      break;
    case Message::Mode:
      s = tr("%Dm***");
      if(nick.isEmpty()) t = tr("%DmUser mode: %DM%1%DM").arg(text());
      else t = tr("%DmMode %DM%1%DM by %DN%2%DN").arg(txt, nick);
      break;
    case Message::Action:
      s = tr("%Da-*-");
      t = tr("%Da%DN%1%DN %2").arg(nick).arg(txt);
      break;
    default:
      s = tr("%De%1").arg(sender());
      t = tr("%De[%1]").arg(txt);
  }
  _formattedSender = s;
  _formattedText = t;
}

QString Message::formattedTimestamp() {
  format();
  return _formattedTimestamp;
}

QString Message::formattedSender() {
  format();
  return _formattedSender;
}

QString Message::formattedText() {
  format();
  return _formattedText;
}

/*
QString Message::formattedToHtml(const QString &f) {


  return f;
}
*/

QDataStream &operator<<(QDataStream &out, const Message &msg) {
  out << msg.msgId() << (quint32)msg.timestamp().toTime_t() << (quint32)msg.type() << (quint8)msg.flags()
      << msg.bufferInfo() << msg.sender().toUtf8() << msg.text().toUtf8();
  return out;
}

QDataStream &operator>>(QDataStream &in, Message &msg) {
  quint8 f;
  quint32 t;
  quint32 ts;
  QByteArray s, m;
  BufferInfo buf;
  in >> msg._msgId >> ts >> t >> f >> buf >> s >> m;
  msg._type = (Message::Type)t;
  msg._flags = (quint8)f;
  msg._bufferInfo = buf;
  msg._timestamp = QDateTime::fromTime_t(ts);
  msg._sender = QString::fromUtf8(s);
  msg._text = QString::fromUtf8(m);
  return in;
}


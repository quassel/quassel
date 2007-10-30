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

#include <QtCore>

#include "chatline.h"

ChatLine::ChatLine(Message msg) {
  _text = msg.text();  // FIXME
  _sender = msg.sender();
  _timestamp = msg.timestamp();
  _msgId = msg.msgId();
  _bufferInfo = msg.buffer();

  _htmlSender = formattedToHtml(msg.formattedSender());
  _htmlText = formattedToHtml(msg.formattedText());
  _htmlTimestamp = formattedToHtml(msg.formattedTimestamp());
}

QString ChatLine::sender() const {
  return _sender;
}

QString ChatLine::text() const {
  return _text;
}

MsgId ChatLine::msgId() const {
  return _msgId;
}

BufferInfo ChatLine::bufferInfo() const {
  return _bufferInfo;
}

QDateTime ChatLine::timestamp() const {
  return _timestamp;
}

QString ChatLine::htmlSender() const {
  return _htmlSender;
}

QString ChatLine::htmlText() const {
  return _htmlText;
}

QString ChatLine::htmlTimestamp() const {
  return _htmlTimestamp;
}


QString ChatLine::formattedToHtml(const QString &f) {
   
  return f;
}

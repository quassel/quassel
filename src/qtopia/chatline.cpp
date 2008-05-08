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

#include <QtCore>

#include "chatline.h"
#include "qtopiaui.h"
#include "qtopiauistyle.h"

ChatLine::ChatLine(Message msg) {
  _styledSender = QtopiaUi::style()->styleString(msg.formattedSender());
  _styledContents = QtopiaUi::style()->styleString(msg.formattedText());
  _timestamp = msg.timestamp();
  _msgId = msg.msgId();
  _bufferInfo = msg.bufferInfo();


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

UiStyle::StyledText ChatLine::styledSender() const {
  return _styledSender;
}

UiStyle::StyledText ChatLine::styledContents() const {
  return _styledContents;
}



QString ChatLine::formattedToHtml(const QString &f) {
   
  return f;
}

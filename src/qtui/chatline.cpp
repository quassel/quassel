/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include <QDateTime>
#include <QString>
#include <QtGui>

#include "bufferinfo.h"
#include "chatitem.h"
#include "chatline.h"
#include "qtui.h"

ChatLine::ChatLine(Message msg) : QGraphicsItem(), AbstractUiMsg() {
  _styledTimestamp = QtUi::style()->styleString(msg.formattedTimestamp());
  _styledSender = QtUi::style()->styleString(msg.formattedSender());
  _styledText = QtUi::style()->styleString(msg.formattedText());
  _msgId = msg.msgId();
  _timestamp = msg.timestamp();

  _tsColWidth = _senderColWidth = _textColWidth = 0;
  QTextOption option;
  option.setWrapMode(QTextOption::NoWrap);
  _tsItem = new ChatItem(this);
  _tsItem->setTextOption(option);
  _tsItem->setText(_styledTimestamp);

  option.setAlignment(Qt::AlignRight);
  _senderItem = new ChatItem(this);
  _senderItem->setTextOption(option);
  _senderItem->setText(_styledSender);

  option.setAlignment(Qt::AlignLeft);
  option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  _textItem = new ChatItem(this);
  _textItem->setTextOption(option);
  _textItem->setText(_styledText);

}

ChatLine::~ChatLine() {
  
}

QString ChatLine::sender() const {
  return QString();
}

QString ChatLine::text() const {
  return QString();
}

MsgId ChatLine::msgId() const {
  return 0;
}

BufferInfo ChatLine::bufferInfo() const {
  Q_ASSERT(false); // do we actually need this function???
  return BufferInfo();
}

QDateTime ChatLine::timestamp() const {
  return QDateTime();
}

QRectF ChatLine::boundingRect () const {
  return childrenBoundingRect();
}

void ChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

}

void ChatLine::setColumnWidths(int tsColWidth, int senderColWidth, int textColWidth) {
  if(tsColWidth >= 0) {
    _tsColWidth = tsColWidth;
    _tsItem->setWidth(tsColWidth);
  }
  if(senderColWidth >= 0) {
    _senderColWidth = senderColWidth;
    _senderItem->setWidth(senderColWidth);
  }
  if(textColWidth >= 0) {
    _textColWidth = textColWidth;
    _textItem->setWidth(textColWidth);
  }
  layout();
}

void ChatLine::layout() {
  prepareGeometryChange();
  _tsItem->setPos(QPointF(0, 0));
  _senderItem->setPos(QPointF(_tsColWidth + QtUi::style()->sepTsSender(), 0));
  _textItem->setPos(QPointF(_tsColWidth + QtUi::style()->sepTsSender() + _senderColWidth + QtUi::style()->sepSenderText(), 0));
}


bool ChatLine::sceneEvent ( QEvent * event ) {
  qDebug() <<(void*)this<< "receiving event";
  event->ignore();
  return false;
}



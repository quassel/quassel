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

#include <QDateTime>
#include <QString>
#include <QtGui>

#include "bufferinfo.h"
#include "chatitem.h"
#include "chatline.h"
#include "qtui.h"

ChatLine::ChatLine(const QModelIndex &index, QGraphicsItem *parent) : QGraphicsItem(parent) {
  _timestampItem = new ChatItem(QPersistentModelIndex(index.sibling(index.row(), ChatLineModel::TimestampColumn)), this);
  _senderItem = new ChatItem(QPersistentModelIndex(index.sibling(index.row(), ChatLineModel::SenderColumn)), this);
  _contentsItem = new ChatItem(QPersistentModelIndex(index.sibling(index.row(), ChatLineModel::ContentsColumn)), this);

  _timestampItem->setPos(0,0);
  _width = _height = 0;
}

ChatLine::~ChatLine() {
  delete _timestampItem;
  delete _senderItem;
  delete _contentsItem;
}

QRectF ChatLine::boundingRect () const {
  //return childrenBoundingRect();
  return QRectF(0, 0, _width, _height);
}

qreal ChatLine::setGeometry(qreal width, qreal firstHandlePos, qreal secondHandlePos) {
  if(width != _width) prepareGeometryChange();
  qreal firstsep = QtUi::style()->firstColumnSeparator()/2;
  qreal secondsep = QtUi::style()->secondColumnSeparator()/2;

  _timestampItem->setWidth(firstHandlePos - firstsep);
  _senderItem->setWidth(secondHandlePos - firstHandlePos - (firstsep+secondsep));
  _height = _contentsItem->setWidth(width - secondHandlePos - secondsep);

  _senderItem->setPos(firstHandlePos + firstsep, 0);
  _contentsItem->setPos(secondHandlePos + secondsep, 0);

  _width = width;
  return _height;
}

void ChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

}

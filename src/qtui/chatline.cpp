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
}

ChatLine::~ChatLine() {
  delete _timestampItem;
  delete _senderItem;
  delete _contentsItem;
}

// FIXME make more efficient by caching width/height
QRectF ChatLine::boundingRect () const {
  return childrenBoundingRect();
}

int ChatLine::setColumnWidths(int ts, int sender, int contents) {
  _timestampItem->setWidth(ts);
  _senderItem->setWidth(sender);
  int h = _contentsItem->setWidth(contents);

  _senderItem->setPos(ts, 0);
  _contentsItem->setPos(ts + sender, 0);

  return h;
}

void ChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

}

/*
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
*/



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
  _selection = 0;

  if(_contentsItem->data(MessageModel::FlagsRole).toInt() & Message::Highlight) setHighlighted(true);
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

ChatItem *ChatLine::item(ChatLineModel::ColumnType column) const {
  switch(column) {
    case ChatLineModel::TimestampColumn: return _timestampItem;
    case ChatLineModel::SenderColumn: return _senderItem;
    case ChatLineModel::ContentsColumn: return _contentsItem;
    default: return 0;
  }
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

void ChatLine::setSelected(bool selected, ChatLineModel::ColumnType minColumn) {
  if(selected) {
    quint8 sel = (_selection & 0x80) | 0x40 | minColumn;
    if(sel != _selection) {
      _selection = sel;
      for(int i = 0; i < minColumn; i++) item((ChatLineModel::ColumnType)i)->clearSelection();
      for(int i = minColumn; i <= ChatLineModel::ContentsColumn; i++) item((ChatLineModel::ColumnType)i)->setFullSelection();
      update();
    }
  } else {
    quint8 sel = _selection & 0x80;
    if(sel != _selection) {
      _selection = sel;
      for(int i = 0; i <= ChatLineModel::ContentsColumn; i++) item((ChatLineModel::ColumnType)i)->clearSelection();
      update();
    }
  }
}

void ChatLine::setHighlighted(bool highlighted) {
  if(highlighted) _selection |= 0x80;
  else _selection &= 0x7f;
  update();
}

void ChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  if(_selection & Highlighted) {
    painter->fillRect(boundingRect(), QBrush(QtUi::style()->highlightColor()));
  }
  if(_selection & Selected) {
    qreal left = item((ChatLineModel::ColumnType)(_selection & 0x3f))->x();
    QRectF selectRect(left, 0, width() - left, height());
    painter->fillRect(selectRect, QApplication::palette().brush(QPalette::Highlight));
  }
}

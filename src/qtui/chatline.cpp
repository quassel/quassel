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
#include "buffersyncer.h"
#include "client.h"
#include "chatitem.h"
#include "chatline.h"
#include "messagemodel.h"
#include "networkmodel.h"
#include "qtui.h"
#include "qtuisettings.h"

ChatLine::ChatLine(int row, QAbstractItemModel *model, QGraphicsItem *parent)
  : QGraphicsItem(parent),
    _row(row), // needs to be set before the items
    _timestampItem(model, this),
    _senderItem(model, this),
    _contentsItem(model, this),
    _width(0),
    _height(0),
    _selection(0)
{
  Q_ASSERT(model);
  QModelIndex index = model->index(row, ChatLineModel::ContentsColumn);
  setHighlighted(model->data(index, MessageModel::FlagsRole).toInt() & Message::Highlight);
}

QRectF ChatLine::boundingRect () const {
  //return childrenBoundingRect();
  return QRectF(0, 0, _width, _height);
}

ChatItem &ChatLine::item(ChatLineModel::ColumnType column) {
  switch(column) {
    case ChatLineModel::TimestampColumn:
      return _timestampItem;
    case ChatLineModel::SenderColumn:
      return _senderItem;
    case ChatLineModel::ContentsColumn:
      return _contentsItem;
  default:
    return *(ChatItem *)0; // provoke an error
  }
}

qreal ChatLine::setGeometry(qreal width) {
  if(width != _width)
    prepareGeometryChange();
  QRectF firstColHandleRect = chatScene()->firstColumnHandleRect();
  QRectF secondColHandleRect = chatScene()->secondColumnHandleRect();

  _height = _contentsItem.setGeometry(width - secondColHandleRect.right());
  _timestampItem.setGeometry(firstColHandleRect.left(), _height);
  _senderItem.setGeometry(secondColHandleRect.left() - firstColHandleRect.right(), _height);

  _senderItem.setPos(firstColHandleRect.right(), 0);
  _contentsItem.setPos(secondColHandleRect.right(), 0);

  _width = width;
  return _height;
}

void ChatLine::setSelected(bool selected, ChatLineModel::ColumnType minColumn) {
  if(selected) {
    quint8 sel = (_selection & 0x80) | 0x40 | minColumn;
    if(sel != _selection) {
      _selection = sel;
      for(int i = 0; i < minColumn; i++)
	item((ChatLineModel::ColumnType)i).clearSelection();
      for(int i = minColumn; i <= ChatLineModel::ContentsColumn; i++)
	item((ChatLineModel::ColumnType)i).setFullSelection();
      update();
    }
  } else {
    quint8 sel = _selection & 0x80;
    if(sel != _selection) {
      _selection = sel;
      for(int i = 0; i <= ChatLineModel::ContentsColumn; i++)
	item((ChatLineModel::ColumnType)i).clearSelection();
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
  Q_UNUSED(option);
  Q_UNUSED(widget);
  if(_selection & Highlighted) {
    painter->fillRect(boundingRect(), QBrush(QtUi::style()->highlightColor()));
  }
  if(_selection & Selected) {
    qreal left = item((ChatLineModel::ColumnType)(_selection & 0x3f)).x();
    QRectF selectRect(left, 0, width() - left, height());
    painter->fillRect(selectRect, QApplication::palette().brush(QPalette::Highlight));
  }

  // new line marker
  const QAbstractItemModel *model_ = model();
  if(model_ && row() > 0) {
    QModelIndex prevRowIdx = model_->index(row() - 1, 0);
    MsgId msgId = model_->data(prevRowIdx, MessageModel::MsgIdRole).value<MsgId>();
    Message::Flags flags = (Message::Flags)model_->data(model_->index(row(), 0), MessageModel::FlagsRole).toInt();
    // don't show the marker if we wrote that new line
    if(!(flags & Message::Self)) {
      BufferId bufferId = model_->data(prevRowIdx, MessageModel::BufferIdRole).value<BufferId>();
      if(msgId == Client::networkModel()->lastSeenMsgId(bufferId) && chatScene()->isSingleBufferScene()) {
	QtUiSettings s("QtUiStyle/Colors");
	QLinearGradient gradient(0, 0, 0, height());
	gradient.setColorAt(0, s.value("newMsgMarkerFG", QColor(Qt::red)).value<QColor>());
	gradient.setColorAt(0.1, Qt::transparent);
	painter->fillRect(boundingRect(), gradient);
      }
    }
  }
}

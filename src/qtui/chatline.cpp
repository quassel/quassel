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
#include "columnhandleitem.h"
#include "messagemodel.h"
#include "networkmodel.h"
#include "qtui.h"
#include "qtuisettings.h"
#include "qtuistyle.h"

ChatLine::ChatLine(int row, QAbstractItemModel *model,
		   const qreal &width,
		   const qreal &timestampWidth, const qreal &senderWidth, const qreal &contentsWidth,
		   const QPointF &senderPos, const QPointF &contentsPos,
		   QGraphicsItem *parent)
  : QGraphicsItem(parent),
    _row(row), // needs to be set before the items
    _model(model),
    _contentsItem(contentsWidth, contentsPos, this),
    _senderItem(senderWidth, _contentsItem.height(), senderPos, this),
    _timestampItem(timestampWidth, _contentsItem.height(), this),
    _width(width),
    _height(_contentsItem.height()),
    _selection(0)
{
  Q_ASSERT(model);
  QModelIndex index = model->index(row, ChatLineModel::ContentsColumn);
  setZValue(0);
  setHighlighted(model->data(index, MessageModel::FlagsRole).toInt() & Message::Highlight);
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

// NOTE: senderPos is in ChatLines coordinate system!
void ChatLine::setFirstColumn(const qreal &timestampWidth, const qreal &senderWidth, const QPointF &senderPos) {
  _timestampItem.prepareGeometryChange();
  _timestampItem.setGeometry(timestampWidth, _height);
  // senderItem doesn't need a geom change as it's Pos is changed (ensured by void ChatScene::firstHandlePositionChanged(qreal xpos))
  _senderItem.setGeometry(senderWidth, _height);
  _senderItem.setPos(senderPos);

  _timestampItem.clearLayout();
  _senderItem.clearLayout();
}

// NOTE: contentsPos is in ChatLines coordinate system!
void ChatLine::setSecondColumn(const qreal &senderWidth, const qreal &contentsWidth,
			       const QPointF &contentsPos, qreal &linePos) {
  // contentsItem doesn't need a geom change as it's Pos is changed (ensured by void ChatScene::firstHandlePositionChanged(qreal xpos))
  qreal height = _contentsItem.setGeometryByWidth(contentsWidth);
  linePos -= height;
  bool needGeometryChange = linePos == pos().y() && height != _height;

  if(needGeometryChange) {
    _timestampItem.prepareGeometryChange();
    _senderItem.prepareGeometryChange();
  }
  _timestampItem.setHeight(height);
  _senderItem.setGeometry(senderWidth, height);

  _contentsItem.setPos(contentsPos);

  _timestampItem.clearLayout();
  _senderItem.clearLayout();

  if(needGeometryChange)
    prepareGeometryChange();

  _height = height;

  setPos(0, linePos);
}

void ChatLine::setGeometryByWidth(const qreal &width, const qreal &contentsWidth, qreal &linePos) {
  qreal height = _contentsItem.setGeometryByWidth(contentsWidth);
  linePos -= height;
  bool needGeometryChange = linePos == pos().y();

  if(needGeometryChange) {
    _timestampItem.prepareGeometryChange();
    _senderItem.prepareGeometryChange();
  }
  _timestampItem.setHeight(height);
  _senderItem.setHeight(height);
  _contentsItem.clearLayout();

  if(needGeometryChange)
    prepareGeometryChange();

  _height = height;
  _width = width;

  setPos(0, linePos); // set pos is _very_ cheap if nothing changes.
}

void ChatLine::setSelected(bool selected, ChatLineModel::ColumnType minColumn) {
  if(selected) {
    quint8 sel = (_selection & Highlighted) | Selected | minColumn;
    if(sel != _selection) {
      _selection = sel;
      for(int i = 0; i < minColumn; i++)
	item((ChatLineModel::ColumnType)i).clearSelection();
      for(int i = minColumn; i <= ChatLineModel::ContentsColumn; i++)
	item((ChatLineModel::ColumnType)i).setFullSelection();
      update();
    }
  } else {
    quint8 sel = _selection & Highlighted;
    if(sel != _selection) {
      _selection = sel;
      for(int i = 0; i <= ChatLineModel::ContentsColumn; i++)
	item((ChatLineModel::ColumnType)i).clearSelection();
      update();
    }
  }
}

void ChatLine::setHighlighted(bool highlighted) {
  if(highlighted) _selection |= Highlighted;
  else _selection &= ~Highlighted;
  update();
}

void ChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);
  if(_selection & Highlighted) {
    painter->fillRect(boundingRect(), QBrush(QtUi::style()->highlightColor()));
  }
  if(_selection & Selected) {
    qreal left = item((ChatLineModel::ColumnType)(_selection & ItemMask)).x();
    QRectF selectRect(left, 0, width() - left, height());
    painter->fillRect(selectRect, QApplication::palette().brush(QPalette::Highlight));
  }

  // new line marker
  const QAbstractItemModel *model_ = model();
  if(model_ && row() > 0  && chatScene()->isSingleBufferScene()) {
    QModelIndex prevRowIdx = model_->index(row() - 1, 0);
    MsgId prevMsgId = model_->data(prevRowIdx, MessageModel::MsgIdRole).value<MsgId>();
    QModelIndex myIdx = model_->index(row(), 0);
    MsgId myMsgId = model_->data(myIdx, MessageModel::MsgIdRole).value<MsgId>();
    Message::Flags flags = (Message::Flags)model_->data(myIdx, MessageModel::FlagsRole).toInt();
    // don't show the marker if we wrote that new line
    if(!(flags & Message::Self)) {
      BufferId bufferId = BufferId(chatScene()->idString().toInt());
      MsgId lastSeenMsgId = Client::networkModel()->lastSeenMsgId(bufferId);
      if(lastSeenMsgId < myMsgId && lastSeenMsgId >= prevMsgId) {
	QtUiStyleSettings s("Colors");
	QLinearGradient gradient(0, 0, 0, contentsItem().fontMetrics()->lineSpacing());
	gradient.setColorAt(0, s.value("newMsgMarkerFG", QColor(Qt::red)).value<QColor>());
	gradient.setColorAt(0.1, Qt::transparent);
	painter->fillRect(boundingRect(), gradient);
      }
    }
  }
}

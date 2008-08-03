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

#include <QApplication>
#include <QClipboard>
#include <QGraphicsSceneMouseEvent>
#include <QPersistentModelIndex>

#include "buffer.h"
#include "chatitem.h"
#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "columnhandleitem.h"
#include "qtui.h"
#include "qtuisettings.h"

const qreal minContentsWidth = 200;

ChatScene::ChatScene(QAbstractItemModel *model, const QString &idString, QObject *parent)
  : QGraphicsScene(parent),
  _idString(idString),
  _model(model)
{
  _width = 0;
  _selectingItem = 0;
  _isSelecting = false;
  _selectionStart = -1;
  connect(this, SIGNAL(sceneRectChanged(const QRectF &)), this, SLOT(rectChanged(const QRectF &)));

  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  connect(model, SIGNAL(modelAboutToBeReset()), this, SLOT(modelReset()));
  for(int i = 0; i < model->rowCount(); i++) {
    ChatLine *line = new ChatLine(model->index(i, 0));
    _lines.append(line);
    addItem(line);
  }

  QtUiSettings s;
  int defaultFirstColHandlePos = s.value("ChatView/DefaultFirstColumnHandlePos", 80).toInt();
  int defaultSecondColHandlePos = s.value("ChatView/DefaultSecondColumnHandlePos", 200).toInt();

  firstColHandlePos = s.value(QString("ChatView/%1/FirstColumnHandlePos").arg(_idString),
                               defaultFirstColHandlePos).toInt();
  secondColHandlePos = s.value(QString("ChatView/%1/SecondColumnHandlePos").arg(_idString),
                                defaultSecondColHandlePos).toInt();

  firstColHandle = new ColumnHandleItem(QtUi::style()->firstColumnSeparator()); addItem(firstColHandle);
  secondColHandle = new ColumnHandleItem(QtUi::style()->secondColumnSeparator()); addItem(secondColHandle);

  connect(firstColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(handlePositionChanged(qreal)));
  connect(secondColHandle, SIGNAL(positionChanged(qreal)), this, SLOT(handlePositionChanged(qreal)));

  firstColHandle->setXPos(firstColHandlePos);
  firstColHandle->setXLimits(0, secondColHandlePos);
  secondColHandle->setXPos(secondColHandlePos);
  secondColHandle->setXLimits(firstColHandlePos, width() - minContentsWidth);

  emit heightChanged(height());
}

ChatScene::~ChatScene() {

}

void ChatScene::rowsInserted(const QModelIndex &index, int start, int end) {
  Q_UNUSED(index);
  // maybe make this more efficient by prepending stuff with negative yval
  // dunno if that's worth not guranteeing that 0 is on the top...
  // TODO bulk inserts, iterators
  qreal h = 0;
  qreal y = 0;
  if(_width && start > 0) y = _lines.value(start - 1)->y() + _lines.value(start - 1)->height();
  for(int i = start; i <= end; i++) {
    ChatLine *line = new ChatLine(model()->index(i, 0));
    _lines.insert(i, line);
    addItem(line);
    if(_width > 0) {
      line->setPos(0, y+h);
      h += line->setGeometry(_width, firstColHandlePos, secondColHandlePos);
    }
  }
  if(h > 0) {
    _height += h;
    for(int i = end+1; i < _lines.count(); i++) {
      _lines.value(i)->moveBy(0, h);
    }
    setSceneRect(QRectF(0, 0, _width, _height));
    emit heightChanged(_height);
  }
}

void ChatScene::modelReset() {
  foreach(ChatLine *line, _lines) {
    removeItem(line);
    delete line;
  }
  _lines.clear();
  setSceneRect(QRectF(0, 0, _width, 0));
}

void ChatScene::setWidth(qreal w) {
  _width = w;
  _height = 0;
  foreach(ChatLine *line, _lines) {
    line->setPos(0, _height);
    _height += line->setGeometry(_width, firstColHandlePos, secondColHandlePos);
  }
  setSceneRect(QRectF(0, 0, w, _height));
  secondColHandle->setXLimits(firstColHandlePos, width() - minContentsWidth);
  emit heightChanged(_height);
}

void ChatScene::rectChanged(const QRectF &rect) {
  firstColHandle->sceneRectChanged(rect);
  secondColHandle->sceneRectChanged(rect);
}

void ChatScene::handlePositionChanged(qreal xpos) {
  bool first = (sender() == firstColHandle);
  qreal oldx;
  if(first) {
    oldx = firstColHandlePos;
    firstColHandlePos = xpos;
  } else {
    oldx = secondColHandlePos;
    secondColHandlePos = xpos;
  }
  QtUiSettings s;
  s.setValue(QString("ChatView/%1/FirstColumnHandlePos").arg(_idString), firstColHandlePos);
  s.setValue(QString("ChatView/%1/SecondColumnHandlePos").arg(_idString), secondColHandlePos);
  s.setValue(QString("ChatView/DefaultFirstColumnHandlePos"), firstColHandlePos);
  s.setValue(QString("ChatView/DefaultSecondColumnHandlePos"), secondColHandlePos);

  setWidth(width());  // readjust all chatlines
  // we get ugly redraw errors if we don't update this explicitly... :(
  // width() should be the same for both handles, so just use firstColHandle regardless
  update(qMin(oldx, xpos) - firstColHandle->width()/2, 0, qMax(oldx, xpos) + firstColHandle->width()/2, height());
}

void ChatScene::setSelectingItem(ChatItem *item) {
  if(_selectingItem) _selectingItem->clearSelection();
  _selectingItem = item;
}

void ChatScene::startGlobalSelection(ChatItem *item, const QPointF &itemPos) {
  _selectionStart = _selectionEnd = item->index().row();
  _selectionStartCol = _selectionMinCol = item->index().column();
  _isSelecting = true;
  _lines[_selectionStart]->setSelected(true, (ChatLineModel::ColumnType)_selectionMinCol);
  updateSelection(item->mapToScene(itemPos));
}

void ChatScene::updateSelection(const QPointF &pos) {
  // This is somewhat hacky... we look at the contents item that is at the cursor's y position (ignoring x), since
  // it has the full height. From this item, we can then determine the row index and hence the ChatLine.
  ChatItem *contentItem = static_cast<ChatItem *>(itemAt(QPointF(secondColHandlePos + secondColHandle->width()/2, pos.y())));
  if(!contentItem) return;

  int curRow = contentItem->index().row();
  int curColumn;
  if(pos.x() > secondColHandlePos + secondColHandle->width()/2) curColumn = ChatLineModel::ContentsColumn;
  else if(pos.x() > firstColHandlePos) curColumn = ChatLineModel::SenderColumn;
  else curColumn = ChatLineModel::TimestampColumn;

  ChatLineModel::ColumnType minColumn = (ChatLineModel::ColumnType)qMin(curColumn, _selectionStartCol);
  if(minColumn != _selectionMinCol) {
    _selectionMinCol = minColumn;
    for(int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++) {
      _lines[l]->setSelected(true, minColumn);
    }
  }

  if(curRow > _selectionEnd && curRow > _selectionStart) {  // select further towards bottom
    for(int l = _selectionEnd + 1; l <= curRow; l++) {
      _lines[l]->setSelected(true, minColumn);
    }
  } else if(curRow > _selectionEnd && curRow <= _selectionStart) { // deselect towards bottom
    for(int l = _selectionEnd; l < curRow; l++) {
      _lines[l]->setSelected(false);
    }
  } else if(curRow < _selectionEnd && curRow >= _selectionStart) {
    for(int l = _selectionEnd; l > curRow; l--) {
      _lines[l]->setSelected(false);
    }
  } else if(curRow < _selectionEnd && curRow < _selectionStart) {
    for(int l = _selectionEnd - 1; l >= curRow; l--) {
      _lines[l]->setSelected(true, minColumn);
    }
  }
  _selectionEnd = curRow;

  if(curRow == _selectionStart && minColumn == ChatLineModel::ContentsColumn) {
    _lines[curRow]->setSelected(false);
    _isSelecting = false;
    _selectingItem->continueSelecting(_selectingItem->mapFromScene(pos));
  }
}

void ChatScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if(_isSelecting && event->buttons() & Qt::LeftButton) {
    updateSelection(event->scenePos());
    event->accept();
  } else {
    QGraphicsScene::mouseMoveEvent(event);
  }
}

void ChatScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if(event->buttons() & Qt::LeftButton && _selectionStart >= 0) {
    for(int l = qMin(_selectionStart, _selectionEnd); l <= qMax(_selectionStart, _selectionEnd); l++) {
      _lines[l]->setSelected(false);
    }
    _selectionStart = -1;
    event->accept();
  } else {
    QGraphicsScene::mousePressEvent(event);
  }
}

void ChatScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if(_isSelecting) {
#   ifdef Q_WS_X11
      QApplication::clipboard()->setText(selectionToString(), QClipboard::Selection);
#   endif
//# else
      QApplication::clipboard()->setText(selectionToString());
//# endif
    _isSelecting = false;
    event->accept();
  } else {
    QGraphicsScene::mouseReleaseEvent(event);
  }
}

//!\brief Convert current selection to human-readable string.
QString ChatScene::selectionToString() const {
  //TODO Make selection format configurable!
  if(!_isSelecting) return "";
  QString result;
  for(int l = _selectionStart; l <= _selectionEnd; l++) {
    if(_selectionMinCol == ChatLineModel::TimestampColumn)
      result += _lines[l]->item(ChatLineModel::TimestampColumn)->data(MessageModel::DisplayRole).toString() + " ";
    if(_selectionMinCol <= ChatLineModel::SenderColumn)
      result += _lines[l]->item(ChatLineModel::SenderColumn)->data(MessageModel::DisplayRole).toString() + " ";
    result += _lines[l]->item(ChatLineModel::ContentsColumn)->data(MessageModel::DisplayRole).toString() + "\n";
  }
  return result;
}

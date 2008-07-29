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
  connect(this, SIGNAL(sceneRectChanged(const QRectF &)), this, SLOT(rectChanged(const QRectF &)));

  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int, int)));
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

void ChatScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {

  QGraphicsScene::mouseMoveEvent(event);
}

void ChatScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {

  QGraphicsScene::mousePressEvent(event);
}

void ChatScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {

  QGraphicsScene::mouseReleaseEvent(event);
}

void ChatScene::setSelectingItem(ChatItem *item) {
  if(_selectingItem) _selectingItem->clearSelection();
  _selectingItem = item;
}

void ChatScene::startGlobalSelection(ChatItem *item) {


}


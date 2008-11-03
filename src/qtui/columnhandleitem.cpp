/***************************************************************************
*   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include "columnhandleitem.h"

#include <QApplication>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPalette>

#include <QDebug>

ColumnHandleItem::ColumnHandleItem(qreal w, QGraphicsItem *parent)
  : QGraphicsItem(parent),
    _width(w),
    _boundingRect(-_width/2, 0, _width, 0),
    _hover(0),
    _moving(false),
    _minXPos(0),
    _maxXPos(0),
    _timeLine(350),
    _rulerColor(QApplication::palette().windowText().color())
{
  _timeLine.setUpdateInterval(20);

  setAcceptsHoverEvents(true);
  setZValue(10);
  setCursor(QCursor(Qt::OpenHandCursor));

  connect(&_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(hoverChanged(qreal)));
}

void ColumnHandleItem::setXPos(qreal xpos) {
  setPos(xpos, 0);
  QRectF sceneBRect = _boundingRect.translated(x(), 0);
  _sceneLeft = sceneBRect.left();
  _sceneRight = sceneBRect.right();
}

void ColumnHandleItem::setXLimits(qreal min, qreal max) {
  _minXPos = min;
  _maxXPos = max;
  //if(x() < min) setPos(min, 0);
  //else if(x() > max) setPos(max - width(), 0);
}

void ColumnHandleItem::sceneRectChanged(const QRectF &rect) {
  prepareGeometryChange();
  _boundingRect = QRectF(-_width/2, rect.y(), _width, rect.height());
}

void ColumnHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if(event->buttons() & Qt::LeftButton && _moving) {
    if(contains(event->lastPos())) {
      qreal newx = x() + (event->scenePos() - event->lastScenePos()).x();
      if(newx < _minXPos) newx = _minXPos;
      else if(newx + width() > _maxXPos) newx = _maxXPos - width();
      setPos(newx, 0);
    }
    event->accept();
  } else {
    event->ignore();
  }
}

void ColumnHandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if(event->buttons() & Qt::LeftButton) {
    setCursor(QCursor(Qt::ClosedHandCursor));
    _moving = true;
    event->accept();
  } else {
    event->ignore();
  }
}

void ColumnHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if(_moving) {
    _moving = false;
    setCursor(QCursor(Qt::OpenHandCursor));
    QRectF sceneBRect = _boundingRect.translated(x(), 0);
    _sceneLeft = sceneBRect.left();
    _sceneRight = sceneBRect.right();
    emit positionChanged(x());
    event->accept();
  } else {
    event->ignore();
  }
}

void ColumnHandleItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
  Q_UNUSED(event);

  _timeLine.setDirection(QTimeLine::Forward);
  if(_timeLine.state() != QTimeLine::Running)
    _timeLine.start();
}

void ColumnHandleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  Q_UNUSED(event);

  _timeLine.setDirection(QTimeLine::Backward);
  if(_timeLine.state() != QTimeLine::Running)
    _timeLine.start();
}

void ColumnHandleItem::hoverChanged(qreal value) {
  _hover = value;
  update();
}

void ColumnHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  QLinearGradient gradient(boundingRect().topLeft(), boundingRect().topRight());
  QColor _rulerColor = QApplication::palette().windowText().color();
  _rulerColor.setAlphaF(_hover);
  gradient.setColorAt(0, Qt::transparent);
  gradient.setColorAt(0.4, _rulerColor);
  gradient.setColorAt(0.6, _rulerColor);
  gradient.setColorAt(1, Qt::transparent);
  painter->fillRect(boundingRect(), gradient);
}


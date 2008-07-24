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

#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>

#include <QDebug>

#include "columnhandleitem.h"

ColumnHandleItem::ColumnHandleItem(qreal w, QGraphicsItem *parent) : QGraphicsItem(parent) {
  _width = w;
  setZValue(10);
  setCursor(QCursor(Qt::OpenHandCursor));
  setFlag(ItemIsMovable);
}

void ColumnHandleItem::setXPos(qreal xpos) {
  setPos(xpos - width()/2, (qreal)0);
}

void ColumnHandleItem::sceneRectChanged(const QRectF &rect) {
  if(rect.height() != boundingRect().height())
    prepareGeometryChange();
}

void ColumnHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {

  QGraphicsItem::mouseMoveEvent(event);
}

void ColumnHandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event) { qDebug() << "pressed!";
  setCursor(QCursor(Qt::ClosedHandCursor));
  QGraphicsItem::mousePressEvent(event);
}

void ColumnHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  setCursor(QCursor(Qt::OpenHandCursor));
  QGraphicsItem::mouseReleaseEvent(event);
}

void ColumnHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->drawRect(boundingRect());


}

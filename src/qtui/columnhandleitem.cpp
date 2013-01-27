/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "chatview.h"
#include "columnhandleitem.h"

#include <QApplication>
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPalette>

ColumnHandleItem::ColumnHandleItem(qreal w, QGraphicsItem *parent)
    : QGraphicsObject(parent),
    _width(w),
    _boundingRect(-_width/2, 0, _width, 0),
    _moving(false),
    _offset(0),
    _minXPos(0),
    _maxXPos(0),
    _opacity(0),
    _animation(new QPropertyAnimation(this, "opacity", this))
{
    setAcceptsHoverEvents(true);
    setZValue(10);
    setCursor(QCursor(Qt::OpenHandCursor));

    _animation->setStartValue(0);
    _animation->setEndValue(1);
    _animation->setDirection(QPropertyAnimation::Forward);
    _animation->setDuration(350);
    _animation->setEasingCurve(QEasingCurve::InOutSine);

    //connect(&_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(hoverChanged(qreal)));
}


void ColumnHandleItem::setXPos(qreal xpos)
{
    setPos(xpos, 0);
    QRectF sceneBRect = _boundingRect.translated(x(), 0);
    _sceneLeft = sceneBRect.left();
    _sceneRight = sceneBRect.right();
}


void ColumnHandleItem::setXLimits(qreal min, qreal max)
{
    _minXPos = min;
    _maxXPos = max;
    //if(x() < min) setPos(min, 0);
    //else if(x() > max) setPos(max - width(), 0);
}


void ColumnHandleItem::sceneRectChanged(const QRectF &rect)
{
    prepareGeometryChange();
    _boundingRect = QRectF(-_width/2, rect.y(), _width, rect.height());
}


void ColumnHandleItem::setOpacity(qreal opacity)
{
    _opacity = opacity;
    update();
}


void ColumnHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && _moving) {
        qreal newx = event->scenePos().x() - _offset;
        if (newx < _minXPos)
            newx = _minXPos;
        else if (newx + width() > _maxXPos)
            newx = _maxXPos - width();
        setPos(newx, 0);
        event->accept();
    }
    else {
        event->ignore();
    }
}


void ColumnHandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        _moving = true;
        _offset = event->pos().x();
        event->accept();
    }
    else {
        event->ignore();
    }
}


void ColumnHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (_moving) {
        _moving = false;
        QRectF sceneBRect = _boundingRect.translated(x(), 0);
        _sceneLeft = sceneBRect.left();
        _sceneRight = sceneBRect.right();
        emit positionChanged(x());
        QApplication::restoreOverrideCursor();
        event->accept();
    }
    else {
        event->ignore();
    }
}


void ColumnHandleItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    _animation->setDirection(QPropertyAnimation::Forward);
    _animation->start();
}


void ColumnHandleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    _animation->setDirection(QPropertyAnimation::Backward);
    _animation->start();
}


void ColumnHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QLinearGradient gradient(boundingRect().topLeft(), boundingRect().topRight());
    QColor color = QApplication::palette().windowText().color();
    color.setAlphaF(_opacity);
    gradient.setColorAt(0, Qt::transparent);
    gradient.setColorAt(0.45, color);
    gradient.setColorAt(0.55, color);
    gradient.setColorAt(1, Qt::transparent);
    painter->fillRect(boundingRect(), gradient);
}

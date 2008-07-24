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

#ifndef COLUMNHANDLEITEM_H_
#define COLUMNHANDLEITEM_H_

#include <QGraphicsItem>

class ColumnHandleItem : public QGraphicsItem {

  public:
    ColumnHandleItem(qreal width, QGraphicsItem *parent = 0);

    inline qreal width() const;
    inline QRectF boundingRect() const;
    void setXPos(qreal xpos);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void sceneRectChanged(const QRectF &);

  private:
    qreal _width;
};

qreal ColumnHandleItem::width() const {
  return _width;
}

QRectF ColumnHandleItem::boundingRect() const {
  return QRectF(0, 0, _width, scene()->height());
}

#endif

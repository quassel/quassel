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

#include <QPainter>

#include "markerlineitem.h"
#include "qtui.h"

MarkerLineItem::MarkerLineItem(qreal sceneWidth, QGraphicsItem *parent)
    : QGraphicsObject(parent),
    _boundingRect(0, 0, sceneWidth, 1),
    _chatLine(0)
{
    setVisible(false);
    setZValue(8);
    styleChanged(); // init brush and height
    connect(QtUi::style(), SIGNAL(changed()), SLOT(styleChanged()));
}


void MarkerLineItem::setChatLine(ChatLine *line)
{
    _chatLine = line;
    if (!line)
        setVisible(false);
}


void MarkerLineItem::styleChanged()
{
    _brush = QtUi::style()->brush(UiStyle::MarkerLine);

    // if this is a solid color, we assume 1px because wesurely  don't surely don't want to fill the entire chatline.
    // else, use the height of a single line of text to play around with gradients etc.
    qreal height = 1.;
    if (_brush.style() != Qt::SolidPattern)
        height = QtUi::style()->fontMetrics(QtUiStyle::PlainMsg, 0)->lineSpacing();

    prepareGeometryChange();
    _boundingRect = QRectF(0, 0, scene() ? scene()->width() : 100, height);
}


void MarkerLineItem::sceneRectChanged(const QRectF &rect)
{
    prepareGeometryChange();
    _boundingRect.setWidth(rect.width());
}


void MarkerLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->fillRect(boundingRect(), _brush);
}

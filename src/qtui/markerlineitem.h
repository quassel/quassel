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

#ifndef MARKERLINEITEM_H_
#define MARKERLINEITEM_H_

#include <QGraphicsObject>

#include "chatscene.h"

class ChatLine;

class MarkerLineItem : public QGraphicsObject
{
    Q_OBJECT

public:
    MarkerLineItem(qreal sceneWidth, QGraphicsItem *parent = 0);
    virtual inline int type() const { return ChatScene::MarkerLineType; }

    inline QRectF boundingRect() const { return _boundingRect; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    inline ChatLine *chatLine() const { return _chatLine; }

public slots:
    //! Set the ChatLine this MarkerLineItem is associated to
    void setChatLine(ChatLine *line);
    void sceneRectChanged(const QRectF &);

private slots:
    void styleChanged();

private:
    QRectF _boundingRect;
    QBrush _brush;
    ChatLine *_chatLine;
};


#endif

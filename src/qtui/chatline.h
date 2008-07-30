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

#ifndef CHATLINE_H_
#define CHATLINE_H_

#include <QGraphicsItem>

#include "chatlinemodel.h"

class ChatItem;

class ChatLine : public QGraphicsItem {

  public:
    ChatLine(const QModelIndex &tempIndex, QGraphicsItem *parent = 0);
    virtual ~ChatLine();

    virtual QRectF boundingRect () const;
    inline qreal width() const { return _width; }
    inline qreal height() const { return _height; }
    ChatItem *item(ChatLineModel::ColumnType) const;

    virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    // returns height
    qreal setGeometry(qreal width, qreal firstColPos, qreal secondColPos);
    void setSelected(bool selected, ChatLineModel::ColumnType minColumn = ChatLineModel::ContentsColumn);
    void setHighlighted(bool highlighted);

  protected:

  private:
    ChatItem *_timestampItem, *_senderItem, *_contentsItem;
    qreal _width, _height;

    enum { Selected = 0x40, Highlighted = 0x80 };
    quint8 _selection;  // save space, so we put both the col and the flags into one byte
};

#endif

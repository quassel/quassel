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

#ifndef _CHATITEM_H_
#define _CHATITEM_H_

#include <QGraphicsItem>
#include <QPersistentModelIndex>
#include <QTextLayout>
#include <QTextOption>

#include "messagemodel.h"
#include "uistyle.h"

class QGraphicsSceneMouseEvent;

class ChatItem : public QGraphicsItem {

  public:
    ChatItem(const QPersistentModelIndex &index, QGraphicsItem *parent = 0);
    virtual ~ChatItem();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    inline QPersistentModelIndex persistentIndex() const { return _index; }
    inline const MessageModel *model() const { return _index.isValid() ? qobject_cast<const MessageModel *>(_index.model()) : 0; }
    inline int row() const { return _index.isValid() ? _index.row() : 0; }
    QVariant data(int role) const;
    //QString text() const;
    //void setText(const UiStyle::StyledText &text);

    //QTextOption textOption() const;
    //void setTextOption(const QTextOption &option);

    //void setWidth(int width);
    //virtual void layout();

  protected:
    //void mouseMoveEvent ( QGraphicsSceneMouseEvent * event );

  private:
    //int _width;
    //QTextLayout _layout;
    //QTextOption _textOption;
    QPersistentModelIndex _index;
};

#endif

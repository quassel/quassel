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
#include "chatitem.h"

class ChatLine : public QGraphicsItem {

public:
  ChatLine(int row, QAbstractItemModel *model, QGraphicsItem *parent = 0);

  virtual QRectF boundingRect () const;

  inline int row() { return _row; }
  inline void setRow(int row) { _row = row; }
  inline const QAbstractItemModel *model() const { return chatScene() ? chatScene()->model() : 0; }
  inline ChatScene *chatScene() const { return qobject_cast<ChatScene *>(scene()); }
  inline qreal width() const { return _width; }
  inline qreal height() const { return _height; }
  ChatItem &item(ChatLineModel::ColumnType);

  virtual void paint (QPainter * painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

  // returns height
  qreal setGeometry(qreal width, qreal firstColPos, qreal secondColPos);
  void setSelected(bool selected, ChatLineModel::ColumnType minColumn = ChatLineModel::ContentsColumn);
  void setHighlighted(bool highlighted);

protected:

private:
  int _row;
  TimestampChatItem _timestampItem;
  SenderChatItem _senderItem;
  ContentsChatItem _contentsItem;
  qreal _width, _height;

  enum { Selected = 0x40, Highlighted = 0x80 };
  quint8 _selection;  // save space, so we put both the col and the flags into one byte
};

#endif

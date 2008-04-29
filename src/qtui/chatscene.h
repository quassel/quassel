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

#ifndef _CHATSCENE_H_
#define _CHATSCENE_H_

#include <QGraphicsScene>

#include "messagemodel.h"

class AbstractUiMsg;
class Buffer;
class ChatItem;
class ChatLine;
class QGraphicsSceneMouseEvent;

class ChatScene : public QGraphicsScene {
  Q_OBJECT

  public:
    ChatScene(MessageModel *model, QObject *parent);
    virtual ~ChatScene();

    Buffer *buffer() const;
    inline MessageModel *model() const { return _model; }

  public slots:

  protected slots:

    void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );

  private:
    //Buffer *_buffer;
    //QList<ChatLine*> _lines;
    MessageModel *_model;
    QList<ChatItem *> _items;

};

#endif

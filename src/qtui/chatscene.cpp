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

#include <QGraphicsSceneMouseEvent>
#include <QPersistentModelIndex>

#include "buffer.h"
#include "chatitem.h"
#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "quasselui.h"

ChatScene::ChatScene(MessageModel *model, QObject *parent) : QGraphicsScene(parent), _model(model) {
  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  for(int i = 0; i < model->rowCount(); i++) {
    ChatItem *item = new ChatItem(QPersistentModelIndex(model->index(i, 2)));
    addItem(item);
    item->setPos(30, i*item->boundingRect().height());
  }

  
}

ChatScene::~ChatScene() {


}


void ChatScene::mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent ) {
  /*
  qDebug() << "recv" << mouseEvent->scenePos();
  ChatLine *line = static_cast<ChatLine*>(itemAt(mouseEvent->scenePos()));
  ChatItem *item = static_cast<ChatItem*>(itemAt(mouseEvent->scenePos()));
  qDebug() << (void*)line << (void*)item;
  if(line) {
    line->myMousePressEvent(mouseEvent);
  } else  QGraphicsScene::mousePressEvent(mouseEvent);
  */
}

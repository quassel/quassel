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

#include "buffer.h"
#include "chatitem.h"
#include "chatline.h"
#include "chatscene.h"
#include "quasselui.h"

ChatScene::ChatScene(Buffer *buf, QObject *parent) : QGraphicsScene(parent) {
  _buffer = buf;

  foreach(AbstractUiMsg *msg, buf->contents()) {
    appendMsg(msg);
  }
  connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), this, SLOT(appendMsg(AbstractUiMsg *)));
  connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), this, SLOT(prependMsg(AbstractUiMsg *)));
}

ChatScene::~ChatScene() {


}

void ChatScene::appendMsg(AbstractUiMsg * msg) {
  ChatLine *line = dynamic_cast<ChatLine*>(msg);
  Q_ASSERT(line);
  _lines.append(line);
  addItem(line);
  line->setPos(0, _lines.count() * 30);
  line->setColumnWidths(80, 80, 400);
}

void ChatScene::prependMsg(AbstractUiMsg * msg) {
  ChatLine *line = dynamic_cast<ChatLine*>(msg);
  Q_ASSERT(line); qDebug() << "prepending";
  _lines.prepend(line);
  addItem(line);
  line->setPos(0, _lines.count() * 30);
}

void ChatScene::mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent ) {
  qDebug() << "recv" << mouseEvent->scenePos();
  ChatLine *line = static_cast<ChatLine*>(itemAt(mouseEvent->scenePos()));
  ChatItem *item = static_cast<ChatItem*>(itemAt(mouseEvent->scenePos()));
  qDebug() << (void*)line << (void*)item;
  if(line) {
    line->myMousePressEvent(mouseEvent);
  } else  QGraphicsScene::mousePressEvent(mouseEvent);
}

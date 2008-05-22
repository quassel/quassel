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
  _width = 0;
  _timestampWidth = 60;
  _senderWidth = 80;
  connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int, int)));
  for(int i = 0; i < model->rowCount(); i++) {
    ChatLine *line = new ChatLine(model->index(i, 0));
    _lines.append(line);
    addItem(line);
  }
  emit heightChanged(height());
}

ChatScene::~ChatScene() {


}

void ChatScene::rowsInserted(const QModelIndex &index, int start, int end) {
  Q_UNUSED(index);
  // maybe make this more efficient by prepending stuff with negative yval
  // dunno if that's worth not guranteeing that 0 is on the top...
  // TODO bulk inserts, iterators
  int h = 0;
  int y = 0;
  if(_width && start > 0) y = _lines.value(start - 1)->y() + _lines.value(start - 1)->height();
  for(int i = start; i <= end; i++) {
    ChatLine *line = new ChatLine(model()->index(i, 0));
    _lines.insert(i, line);
    addItem(line);
    if(_width > 0) {
      line->setPos(0, y+h);
      h += line->setColumnWidths(_timestampWidth, _senderWidth, _width - _timestampWidth - _senderWidth);
    }
  }
  if(h > 0) {
    _height += h;
    for(int i = end+1; i < _lines.count(); i++) {
      _lines.value(i)->moveBy(0, h);
    }
    emit heightChanged(height());
  }
}

void ChatScene::setWidth(int w) {
  _width = w;
  _height = 0;
  foreach(ChatLine *line, _lines) {
    line->setPos(0, _height);
    _height += line->setColumnWidths(_timestampWidth, _senderWidth, w - _timestampWidth - _senderWidth);
  }
  emit heightChanged(_height);
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

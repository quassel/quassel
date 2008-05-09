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

#include <QGraphicsTextItem>

#include "buffer.h"
#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "chatview.h"
#include "client.h"
#include "quasselui.h"

ChatView::ChatView(Buffer *buf, QWidget *parent) : QGraphicsView(parent), AbstractChatView() {
  _scene = new ChatScene(Client::messageModel(), this);
  setScene(_scene);

  //QGraphicsTextItem *item = scene()->addText(buf->bufferInfo().bufferName());

}


ChatView::~ChatView() {

}


ChatScene *ChatView::scene() const {
  return _scene;
}


void ChatView::clear()
{
}

void ChatView::prependMsg(AbstractUiMsg *msg) {
  //ChatLine *line = dynamic_cast<ChatLine*>(msg);
  //Q_ASSERT(line);
  //prependChatLine(line);
}

void ChatView::prependChatLine(ChatLine *line) {
  //qDebug() << "prepending";
}

void ChatView::prependChatLines(QList<ChatLine *> clist) {

}

void ChatView::appendMsg(AbstractUiMsg *msg) {
  //ChatLine *line = dynamic_cast<ChatLine*>(msg);
  //Q_ASSERT(line);
  //appendChatLine(line);
}

void ChatView::appendChatLine(ChatLine *line) {
  //qDebug() << "appending";
}


void ChatView::appendChatLines(QList<ChatLine *> list) {
  //foreach(ChatLine *line, list) {
    
  //}
}

void ChatView::setContents(const QList<AbstractUiMsg *> &list) {
  //qDebug() << "setting" << list.count();
  //appendChatLines(list);
}


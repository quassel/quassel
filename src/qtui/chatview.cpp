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
#include <QScrollBar>

#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "chatview.h"
#include "client.h"
#include "messagefilter.h"
#include "quasselui.h"

ChatView::ChatView(BufferId bufferId, QWidget *parent)
  : QGraphicsView(parent),
    AbstractChatView()
{
  QList<BufferId> filterList;
  filterList.append(bufferId);
  MessageFilter *filter = new MessageFilter(Client::messageModel(), filterList, this);
  init(filter);
}

ChatView::ChatView(MessageFilter *filter, QWidget *parent)
  : QGraphicsView(parent),
    AbstractChatView()
{
  init(filter);
}

void ChatView::init(MessageFilter *filter) {
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setAlignment(Qt::AlignBottom);
  setInteractive(true);

  _scene = new ChatScene(filter, filter->idString(), this);
  connect(_scene, SIGNAL(heightChangedAt(qreal, qreal)), this, SLOT(sceneHeightChangedAt(qreal, qreal)));
  setScene(_scene);

  _lastScrollbarPos = 0;
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(verticalScrollbarChanged(int)));
}

void ChatView::resizeEvent(QResizeEvent *event) {
  scene()->setWidth(event->size().width() - 2);  // FIXME figure out why we have to hardcode the -2 here
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void ChatView::sceneHeightChangedAt(qreal ypos, qreal hdiff) {
  setSceneRect(scene()->sceneRect());
  int y = mapFromScene(0, ypos).y();
  if(y <= viewport()->height() + 2) {  // be a bit tolerant here, also FIXME (why we need the 2px?)
    verticalScrollBar()->setValue(verticalScrollBar()->value() + hdiff);
  }
}

void ChatView::verticalScrollbarChanged(int newPos) {
  QAbstractSlider *vbar = verticalScrollBar();
  Q_ASSERT(vbar);
  
  // FIXME dirty hack to battle the "I just scroll up a pixel on hide()/show()" problem
  if(vbar->maximum() - vbar->value() < 5) vbar->setValue(vbar->maximum()); 

  if(newPos < _lastScrollbarPos) {
    int relativePos = 100;
    if(vbar->maximum() - vbar->minimum() != 0)
      relativePos = (newPos - vbar->minimum()) * 100 / (vbar->maximum() - vbar->minimum());

    if(relativePos < 20) {
      scene()->requestBacklog();
    }
  }
  _lastScrollbarPos = newPos;
}

MsgId ChatView::lastMsgId() const {
  if(!scene())
    return MsgId();

  QAbstractItemModel *model = scene()->model();
  if(!model || model->rowCount() == 0)
    return MsgId();

  return model->data(model->index(model->rowCount() - 1, 0), MessageModel::MsgIdRole).value<MsgId>();
}

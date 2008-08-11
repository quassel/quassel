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

#include "buffer.h"
#include "chatlinemodelitem.h"
#include "chatscene.h"
#include "chatview.h"
#include "client.h"
#include "messagefilter.h"
#include "quasselui.h"

ChatView::ChatView(Buffer *buf, QWidget *parent)
  : QGraphicsView(parent),
    AbstractChatView()
{
  QList<BufferId> filterList;
  filterList.append(buf->bufferInfo().bufferId());
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
  connect(_scene, SIGNAL(heightChanged(qreal)), this, SLOT(sceneHeightChanged(qreal)));
  setScene(_scene);

  connect(verticalScrollBar(), SIGNAL(sliderPressed()), this, SLOT(sliderPressed()));
  connect(verticalScrollBar(), SIGNAL(sliderReleased()), this, SLOT(sliderReleased()));
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(verticalScrollbarChanged(int)));
}

void ChatView::resizeEvent(QResizeEvent *event) {
  scene()->setWidth(event->size().width() - 2);  // FIXME figure out why we have to hardcode the -2 here
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void ChatView::sceneHeightChanged(qreal h) {
  Q_UNUSED(h)
  bool scrollable = qAbs(verticalScrollBar()->value() - verticalScrollBar()->maximum()) <= 2; // be a bit tolerant here, also FIXME (why we need this?)
  setSceneRect(scene()->sceneRect());
  if(scrollable) verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void ChatView::setBufferForBacklogFetching(BufferId id) {
  scene()->setBufferForBacklogFetching(id);
}

void ChatView::sliderPressed() {
  verticalScrollbarChanged(verticalScrollBar()->value());
}

void ChatView::sliderReleased() {
  if(scene()->isFetchingBacklog()) scene()->setIsFetchingBacklog(false);
}

void ChatView::verticalScrollbarChanged(int newPos) {
  Q_UNUSED(newPos);
  if(!scene()->isBacklogFetchingEnabled()) return;

  QAbstractSlider *vbar = verticalScrollBar();
  if(!vbar)
    return;
  if(vbar->isSliderDown()) {
    /*
    int relativePos = 100;
    if(vbar->maximum() - vbar->minimum() != 0)
      relativePos = (newPos - vbar->minimum()) * 100 / (vbar->maximum() - vbar->minimum());
    scene()->setIsFetchingBacklog(relativePos < 20);
    */
    scene()->setIsFetchingBacklog(vbar->value() == vbar->minimum());
  }
}

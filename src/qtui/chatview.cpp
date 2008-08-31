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

  _scene = new ChatScene(filter, filter->idString(), viewport()->width() - 2, this); // see below: resizeEvent()
  connect(_scene, SIGNAL(sceneHeightChanged(qreal)), this, SLOT(sceneHeightChanged(qreal)));
  setScene(_scene);

  _lastScrollbarPos = verticalScrollBar()->maximum();
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(verticalScrollbarChanged(int)));
}

void ChatView::resizeEvent(QResizeEvent *event) {
  QGraphicsView::resizeEvent(event);
  scene()->setWidth(viewport()->width() - 2);  // FIXME figure out why we have to hardcode the -2 here -> Qt-Bug most probably
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void ChatView::sceneHeightChanged(qreal dh) {
  QAbstractSlider *vbar = verticalScrollBar();
  Q_ASSERT(vbar);
  if(vbar->maximum() - vbar->value() <= dh + 5) // in case we had scrolled only about half a line to the bottom we allow a grace of 5
    vbar->setValue(vbar->maximum());
}

void ChatView::verticalScrollbarChanged(int newPos) {
  QAbstractSlider *vbar = verticalScrollBar();
  Q_ASSERT(vbar);

  if(vbar->maximum() - vbar->value() <= 5) // FIXME workaround the fact that the view gets scrolled up a few px on buffer change
    vbar->setValue(vbar->maximum());

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

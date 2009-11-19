/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>

#include "bufferwidget.h"
#include "chatscene.h"
#include "chatview.h"
#include "client.h"
#include "messagefilter.h"
#include "qtui.h"
#include "qtuistyle.h"
#include "clientignorelistmanager.h"

ChatView::ChatView(BufferId bufferId, QWidget *parent)
  : QGraphicsView(parent),
    AbstractChatView(),
    _bufferContainer(0),
    _currentScaleFactor(1),
    _invalidateFilter(false)
{
  QList<BufferId> filterList;
  filterList.append(bufferId);
  MessageFilter *filter = new MessageFilter(Client::messageModel(), filterList, this);
  init(filter);
}

ChatView::ChatView(MessageFilter *filter, QWidget *parent)
  : QGraphicsView(parent),
    AbstractChatView(),
    _bufferContainer(0),
    _currentScaleFactor(1),
    _invalidateFilter(false)
{
  init(filter);
}

void ChatView::init(MessageFilter *filter) {
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setAlignment(Qt::AlignBottom);
  setInteractive(true);
  //setOptimizationFlags(QGraphicsView::DontClipPainter | QGraphicsView::DontAdjustForAntialiasing);
  // setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing);
  setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
  // setTransformationAnchor(QGraphicsView::NoAnchor);
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);

  _scrollTimer.setInterval(100);
  _scrollTimer.setSingleShot(true);
  connect(&_scrollTimer, SIGNAL(timeout()), SLOT(scrollTimerTimeout()));

  _scene = new ChatScene(filter, filter->idString(), viewport()->width() - 4, this); // see below: resizeEvent()
  connect(_scene, SIGNAL(sceneRectChanged(const QRectF &)), this, SLOT(sceneRectChanged(const QRectF &)));
  connect(_scene, SIGNAL(lastLineChanged(QGraphicsItem *, qreal)), this, SLOT(lastLineChanged(QGraphicsItem *, qreal)));
  connect(_scene, SIGNAL(mouseMoveWhileSelecting(const QPointF &)), this, SLOT(mouseMoveWhileSelecting(const QPointF &)));
  setScene(_scene);

  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(verticalScrollbarChanged(int)));

  // only connect if client is synched with a core
  if(Client::isConnected())
    connect(Client::ignoreListManager(), SIGNAL(ignoreListChanged()), this, SLOT(invalidateFilter()));
}

bool ChatView::event(QEvent *event) {
  if(event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    switch(keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
      if(!verticalScrollBar()->isVisible()) {
        scene()->requestBacklog();
        return true;
      }
    default:
      break;
    }
  }

  if(event->type() == QEvent::Wheel) {
    if(!verticalScrollBar()->isVisible()) {
      scene()->requestBacklog();
      return true;
    }
  }

  if(event->type() == QEvent::Show) {
    if(_invalidateFilter)
      invalidateFilter();
  }

  return QGraphicsView::event(event);
}

void ChatView::resizeEvent(QResizeEvent *event) {
  QGraphicsView::resizeEvent(event);

  // we can reduce viewport updates if we scroll to the bottom allready at the beginning
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());

  // FIXME: without the hardcoded -4 Qt reserves space for a horizontal scrollbar even though it's disabled permanently.
  // this does only occur on QtX11 (at least not on Qt for Mac OS). Seems like a Qt Bug.
  scene()->updateForViewport(viewport()->width() - 4, viewport()->height());

  _lastScrollbarPos = verticalScrollBar()->maximum();
  verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void ChatView::mouseMoveWhileSelecting(const QPointF &scenePos) {
  int y = (int)mapFromScene(scenePos).y();
  _scrollOffset = 0;
  if(y < 0)
    _scrollOffset = y;
  else if(y > height())
    _scrollOffset = y - height();

  if(_scrollOffset && !_scrollTimer.isActive())
    _scrollTimer.start();
}

void ChatView::scrollTimerTimeout() {
  // scroll view
  QAbstractSlider *vbar = verticalScrollBar();
  if(_scrollOffset < 0 && vbar->value() > 0)
    vbar->setValue(qMax(vbar->value() + _scrollOffset, 0));
  else if(_scrollOffset > 0 && vbar->value() < vbar->maximum())
    vbar->setValue(qMin(vbar->value() + _scrollOffset, vbar->maximum()));
}

void ChatView::lastLineChanged(QGraphicsItem *chatLine, qreal offset) {
  Q_UNUSED(chatLine)
  // disabled until further testing/discussion
  //if(!scene()->isScrollingAllowed())
  //  return;

  QAbstractSlider *vbar = verticalScrollBar();
  Q_ASSERT(vbar);
  if(vbar->maximum() - vbar->value() <= (offset + 5) * _currentScaleFactor ) { // 5px grace area
    vbar->setValue(vbar->maximum());
  }
}

void ChatView::verticalScrollbarChanged(int newPos) {
  QAbstractSlider *vbar = verticalScrollBar();
  Q_ASSERT(vbar);

  // check for backlog request
  if(newPos < _lastScrollbarPos) {
    int relativePos = 100;
    if(vbar->maximum() - vbar->minimum() != 0)
      relativePos = (newPos - vbar->minimum()) * 100 / (vbar->maximum() - vbar->minimum());

    if(relativePos < 20) {
      scene()->requestBacklog();
    }
  }
  _lastScrollbarPos = newPos;

  // FIXME: Fugly workaround for the ChatView scrolling up 1px on buffer switch
  if(vbar->maximum() - newPos <= 2)
    vbar->setValue(vbar->maximum());
}

MsgId ChatView::lastMsgId() const {
  if(!scene())
    return MsgId();

  QAbstractItemModel *model = scene()->model();
  if(!model || model->rowCount() == 0)
    return MsgId();

  return model->data(model->index(model->rowCount() - 1, 0), MessageModel::MsgIdRole).value<MsgId>();
}

void ChatView::addActionsToMenu(QMenu *menu, const QPointF &pos) {
  // zoom actions
  BufferWidget *bw = qobject_cast<BufferWidget *>(bufferContainer());
  if(bw) {
    bw->addActionsToMenu(menu, pos);
    menu->addSeparator();
  }
}

void ChatView::zoomIn() {
    _currentScaleFactor *= 1.2;
    scale(1.2, 1.2);
    scene()->setWidth(viewport()->width() / _currentScaleFactor - 2);
}

void ChatView::zoomOut() {
    _currentScaleFactor /= 1.2;
    scale(1 / 1.2, 1 / 1.2);
    scene()->setWidth(viewport()->width() / _currentScaleFactor - 2);
}

void ChatView::zoomOriginal() {
    scale(1/_currentScaleFactor, 1/_currentScaleFactor);
    _currentScaleFactor = 1;
    scene()->setWidth(viewport()->width() - 2);
}

void ChatView::invalidateFilter() {
  // if this is the currently selected chatview
  // invalidate immediately
  if(isVisible()) {
    _scene->filter()->invalidateFilter();
    _invalidateFilter = false;
  }
  // otherwise invalidate whenever the view is shown
  else {
    _invalidateFilter = true;
  }
}

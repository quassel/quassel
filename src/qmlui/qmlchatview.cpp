/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#include <QAbstractItemModel>
#include <QDeclarativeContext>
#include <QKeyEvent>
#include <QScrollBar>

//#include "bufferwidget.h"
#include "client.h"
#include "clientignorelistmanager.h"
#include "messagefilter.h"
#include "networkmodel.h"
#include "qmlchatview.h"

QmlChatView::QmlChatView(BufferId bufferId, QWidget *parent)
  : QDeclarativeView(parent),
    AbstractChatView()
{
  QList<BufferId> filterList;
  filterList.append(bufferId);
  MessageFilter *filter = new MessageFilter(Client::messageModel(), filterList, this);
  init(filter);
}

QmlChatView::QmlChatView(MessageFilter *filter, QWidget *parent)
  : QDeclarativeView(parent),
    AbstractChatView()
{
  init(filter);
}

void QmlChatView::init(MessageFilter *filter) {
  _bufferContainer = 0;
  _invalidateFilter = false;

  setResizeMode(SizeRootObjectToView);

  QDeclarativeContext *ctxt = rootContext();
  ctxt->setContextProperty("msgModel", filter);

  setSource(QUrl("qrc:/qml/ChatView.qml"));

  //connect(Client::networkModel(), SIGNAL(markerLineSet(BufferId,MsgId)), SLOT(markerLineSet(BufferId,MsgId)));

  // only connect if client is synched with a core
  if(Client::isConnected())
    connect(Client::ignoreListManager(), SIGNAL(ignoreListChanged()), SLOT(invalidateFilter()));
}

bool QmlChatView::event(QEvent *event) {
  if(event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    switch(keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
      if(!verticalScrollBar()->isVisible()) {
//        scene()->requestBacklog();
        return true;
      }
    default:
      break;
    }
  }

  if(event->type() == QEvent::Wheel) {
    if(!verticalScrollBar()->isVisible()) {
//      scene()->requestBacklog();
      return true;
    }
  }

  if(event->type() == QEvent::Show) {
    if(_invalidateFilter)
      invalidateFilter();
  }

  return QGraphicsView::event(event);
}

MsgId QmlChatView::lastMsgId() const {
/*
  if(!scene())
    return MsgId();

  QAbstractItemModel *model = scene()->model();
  if(!model || model->rowCount() == 0)
    return MsgId();

  return model->index(model->rowCount() - 1, 0).data(MessageModel::MsgIdRole).value<MsgId>();
*/
  return 0;
}
/*
void QmlChatView::setMarkerLineVisible(bool visible) {
  scene()->setMarkerLineVisible(visible);
}

void QmlChatView::setMarkerLine(MsgId msgId) {
  if(!scene()->isSingleBufferScene())
    return;

  BufferId bufId = scene()->singleBufferId();
  Client::setMarkerLine(bufId, msgId);
}

void QmlChatView::markerLineSet(BufferId buffer, MsgId msgId) {
  if(!scene()->isSingleBufferScene() || scene()->singleBufferId() != buffer)
    return;

  scene()->setMarkerLine(msgId);
  scene()->setMarkerLineVisible(true);
}

void QmlChatView::jumpToMarkerLine(bool requestBacklog) {
  scene()->jumpToMarkerLine(requestBacklog);
}
*/
void QmlChatView::addActionsToMenu(QMenu *menu, const QPointF &pos) {
/*
  // zoom actions
  BufferWidget *bw = qobject_cast<BufferWidget *>(bufferContainer());
  if(bw) {
    bw->addActionsToMenu(menu, pos);
    menu->addSeparator();
  }
*/
}

void QmlChatView::invalidateFilter() {
  // if this is the currently selected QmlChatView
  // invalidate immediately
  if(isVisible()) {
    //_scene->filter()->invalidateFilter();
    _invalidateFilter = false;
  }
  // otherwise invalidate whenever the view is shown
  else {
    _invalidateFilter = true;
  }
}

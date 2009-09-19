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

#ifndef CHATVIEW_H_
#define CHATVIEW_H_

#include <QGraphicsView>
#include <QTimer>

#include "abstractbuffercontainer.h"

class AbstractBufferContainer;
class AbstractUiMsg;
class Buffer;
class ChatLine;
class ChatScene;
class MessageFilter;
class QMenu;

class ChatView : public QGraphicsView, public AbstractChatView {
  Q_OBJECT

public:
  ChatView(MessageFilter *, QWidget *parent = 0);
  ChatView(BufferId bufferId, QWidget *parent = 0);

  virtual MsgId lastMsgId() const;
  inline AbstractBufferContainer *bufferContainer() const { return _bufferContainer; }
  inline void setBufferContainer(AbstractBufferContainer *c) { _bufferContainer = c; }

  inline ChatScene *scene() const { return _scene; }

  virtual void addActionsToMenu(QMenu *, const QPointF &pos);

  virtual bool event(QEvent *event);

public slots:
  inline virtual void clear() {}
  void zoomIn();
  void zoomOut();
  void zoomOriginal();

protected:
  virtual void resizeEvent(QResizeEvent *event);

protected slots:
  virtual inline void sceneRectChanged(const QRectF &rect) { setSceneRect(rect); }
  virtual void verticalScrollbarChanged(int);

private slots:
  void lastLineChanged(QGraphicsItem *chatLine, qreal offset);
  void mouseMoveWhileSelecting(const QPointF &scenePos);
  void scrollTimerTimeout();
  void invalidateFilter();

private:
  void init(MessageFilter *filter);

  AbstractBufferContainer *_bufferContainer;
  ChatScene *_scene;
  int _lastScrollbarPos;
  qreal _currentScaleFactor;
  QTimer _scrollTimer;
  int _scrollOffset;
  bool _invalidateFilter;
};


#endif

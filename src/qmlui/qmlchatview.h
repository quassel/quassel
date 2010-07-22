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

#ifndef QMLCHATVIEW_H_
#define QMLCHATVIEW_H_

#include <QDeclarativeView>

#include "abstractbuffercontainer.h"

class AbstractBufferContainer;
class AbstractUiMsg;
class Buffer;
class MessageFilter;
class QMenu;

class QmlChatView : public QDeclarativeView, public AbstractChatView {
  Q_OBJECT

public:
  QmlChatView(MessageFilter *, QWidget *parent = 0);
  QmlChatView(BufferId bufferId, QWidget *parent = 0);

  virtual MsgId lastMsgId() const;
//  virtual MsgId lastVisibleMsgId() const;
  inline AbstractBufferContainer *bufferContainer() const { return _bufferContainer; }
  inline void setBufferContainer(AbstractBufferContainer *c) { _bufferContainer = c; }

  virtual void addActionsToMenu(QMenu *, const QPointF &pos);

public slots:
  inline virtual void clear() {}

//  void setMarkerLineVisible(bool visible = true);
//  void setMarkerLine(MsgId msgId);
//  void jumpToMarkerLine(bool requestBacklog);

protected:
  virtual bool event(QEvent *event);
//  virtual void resizeEvent(QResizeEvent *event);
//  virtual void scrollContentsBy(int dx, int dy);

protected slots:
//  virtual void verticalScrollbarChanged(int);

private slots:
//  void lastLineChanged(QGraphicsItem *chatLine, qreal offset);
//  void adjustSceneRect();
//  void checkChatLineCaches();
//  void mouseMoveWhileSelecting(const QPointF &scenePos);
//  void scrollTimerTimeout();
  void invalidateFilter();
//  void markerLineSet(BufferId buffer, MsgId msg);

private:
  void init(MessageFilter *filter);

  AbstractBufferContainer *_bufferContainer;
  bool _invalidateFilter;
};


#endif

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

#ifndef BUFFERWIDGET_H_
#define BUFFERWIDGET_H_

#include "ui_bufferwidget.h"

#include "abstractbuffercontainer.h"

class QGraphicsItem;
class ChatViewSearchBar;
class ChatViewSearchController;

class BufferWidget : public AbstractBufferContainer {
  Q_OBJECT

public:
  BufferWidget(QWidget *parent);
  ~BufferWidget();

  virtual bool eventFilter(QObject *watched, QEvent *event);

  inline ChatViewSearchBar *searchBar() const { return ui.searchBar; }

protected:
  virtual AbstractChatView *createChatView(BufferId);
  virtual void removeChatView(BufferId);

protected slots:
  virtual void showChatView(BufferId);

private slots:
  void scrollToHighlight(QGraphicsItem *highlightItem);
  void zoomIn();
  void zoomOut();
  void zoomNormal();

private:
  Ui::BufferWidget ui;
  QHash<BufferId, QWidget *> _chatViews;

  ChatViewSearchController *_chatViewSearchController;
};

#endif

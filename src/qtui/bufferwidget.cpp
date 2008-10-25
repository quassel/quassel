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

#include "bufferwidget.h"
#include "chatview.h"
#include "chatviewsearchbar.h"
#include "chatviewsearchcontroller.h"
#include "settings.h"
#include "client.h"

#include "action.h"
#include "actioncollection.h"
#include "qtui.h"

#include <QLayout>
#include <QKeyEvent>
#include <QScrollBar>

BufferWidget::BufferWidget(QWidget *parent)
  : AbstractBufferContainer(parent),
    _chatViewSearchController(new ChatViewSearchController(this))
{
  ui.setupUi(this);
  layout()->setContentsMargins(0, 0, 0, 0);
  layout()->setSpacing(0);
  // ui.searchBar->hide();

  _chatViewSearchController->setCaseSensitive(ui.searchBar->caseSensitiveBox()->isChecked());
  _chatViewSearchController->setSearchSenders(ui.searchBar->searchSendersBox()->isChecked());
  _chatViewSearchController->setSearchMsgs(ui.searchBar->searchMsgsBox()->isChecked());
  _chatViewSearchController->setSearchOnlyRegularMsgs(ui.searchBar->searchOnlyRegularMsgsBox()->isChecked());

  connect(ui.searchBar->searchEditLine(), SIGNAL(textChanged(const QString &)),
	  _chatViewSearchController, SLOT(setSearchString(const QString &)));
  connect(ui.searchBar->caseSensitiveBox(), SIGNAL(toggled(bool)),
	  _chatViewSearchController, SLOT(setCaseSensitive(bool)));
  connect(ui.searchBar->searchSendersBox(), SIGNAL(toggled(bool)),
	  _chatViewSearchController, SLOT(setSearchSenders(bool)));
  connect(ui.searchBar->searchMsgsBox(), SIGNAL(toggled(bool)),
	  _chatViewSearchController, SLOT(setSearchMsgs(bool)));
  connect(ui.searchBar->searchOnlyRegularMsgsBox(), SIGNAL(toggled(bool)),
	  _chatViewSearchController, SLOT(setSearchOnlyRegularMsgs(bool)));
  connect(ui.searchBar->searchUpButton(), SIGNAL(clicked()),
	  _chatViewSearchController, SLOT(highlightPrev()));
  connect(ui.searchBar->searchDownButton(), SIGNAL(clicked()),
	  _chatViewSearchController, SLOT(highlightNext()));

  connect(_chatViewSearchController, SIGNAL(newCurrentHighlight(QGraphicsItem *)),
	  this, SLOT(scrollToHighlight(QGraphicsItem *)));
  
  ActionCollection *coll = QtUi::actionCollection();

  Action *zoomChatview = coll->add<Action>("ZoomChatView");
  connect(zoomChatview, SIGNAL(triggered()), SLOT(zoomIn()));
  zoomChatview->setText(tr("Enlarge Chat View"));
  zoomChatview->setShortcut(tr("Ctrl++"));

  Action *zoomOutChatview = coll->add<Action>("ZoomOutChatView");
  connect(zoomOutChatview, SIGNAL(triggered()), SLOT(zoomOut()));
  zoomOutChatview->setText(tr("Demagnify Chat View"));
  zoomOutChatview->setShortcut(tr("Ctrl+-"));

  Action *zoomNormalChatview = coll->add<Action>("ZoomNormalChatView");
  connect(zoomNormalChatview, SIGNAL(triggered()), SLOT(zoomNormal()));
  zoomNormalChatview->setText(tr("Normalize zoom of Chat View"));
  zoomNormalChatview->setShortcut(tr("Ctrl+0"));
}

BufferWidget::~BufferWidget() {
  delete _chatViewSearchController;
  _chatViewSearchController = 0;
}

AbstractChatView *BufferWidget::createChatView(BufferId id) {
  ChatView *chatView;
  chatView = new ChatView(id, this);
  _chatViews[id] = chatView;
  ui.stackedWidget->addWidget(chatView);
  chatView->setFocusProxy(this);
  return chatView;
}

void BufferWidget::removeChatView(BufferId id) {
  QWidget *view = _chatViews.value(id, 0);
  if(!view) return;
  ui.stackedWidget->removeWidget(view);
  view->deleteLater();
  _chatViews.take(id);
}

void BufferWidget::showChatView(BufferId id) {
  if(!id.isValid()) {
    ui.stackedWidget->setCurrentWidget(ui.page);
  } else {
    ChatView *view = qobject_cast<ChatView *>(_chatViews.value(id));
    Q_ASSERT(view);
    ui.stackedWidget->setCurrentWidget(view);
    _chatViewSearchController->setScene(view->scene());
  }
}

void BufferWidget::scrollToHighlight(QGraphicsItem *highlightItem) {
  ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
  if(view) {
    view->centerOn(highlightItem);
  }
}


void BufferWidget::zoomIn() {
  ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
  if(!view) return;
  view->zoomIn();
}

void BufferWidget::zoomOut() {
  ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
  if(!view) return;
  view->zoomOut();
}

void BufferWidget::zoomNormal() {
  ChatView *view = qobject_cast<ChatView *>(ui.stackedWidget->currentWidget());
  if(!view) return;
  view->zoomNormal();
}

bool BufferWidget::eventFilter(QObject *watched, QEvent *event) {
  Q_UNUSED(watched);
  if(event->type() != QEvent::KeyPress)
    return false;

  QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

  int direction = 1;
  switch(keyEvent->key()) {
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
      // static cast to access public qobject::event
      return static_cast<QObject*>(ui.stackedWidget->currentWidget())->event(event);

    case Qt::Key_Up:
      direction = -1;
    case Qt::Key_Down:
      if(keyEvent->modifiers() == Qt::ShiftModifier) {
        QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea*>(ui.stackedWidget->currentWidget());
        if(!scrollArea)
          return false;
        int sliderPosition = scrollArea->verticalScrollBar()->value();
        scrollArea->verticalScrollBar()->setValue(sliderPosition + (direction * 12));
      }
    default:
      return false;
  }
}

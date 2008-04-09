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

#include "mainwidget.h"

#include "buffer.h"
#include "chatwidget.h"
#include "client.h"

MainWidget::MainWidget(QWidget *parent) : AbstractBufferContainer(parent) {
  ui.setupUi(this);
  ui.inputLine->hide(); ui.topicBar->hide();
  connect(ui.inputLine, SIGNAL(sendText(const QString &)), this, SLOT(userInput(const QString &)));
  connect(this, SIGNAL(userInput(BufferInfo, QString)), Client::instance(), SIGNAL(sendInput(BufferInfo, QString)));
}

MainWidget::~MainWidget() {



}

AbstractChatView *MainWidget::createChatView(BufferId id) {
  Q_UNUSED(id)
  ChatWidget *widget = new ChatWidget(this);
  AbstractChatView *chatView = static_cast<AbstractChatView *>(widget); // can't use dynamic_cast on some Qtopia devices
  Q_ASSERT(chatView);
  _chatViews[id] = widget;
  ui.stack->addWidget(widget);
  widget->setFocusProxy(this);
  return chatView;
}

void MainWidget::removeChatView(BufferId id) {
  ChatWidget *view = _chatViews.value(id, 0);
  if(!view) return;
  ui.stack->removeWidget(view);
  view->deleteLater();
}

void MainWidget::showChatView(BufferId id) {
  if(id.isValid()) currentBufferInfo = Client::buffer(id)->bufferInfo();
  else currentBufferInfo = BufferInfo();
  ChatWidget *widget = _chatViews.value(id, 0);
  if(!widget) ui.stack->setCurrentIndex(0);
  else {
    ui.stack->setCurrentWidget(widget);
    ui.inputLine->show(); ui.topicBar->show();
    ui.inputLine->setFocus();
  }
}


/*
void MainWidget::setBuffer(Buffer *buf) {

  if(!buf) {
    ui.stack->setCurrentIndex(0);
    currentBuffer = 0;
    return;
  }
  //  TODO update topic if changed; handle status buffer display
//  QString title = QString("%1 (%2): \"%3\"").arg(buf->bufferInfo().bufferName()).arg(buf->bufferInfo().networkName()).arg(buf->topic());
  QString title = "foobar";
  ui.topicBar->setContents(title);

  //ui.chatWidget->setStyleSheet("div { color: #777777; }");
  //ui.chatWidget->setHtml("<style type=\"text/css\">.foo { color: #777777; } .bar { font-style: italic }</style>"
  //                       "<div class=\"foo\">foo</div> <div class=\"bar\">bar</div> baz");
  //ui.chatWidget->moveCursor(QTextCursor::End);
  //ui.chatWidget->insertHtml("<div class=\"foo\"> brumm</div>");

  ChatWidget *chatWidget;
  if(!chatWidgets.contains(buf)) {
    chatWidget = new ChatWidget(this);
    QList<ChatLine *> lines;
    QList<AbstractUiMsg *> msgs = buf->contents();
    foreach(AbstractUiMsg *msg, msgs) {
      lines.append((ChatLine *)(msg));
    }
    chatWidget->setContents(lines);
    connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), chatWidget, SLOT(appendMsg(AbstractUiMsg *)));
    connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), chatWidget, SLOT(prependMsg(AbstractUiMsg *)));
    connect(buf, SIGNAL(topicSet(QString)), this, SLOT(setTopic(QString)));
    //connect(buf, SIGNAL(ownNickSet(QString)), this, SLOT(setOwnNick(QString)));
    ui.stack->addWidget(chatWidget);
    chatWidgets.insert(buf, chatWidget);
    chatWidget->setFocusProxy(ui.inputLine);
  } else chatWidget = chatWidgets[buf];
  ui.inputLine->show(); ui.topicBar->show();
  ui.stack->setCurrentWidget(chatWidget);
  ui.inputLine->setFocus();
  currentBuffer = buf;
  
}
*/

void MainWidget::userInput(const QString &input) {
  if(!currentBufferInfo.isValid()) return;
  QStringList lines = input.split('\n', QString::SkipEmptyParts);
  foreach(QString msg, lines) {
    if(msg.isEmpty()) continue;
    emit userInput(currentBufferInfo, msg);
  }
  ui.inputLine->clear();
}

/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

MainWidget::MainWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  ui.inputLine->hide(); ui.topicBar->hide();
  connect(ui.inputLine, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  currentBuffer = 0;
}

MainWidget::~MainWidget() {



}

void MainWidget::setBuffer(Buffer *buf) {
  //  TODO update topic if changed; handle status buffer display
  QString title = QString("%1 (%2): \"%3\"").arg(buf->name()).arg(buf->networkName()).arg(buf->topic());
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
      lines.append((ChatLine*)(msg));
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

void MainWidget::enterPressed() {
  QStringList lines = ui.inputLine->text().split('\n', QString::SkipEmptyParts);
  foreach(QString msg, lines) {
    if(msg.isEmpty()) continue;
    if(currentBuffer) currentBuffer->processUserInput(msg);
  }
  ui.inputLine->clear();
}

// FIXME make this more elegant, we don't need to send a string around...
void MainWidget::setTopic(QString topic) {
  Q_UNUSED(topic);
  if(currentBuffer) {
    QString title = QString("%1 (%2): \"%3\"").arg(currentBuffer->name()).arg(currentBuffer->networkName()).arg(currentBuffer->topic());
    ui.topicBar->setContents(title);
  }
}

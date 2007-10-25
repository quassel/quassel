/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#include "buffer.h"
#include "chatline.h"
#include "chatwidget.h"
#include "settings.h"

BufferWidget::BufferWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
}

void BufferWidget::init() {
}

BufferWidget::~BufferWidget() {
}

void BufferWidget::setBuffer(Buffer *buf) {
  ChatWidget *chatWidget;
  if(_chatWidgets.contains(buf->uid())) {
     chatWidget = _chatWidgets[buf->uid()];
  } else {
    chatWidget = new ChatWidget(this);
    chatWidget->init(networkName, bufferName);
    QList<ChatLine *> lines;
    QList<AbstractUiMsg *> msgs = buf->contents();
    foreach(AbstractUiMsg *msg, msgs) {
      lines.append(dynamic_cast<ChatLine*>(msg));
    }
    chatWidget->setContents(lines);
    connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), chatWidget, SLOT(appendMsg(AbstractUiMsg *)));
    connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), chatWidget, SLOT(prependMsg(AbstractUiMsg *)));
    _chatWidgets[buf->uid()] = chatWidget;
    ui.stackedWidget->addWidget(chatWidget);
  }
  ui.stackedWidget->setCurrentWidget(chatWidget);
  disconnect(this, SIGNAL(userInput(QString)), 0, 0);
  connect(this, SIGNAL(userInput(QString)), buf, SLOT(processUserInput(QString)));
  chatWidget->setFocusProxy(ui.inputEdit);
  ui.inputEdit->setFocus();
  ui.ownNick->clear();  // TODO add nick history
  // ui.ownNick->addItem(state->ownNick);
}

void BufferWidget::saveState() {
}

QSize BufferWidget::sizeHint() const {
  return QSize(800,400);
}

void BufferWidget::enterPressed() {
  QStringList lines = ui.inputEdit->text().split('\n', QString::SkipEmptyParts);
  foreach(QString msg, lines) {
    if(msg.isEmpty()) continue;
    emit userInput(msg);
  }
  ui.inputEdit->clear();
}

void BufferWidget::setActive(bool act) {
  if(act != active) {
    active = act;
    //renderContents();
    //scrollToEnd();
  }
}


/*
void BufferWidget::displayMsg(Message msg) {
  chatWidget->appendMsg(msg);
}
*/


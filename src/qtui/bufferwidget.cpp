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
#include "buffer.h"
#include "chatline-old.h"
#include "chatwidget.h"
#include "settings.h"
#include "client.h"

BufferWidget::BufferWidget(QWidget *parent)
  : QWidget(parent),
    _currentBuffer(0)
{
  ui.setupUi(this);
  ui.ownNick->clear();  // TODO add nick history
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
}

void BufferWidget::init() {

}

BufferWidget::~BufferWidget() {
}

BufferId BufferWidget::currentBuffer() const {
  return _currentBuffer;
}

void BufferWidget::setCurrentBuffer(BufferId bufferId) {
  ChatWidget *chatWidget;
  if(_chatWidgets.contains(bufferId)) {
     chatWidget = _chatWidgets[bufferId];
  } else {
    Buffer *buf = Client::buffer(bufferId);
    if(!buf) {
      qWarning() << "BufferWidget::setBuffer(BufferId): Can't show unknown Buffer:" << bufferId;
      return;
    }
    chatWidget = new ChatWidget(this);
    chatWidget->init(bufferId);
    QList<ChatLine *> lines;
    QList<AbstractUiMsg *> msgs = buf->contents();
    foreach(AbstractUiMsg *msg, msgs) {
      lines.append(dynamic_cast<ChatLine*>(msg));
    }
    chatWidget->setContents(lines);
    connect(buf, SIGNAL(msgAppended(AbstractUiMsg *)), chatWidget, SLOT(appendMsg(AbstractUiMsg *)));
    connect(buf, SIGNAL(msgPrepended(AbstractUiMsg *)), chatWidget, SLOT(prependMsg(AbstractUiMsg *)));
    _chatWidgets[bufferId] = chatWidget;
    ui.stackedWidget->addWidget(chatWidget);
  }
  ui.stackedWidget->setCurrentWidget(chatWidget);
  disconnect(this, SIGNAL(userInput(QString)), 0, 0);
  connect(this, SIGNAL(userInput(QString)), Client::buffer(bufferId), SLOT(processUserInput(QString)));
  chatWidget->setFocusProxy(ui.inputEdit);
  ui.inputEdit->setFocus();

}

void BufferWidget::removeBuffer(BufferId bufferId) {
  if(!_chatWidgets.contains(bufferId))
    return;

  ChatWidget *chatWidget = _chatWidgets.take(bufferId);
  ui.stackedWidget->removeWidget(chatWidget);
  chatWidget->deleteLater();
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



/*
void BufferWidget::displayMsg(Message msg) {
  chatWidget->appendMsg(msg);
}
*/


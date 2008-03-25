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
#include "chatline.h"
#include "chatline-old.h"
#include "chatview.h"
#include "chatwidget.h"
#include "settings.h"
#include "client.h"

#include "global.h"

BufferWidget::BufferWidget(QWidget *parent) : AbstractBufferContainer(parent) {
  ui.setupUi(this);
}

BufferWidget::~BufferWidget() {

}

AbstractChatView *BufferWidget::createChatView(BufferId id) {
  AbstractChatView *chatView;
  if(Global::SPUTDEV) {
    chatView = new ChatView(Client::buffer(id), this);
  } else {
    chatView = new ChatWidget(id, this);
  }
  ui.stackedWidget->addWidget(dynamic_cast<QWidget *>(chatView));
  dynamic_cast<QWidget *>(chatView)->setFocusProxy(this);
  return chatView;
}

void BufferWidget::removeChatView(AbstractChatView *view) {
  ui.stackedWidget->removeWidget(dynamic_cast<QWidget *>(view));
  dynamic_cast<QWidget *>(view)->deleteLater();
}

void BufferWidget::showChatView(AbstractChatView *view) {
  if(!view) ui.stackedWidget->setCurrentWidget(ui.page);
  else ui.stackedWidget->setCurrentWidget(dynamic_cast<QWidget *>(view));
}


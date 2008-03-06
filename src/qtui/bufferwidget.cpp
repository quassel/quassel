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
#include "identity.h"
#include "network.h"
#include "networkmodel.h"

BufferWidget::BufferWidget(QWidget *parent)
  : QWidget(parent),
    _bufferModel(0),
    _selectionModel(0),
    _currentBuffer(0)
{
  ui.setupUi(this);
}

BufferWidget::~BufferWidget() {
}

void BufferWidget::init() {
}

void BufferWidget::setModel(BufferModel *bufferModel) {
  if(_bufferModel) {
    disconnect(_bufferModel, 0, this, 0);
  }
  _bufferModel = bufferModel;

  if(bufferModel) {
    connect(bufferModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),
	    this, SLOT(rowsAboutToBeRemoved(QModelIndex, int, int)));
  }
}

void BufferWidget::setSelectionModel(QItemSelectionModel *selectionModel) {
  if(_selectionModel) {
    disconnect(_selectionModel, 0, this, 0);
  }
  _selectionModel = selectionModel;

  if(selectionModel) {
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	    this, SLOT(currentChanged(QModelIndex, QModelIndex)));
  }
}

void BufferWidget::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
  Q_ASSERT(model());
  if(!parent.isValid()) {
    // ok this means that whole networks are about to be removed
    // we can't determine which buffers are affect, so we hope that all nets are removed
    // this is the most common case (for example disconnecting from the core or terminating the clint)
    if(model()->rowCount(parent) != end - start + 1)
      return;

    ChatWidget *chatWidget;
    QHash<BufferId, ChatWidget *>::iterator iter = _chatWidgets.begin();
    while(iter != _chatWidgets.end()) {
      chatWidget = *iter;
      iter = _chatWidgets.erase(iter);
      ui.stackedWidget->removeWidget(chatWidget);
      chatWidget->deleteLater();
    }
    
  } else {
    // check if there are explicitly buffers removed
    for(int i = start; i <= end; i++) {
      QVariant variant = parent.child(i,0).data(NetworkModel::BufferIdRole);
      if(!variant.isValid())
	continue;
      
      BufferId bufferId = qVariantValue<BufferId>(variant);
      removeBuffer(bufferId);
    }
  }
}

void BufferWidget::removeBuffer(BufferId bufferId) {
  if(!_chatWidgets.contains(bufferId))
    return;

  if(Client::buffer(bufferId)) Client::buffer(bufferId)->setVisible(false);
  ChatWidget *chatWidget = _chatWidgets.take(bufferId);
  ui.stackedWidget->removeWidget(chatWidget);
  chatWidget->deleteLater();
}

void BufferWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  BufferId newBufferId = current.data(NetworkModel::BufferIdRole).value<BufferId>();
  BufferId oldBufferId = previous.data(NetworkModel::BufferIdRole).value<BufferId>();
  if(newBufferId != oldBufferId)
    setCurrentBuffer(newBufferId);
}

void BufferWidget::setCurrentBuffer(BufferId bufferId) {
  if(!bufferId.isValid()) {
    ui.stackedWidget->setCurrentWidget(ui.page);
    return;
  }
  
  ChatWidget *chatWidget = 0;
  Buffer *buf = Client::buffer(bufferId);
  if(!buf) {
    qWarning() << "BufferWidget::setBuffer(BufferId): Can't show unknown Buffer:" << bufferId;
    return;
  }
  Buffer *prevBuffer = Client::buffer(currentBuffer());
  if(prevBuffer) prevBuffer->setVisible(false);
  if(_chatWidgets.contains(bufferId)) {
     chatWidget = _chatWidgets[bufferId];
  } else {
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
    chatWidget->setFocusProxy(this);
  }
  _currentBuffer = bufferId;
  ui.stackedWidget->setCurrentWidget(chatWidget);
  buf->setVisible(true);
  setFocus();
}


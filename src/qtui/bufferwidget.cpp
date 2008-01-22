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
    _selectionModel(0)
{
  ui.setupUi(this);
  ui.ownNick->clear();  // TODO add nick history
  connect(ui.inputEdit, SIGNAL(returnPressed()), this, SLOT(enterPressed()));
  connect(ui.ownNick, SIGNAL(activated(QString)), this, SLOT(changeNick(QString)));
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
  connect(bufferModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),
	  this, SLOT(rowsAboutToBeRemoved(QModelIndex, int, int)));
}

void BufferWidget::setSelectionModel(QItemSelectionModel *selectionModel) {
  if(_selectionModel) {
    disconnect(_selectionModel, 0, this, 0);
  }
  _selectionModel = selectionModel;
  connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(currentChanged(QModelIndex, QModelIndex)));
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

  ChatWidget *chatWidget = _chatWidgets.take(bufferId);
  ui.stackedWidget->removeWidget(chatWidget);
  chatWidget->deleteLater();
}

void BufferWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous);
  QVariant variant;

  variant = current.data(NetworkModel::BufferIdRole);
  if(!variant.isValid())
    return;
  
  setCurrentBuffer(qVariantValue<BufferId>(variant));
  updateNickSelector();
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

Network *BufferWidget::currentNetwork() const {
  if(!selectionModel())
    return 0;

  QVariant variant = selectionModel()->currentIndex().data(NetworkModel::NetworkIdRole);
  if(!variant.isValid())
    return 0;

  return Client::network(qVariantValue<NetworkId>(variant));
}

void BufferWidget::updateNickSelector() const {
  Network *net = currentNetwork();
  if(!net)
    return;

  const Identity *identity = Client::identity(net->identity());
  if(!identity) {
    qWarning() << "BufferWidget::setCurrentNetwork(): can't find Identity for Network" << net->networkId();
    return;
  }

  int nickIdx;
  QStringList nicks = identity->nicks();
  if((nickIdx = nicks.indexOf(net->myNick())) == -1) {
    nicks.prepend(net->myNick());
    nickIdx = 0;
  }
  
  ui.ownNick->clear();
  ui.ownNick->addItems(nicks);
  ui.ownNick->setCurrentIndex(nickIdx);
}

void BufferWidget::changeNick(const QString &newNick) const {
  Network *net = currentNetwork();
  if(!net || net->isMyNick(newNick))
    return;
  emit userInput(QString("/nick %1").arg(newNick));
}

void BufferWidget::enterPressed() {
  QStringList lines = ui.inputEdit->text().split('\n', QString::SkipEmptyParts);
  foreach(QString msg, lines) {
    if(msg.isEmpty()) continue;
    emit userInput(msg);
  }
  ui.inputEdit->clear();
}


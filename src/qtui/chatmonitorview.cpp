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

#include "chatmonitorview.h"

#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>

#include "chatmonitorfilter.h"
#include "chatlinemodel.h"
#include "chatitem.h"
#include "chatscene.h"
#include "client.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "messagemodel.h"
#include "qtuisettings.h"

ChatMonitorView::ChatMonitorView(ChatMonitorFilter *filter, QWidget *parent)
  : ChatView(filter, parent),
    _filter(filter)
{
}

void ChatMonitorView::contextMenuEvent(QContextMenuEvent *event) {
  if(scene()->sectionByScenePos(event->pos()) != ChatLineModel::SenderColumn)
    return;

  int showFields = _filter->showFields();

  QMenu contextMenu(this);
  QAction *showNetworkAction = contextMenu.addAction(tr("Show network name"), this, SLOT(showFieldsChanged(bool)));
  showNetworkAction->setCheckable(true);
  showNetworkAction->setChecked(showFields & ChatMonitorFilter::NetworkField);
  showNetworkAction->setData(ChatMonitorFilter::NetworkField);

  QAction *showBufferAction = contextMenu.addAction(tr("Show buffer name"), this, SLOT(showFieldsChanged(bool)));
  showBufferAction->setCheckable(true);
  showBufferAction->setChecked(showFields & ChatMonitorFilter::BufferField);
  showBufferAction->setData(ChatMonitorFilter::BufferField);

  contextMenu.exec(QCursor::pos());
}

void ChatMonitorView::mouseDoubleClickEvent(QMouseEvent *event) {
  if(scene()->sectionByScenePos(event->pos()) != ChatLineModel::SenderColumn) {
    ChatView::mouseDoubleClickEvent(event);
    return;
  }

  ChatItem *chatItem = dynamic_cast<ChatItem *>(itemAt(event->pos()));
  if(!chatItem) {
    event->ignore();
    return;
  }

  event->accept();
  BufferId bufferId = chatItem->data(MessageModel::BufferIdRole).value<BufferId>();
  if(!bufferId.isValid())
    return;

  QModelIndex bufferIdx = Client::networkModel()->bufferIndex(bufferId);
  if(!bufferIdx.isValid())
    return;

  Client::bufferModel()->setCurrentIndex(Client::bufferModel()->mapFromSource(bufferIdx));
}

void ChatMonitorView::showFieldsChanged(bool checked) {
  QAction *showAction = qobject_cast<QAction *>(sender());
  if(!showAction)
    return;

  if(checked)
    _filter->addShowField(showAction->data().toInt());
  else
    _filter->removeShowField(showAction->data().toInt());
}

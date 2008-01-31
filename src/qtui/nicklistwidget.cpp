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

#include "nicklistwidget.h"

#include "buffer.h"
#include "nickview.h"
#include "client.h"
#include "networkmodel.h"
#include "nickviewfilter.h"

NickListWidget::NickListWidget(QWidget *parent)
  : QWidget(parent),
    _currentBuffer(0)
{
  ui.setupUi(this);
}


BufferId NickListWidget::currentBuffer() const {
  return _currentBuffer;
}

void NickListWidget::setCurrentBuffer(BufferId bufferId) {
  QModelIndex bufferIdx = Client::networkModel()->bufferIndex(bufferId);
  
  if(bufferIdx.data(NetworkModel::BufferTypeRole) != BufferItem::ChannelType) {
    ui.stackedWidget->setCurrentWidget(ui.emptyPage);
    return;
  }

  if(nickViews.contains(bufferId)) {
    ui.stackedWidget->setCurrentWidget(nickViews.value(bufferId));
  } else {
    NickView *view = new NickView(this);
    NickViewFilter *filter = new NickViewFilter(Client::networkModel());
    view->setModel(filter);
    view->setRootIndex(filter->mapFromSource(bufferIdx));
    nickViews[bufferId] = view;
    ui.stackedWidget->addWidget(view);
    ui.stackedWidget->setCurrentWidget(view);
  }
}

void NickListWidget::reset() {
  foreach(NickView *view, nickViews.values()) {
    ui.stackedWidget->removeWidget(view);
    view->deleteLater();
  }
  nickViews.clear();
}

void NickListWidget::removeBuffer(BufferId bufferId) {
  if(!nickViews.contains(bufferId))
    return;
  
  NickView *view = nickViews.take(bufferId);
  ui.stackedWidget->removeWidget(view);
  view->deleteLater();
}

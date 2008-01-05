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

NickListWidget::NickListWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);

}

void NickListWidget::setBuffer(Buffer *buf) {
  if(!buf) {
    ui.stackedWidget->setCurrentWidget(ui.emptyPage);
    return;
  }
  if(buf->bufferType() != Buffer::ChannelType) {
    ui.stackedWidget->setCurrentWidget(ui.emptyPage);
  } else {
    if(nickViews.contains(buf)) {
      ui.stackedWidget->setCurrentWidget(nickViews.value(buf));
    } else {
      NickView *view = new NickView(this);
      view->setModel(buf->nickModel());
      nickViews[buf] = view;
      ui.stackedWidget->addWidget(view);
      ui.stackedWidget->setCurrentWidget(view);
      connect(buf, SIGNAL(destroyed(QObject *)), this, SLOT(bufferDestroyed(QObject *)));
    }
  }
}

void NickListWidget::reset() {
  foreach(NickView *view, nickViews.values()) {
    ui.stackedWidget->removeWidget(view);
    view->deleteLater();
  }
  nickViews.clear();
}

void NickListWidget::bufferDestroyed(QObject *buf) {
  if(nickViews.contains((Buffer *)buf)) {
    NickView *view = nickViews.take((Buffer *)buf);
    ui.stackedWidget->removeWidget(view);
    view->deleteLater();
  }
}

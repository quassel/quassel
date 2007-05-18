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

#include "global.h"
#include "networkview.h"

NetworkViewWidget::NetworkViewWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);

  //setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
}


QSize NetworkViewWidget::sizeHint() const {
  return QSize(150,100);

}

/**************************************************************************/

NetworkView::NetworkView(QString n, int m, QStringList nets, QWidget *parent) : QDockWidget(parent) {
  setObjectName(QString("View-"+n)); // should be unique for mainwindow state!
  name = n; mode = m;
  setWindowTitle(name);
  networks = nets;
  currentBuffer = 0;
  setWidget(new NetworkViewWidget(this));
  tree = qobject_cast<NetworkViewWidget*>(widget())->tree();
  tree->header()->hide();
  tree->setSortingEnabled(true);
  tree->setRootIsDecorated(true);
  tree->setIndentation(10);
  //tree->setAnimated(true);
  connect(tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(itemClicked(QTreeWidgetItem*)));
  connect(tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*)));
  connect(this, SIGNAL(fakeUserInput(BufferId, QString)), guiProxy, SLOT(gsUserInput(BufferId, QString)));
  
}

void NetworkView::setBuffers(QList<Buffer *> buffers) {
  tree->clear(); bufitems.clear(); netitems.clear();
  foreach(Buffer *b, buffers) {
    bufferUpdated(b);
  }
}

void NetworkView::bufferUpdated(Buffer *b) {
  if(bufitems.contains(b)) {
    // FIXME this looks ugly
    /*
      this is actually fugly! :) - EgS
      if anyone else wonders what this does: it takes the TreeItem related to the buffer out of the parents child list
      therefore we need to know the childs index
    */
    QTreeWidgetItem *item = bufitems[b]->parent()->takeChild(bufitems[b]->parent()->indexOfChild(bufitems[b]));
    delete item;
    bufitems.remove(b);
  }
  if(shouldShow(b)) {
    QString net = b->networkName();
    QString buf = b->bufferName();
    QTreeWidgetItem *item;
    QStringList label;
    if(b->bufferType() == Buffer::ServerBuffer) label << tr("Status");
    else label << buf;
    if((mode & SomeNets) || ( mode & AllNets)) {
      if(!netitems.contains(net)) {
        netitems[net] = new QTreeWidgetItem(tree, QStringList(net));
        netitems[net]->setFlags(Qt::ItemIsEnabled);
      }
      QTreeWidgetItem *ni = netitems[net];
      ni->setExpanded(true);
      ni->setFlags(Qt::ItemIsEnabled);
      item = new QTreeWidgetItem(ni, label);
    } else {
      item = new QTreeWidgetItem(label);
    }
    //item->setFlags(Qt::ItemIsEnabled);
    bufitems[b] = item;
    // TODO Use style engine!
    if(!b->isActive()) {
      item->setForeground(0, QColor("grey"));
    }
    if(b == currentBuffer) {
      // this fixes the multiple simultaneous selections
      emit bufferSelected(b);
      //item->setSelected(true);
    }
  }
  foreach(QString key, netitems.keys()) {
    if(!netitems[key]->childCount()) {
      delete netitems[key];
      QTreeWidgetItem *item = netitems[key]->parent()->takeChild(netitems[key]->parent()->indexOfChild(netitems[key]));
      delete item;
      netitems.remove(key);
    }
  }
}

bool NetworkView::shouldShow(Buffer *b) {
  // bool f = false;
  if((mode & NoActive) && b->isActive()) return false;
  if((mode & NoInactive) && !b->isActive()) return false;
  if((mode & NoChannels) && b->bufferType() == Buffer::ChannelBuffer) return false;
  if((mode & NoQueries) && b->bufferType() == Buffer::QueryBuffer) return false;
  if((mode & NoServers) && b->bufferType() == Buffer::ServerBuffer) return false;
  if((mode & SomeNets) && !networks.contains(b->networkName())) return false;
  return true;
}

void NetworkView::bufferDestroyed(Buffer *b) {

}

void NetworkView::itemClicked(QTreeWidgetItem *item) {
  Buffer *b = bufitems.key(item);
  if(b) {
    // there is a buffer associated with the item (aka: status/channel/query)
    emit bufferSelected(b);
  } else {
    // network item
    item->setExpanded(!item->isExpanded());
  }
}

void NetworkView::itemDoubleClicked(QTreeWidgetItem *item) {
  Buffer *b = bufitems.key(item);
  if(b && Buffer::ChannelBuffer == b->bufferType()) {
    emit fakeUserInput(b->bufferId(), QString("/join " + b->bufferName()));
    emit bufferSelected(b);
  }
}

void NetworkView::selectBuffer(Buffer *b) {
  QTreeWidgetItem *item = 0;
  if(bufitems.contains(b)) item = bufitems[b];
  QList<QTreeWidgetItem *> sel = tree->selectedItems();
  foreach(QTreeWidgetItem *i, sel) { if(i != item) i->setSelected(false); }
  if(item) {
    item->setSelected(true);
    currentBuffer = b;
  } else {
    currentBuffer = 0;
  }
}

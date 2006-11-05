/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

}

QSize NetworkViewWidget::sizeHint() const {
  return QSize(150,100);

}

/**************************************************************************/

NetworkView::NetworkView(QString net, QWidget *parent) : network(net), QDockWidget(parent) {
  if(!net.isEmpty()) setWindowTitle(net);
  else setWindowTitle(tr("All Buffers"));
  setWidget(new NetworkViewWidget(this));
  tree = qobject_cast<NetworkViewWidget*>(widget())->tree();
  tree->setSortingEnabled(false);
  connect(tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(itemClicked(QTreeWidgetItem*)));
}

void NetworkView::buffersUpdated(BufferHash buffers) {
  tree->clear(); items.clear();
  if(network.isEmpty()) {
    tree->setHeaderLabel(tr("Networks"));
    foreach(QString net, buffers.keys()) {
      QTreeWidgetItem *netItem = new QTreeWidgetItem(QStringList(net));
      foreach(QString buf, buffers[net].keys()) {
        Buffer *b = buffers[net][buf];
        QStringList label;
        if(buf.isEmpty()) label << tr("Status");
        else label << buf;
        QTreeWidgetItem *item = new QTreeWidgetItem(netItem, label);
        items[net][buf] = item;
        VarMap d;
        d["Network"] = net;
        d["Buffer"] = buf;
        item->setData(0, Qt::UserRole, d);
        if(!b->isActive()) {
          item->setForeground(0, QColor("grey"));
        }
      }
      VarMap d;
      d["Network"] = net; d["Buffer"] = "";
      netItem->setData(0, Qt::UserRole, d);
      netItem->setFlags(netItem->flags() & ~Qt::ItemIsSelectable);
      tree->addTopLevelItem(netItem);
      netItem->setExpanded(true);
    }
    if(items[currentNetwork][currentBuffer]) items[currentNetwork][currentBuffer]->setSelected(true);
  }
}

void NetworkView::itemClicked(QTreeWidgetItem *item) {
  if(network.isEmpty()) {
    VarMap d = item->data(0, Qt::UserRole).toMap();
    QString net = d["Network"].toString(); QString buf = d["Buffer"].toString();
    if(!net.isEmpty()) {
      emit bufferSelected(net, buf);
      currentNetwork = net; currentBuffer = buf;
    }
  }
}

void NetworkView::selectBuffer(QString net, QString buf) {
  if(items[net][buf]) items[net][buf]->setSelected(true);
  currentNetwork = net; currentBuffer = buf;
}

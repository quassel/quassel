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

#include "client.h"
//#include "buffer.h"
#include "bufferview.h"
#include "networkmodel.h"

/*****************************************
* The TreeView showing the Buffers
*****************************************/
// Please be carefull when reimplementing methods which are used to inform the view about changes to the data
// to be on the safe side: call QTreeView's method aswell
BufferView::BufferView(QWidget *parent) : QTreeView(parent) {
}

void BufferView::init() {
  setIndentation(10);
  header()->setContextMenuPolicy(Qt::ActionsContextMenu);
  expandAll();

  setAnimated(true);

#ifndef QT_NO_DRAGANDDROP
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
#endif

  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(joinChannel(QModelIndex)));
}

void BufferView::setFilteredModel(QAbstractItemModel *model, BufferViewFilter::Modes mode, QList<NetworkId> nets) {
  BufferViewFilter *filter = new BufferViewFilter(model, mode, nets);
  setModel(filter);
  connect(this, SIGNAL(removeBuffer(const QModelIndex &)), filter, SLOT(removeBuffer(const QModelIndex &)));
}

void BufferView::setModel(QAbstractItemModel *model) {
  delete selectionModel();
  QTreeView::setModel(model);
  init();

  // remove old Actions
  QList<QAction *> oldactions = header()->actions();
  foreach(QAction *action, oldactions) {
    header()->removeAction(action);
    action->deleteLater();
  }

  QString sectionName;
  QAction *showSection;
  for(int i = 0; i < model->columnCount(); i++) {
    sectionName = (model->headerData(i, Qt::Horizontal, Qt::DisplayRole)).toString();
    showSection = new QAction(sectionName, header());
    showSection->setCheckable(true);
    showSection->setChecked(true);
    showSection->setProperty("column", i);
    connect(showSection, SIGNAL(toggled(bool)), this, SLOT(toggleHeader(bool)));
    header()->addAction(showSection);
  }
  
}

void BufferView::joinChannel(const QModelIndex &index) {
  BufferItem::Type bufferType = (BufferItem::Type)index.data(NetworkModel::BufferTypeRole).toInt();

  if(bufferType != BufferItem::ChannelType)
    return;
  
  Client::fakeInput(index.data(NetworkModel::BufferIdRole).value<BufferId>(), QString("/JOIN %1").arg(index.sibling(index.row(), 0).data().toString()));
}

void BufferView::keyPressEvent(QKeyEvent *event) {
  if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
    event->accept();
    QModelIndex index = selectionModel()->selectedIndexes().first();
    if(index.isValid()) {
      emit removeBuffer(index);
    }
  }
  QTreeView::keyPressEvent(event);
}

// ensure that newly inserted network nodes are expanded per default
void BufferView::rowsInserted(const QModelIndex & parent, int start, int end) {
  QTreeView::rowsInserted(parent, start, end);
  if(model()->rowCount(parent) == 1 && parent != QModelIndex()) {
    // without updating the parent the expand will have no effect... Qt Bug?
    update(parent); 
    expand(parent);
  }
}

void BufferView::toggleHeader(bool checked) {
  QAction *action = qobject_cast<QAction *>(sender());
  header()->setSectionHidden((action->property("column")).toInt(), !checked);
}

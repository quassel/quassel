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

#include <QHeaderView>
#include <QDebug>
#include <QMenu>

#include "nickview.h"
#include "nickmodel.h"
#include "networkmodel.h"
#include "types.h"
#include "client.h"


NickView::NickView(QWidget *parent)
  : QTreeView(parent)
{
  setIndentation(10);
  setAnimated(true);
  header()->hide();
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), 
          this, SLOT(showContextMenu(const QPoint&)));
}

NickView::~NickView() {
}

void NickView::init() {
  if(!model())
    return;

  for(int i = 1; i < model()->columnCount(); i++)
    setColumnHidden(i, true);

  expandAll();
}

void NickView::setModel(QAbstractItemModel *model) {
  QTreeView::setModel(model);
  init();
}

void NickView::rowsInserted(const QModelIndex &index, int start, int end) {
  QTreeView::rowsInserted(index, start, end);
  expandAll();  // FIXME We need to do this more intelligently. Maybe a pimped TreeView?
}

void NickView::showContextMenu(const QPoint & pos ) {
  QModelIndex index = indexAt(pos);
  QString username = index.sibling(index.row(), 0).data().toString();
  BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();

  QMenu nickContextMenu(this);
  nickContextMenu.addAction(tr("context menu for %1").arg(username));
  nickContextMenu.addSeparator();

  QAction *whoisAction = nickContextMenu.addAction(tr("WHOIS"));
  QAction *result = nickContextMenu.exec(QCursor::pos());

  if (result == whoisAction ) {
    Client::userInput(bufferInfo, "/WHOIS "+username);
  }
}

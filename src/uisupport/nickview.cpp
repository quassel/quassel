/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include <QScrollBar>
#include <QDebug>
#include <QMenu>

#include "buffermodel.h"
#include "client.h"
#include "nickview.h"
#include "nickviewfilter.h"
#include "networkmodel.h"
#include "quasselui.h"
#include "types.h"

class ExpandAllEvent : public QEvent {
public:
  ExpandAllEvent() : QEvent(QEvent::User) {}
};

NickView::NickView(QWidget *parent)
  : QTreeView(parent)
{
  setIndentation(10);
  setAnimated(true);
  header()->hide();
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

  setContextMenuPolicy(Qt::CustomContextMenu);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(showContextMenu(const QPoint&)));

#if defined Q_WS_QWS || defined Q_WS_X11
  connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(startQuery(QModelIndex)));
#else
  // afaik this is better on Mac and Windows
  connect(this, SIGNAL(activated(QModelIndex)), SLOT(startQuery(QModelIndex)));
#endif
}

void NickView::init() {
  if(!model())
    return;

  for(int i = 1; i < model()->columnCount(); i++)
    setColumnHidden(i, true);
}

void NickView::setModel(QAbstractItemModel *model) {
  QTreeView::setModel(model);
  init();
}

void NickView::rowsInserted(const QModelIndex &parent, int start, int end) {
  QTreeView::rowsInserted(parent, start, end);
  if(model()->data(parent, NetworkModel::ItemTypeRole) == NetworkModel::UserCategoryItemType && !isExpanded(parent)) {
    QCoreApplication::postEvent(this, new ExpandAllEvent);
  }
}

void NickView::setRootIndex(const QModelIndex &index) {
  QAbstractItemView::setRootIndex(index);
  if(index.isValid())
    QCoreApplication::postEvent(this, new ExpandAllEvent);
}

void NickView::showContextMenu(const QPoint & pos ) {
  QModelIndex index = indexAt(pos);
  if(index.data(NetworkModel::ItemTypeRole) != NetworkModel::IrcUserItemType)
    return;

  QModelIndexList indexList = selectedIndexes();
  // make sure the item we clicked on is first
  indexList.removeAll(index);
  indexList.prepend(index);

  QMenu contextMenu(this);
  Client::mainUi()->actionProvider()->addActions(&contextMenu, indexList);
  contextMenu.exec(QCursor::pos());
}

void NickView::startQuery(const QModelIndex &index) {
  if(index.data(NetworkModel::ItemTypeRole) != NetworkModel::IrcUserItemType)
    return;

  IrcUser *ircUser = qobject_cast<IrcUser *>(index.data(NetworkModel::IrcUserRole).value<QObject *>());
  NetworkId networkId = index.data(NetworkModel::NetworkIdRole).value<NetworkId>();
  if(!ircUser || !networkId.isValid())
    return;

  BufferId bufId = Client::networkModel()->bufferId(networkId, ircUser->nick());
  if(bufId.isValid())
    Client::bufferModel()->switchToBuffer(bufId);
  else
    Client::userInput(index.data(NetworkModel::BufferInfoRole).value<BufferInfo>(), QString("/QUERY %1").arg(ircUser->nick()));
}

void NickView::customEvent(QEvent *event) {
  // THIS IS A REPLACEMENT FOR expandAll()
  /* WARNING: do not call expandAll()!
   * it fucks up big time in combination with sorting and changing the rootIndex
   * the following sequence of commands leads to unexpected behavior when inserting new items
   * setSortingEnabled(true);
   * setModel();
   * expandAll();
   * setRootIndex();
   */
  if(event->type() != QEvent::User)
    return;

  QModelIndex topLevelIdx;
  for(int i = 0; i < model()->rowCount(rootIndex()); i++) {
    topLevelIdx = model()->index(i, 0, rootIndex());
    if(isExpanded(topLevelIdx))
      continue;
    else {
      expand(topLevelIdx);
      if(i < model()->rowCount(rootIndex()) - 1)
        QCoreApplication::postEvent(this, new ExpandAllEvent);
      break;
    }
  }
  event->accept();
}

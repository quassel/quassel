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
#include <QScrollBar>
#include <QDebug>
#include <QMenu>

#include "nickview.h"
#include "nickviewfilter.h"
#include "networkmodel.h"
#include "buffermodel.h"
#include "types.h"
#include "client.h"

struct LazySizeHint {
  LazySizeHint() : size(QSize()), needsUpdate(true) {};
  QSize size;
  bool needsUpdate;
};

NickView::NickView(QWidget *parent)
  : QTreeView(parent),
    _sizeHint(new LazySizeHint())
{
  setIndentation(10);
  setAnimated(true);
  header()->hide();
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(showContextMenu(const QPoint&)));
  connect(this, SIGNAL(activated( const QModelIndex& )),
          this, SLOT(startQuery( const QModelIndex& )));
}

NickView::~NickView() {
  delete _sizeHint;
  _sizeHint = 0;
}

void NickView::init() {
  if(!model())
    return;

  for(int i = 1; i < model()->columnCount(); i++)
    setColumnHidden(i, true);

  expandAll();
  _sizeHint->needsUpdate = true;
}

void NickView::setModel(QAbstractItemModel *model) {
  QTreeView::setModel(model);
  init();
}

void NickView::rowsInserted(const QModelIndex &parent, int start, int end) {
  QTreeView::rowsInserted(parent, start, end);
  if(model()->data(parent, NetworkModel::ItemTypeRole) == NetworkModel::UserCategoryItemType && !isExpanded(parent))
    expand(parent);
  _sizeHint->needsUpdate = true;
}

void NickView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
  QTreeView::rowsAboutToBeRemoved(parent, start, end);
  _sizeHint->needsUpdate = true;
}

void NickView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QTreeView::dataChanged(topLeft, bottomRight);
  _sizeHint->needsUpdate = true;
}

QString NickView::nickFromModelIndex(const QModelIndex & index) {
  QString nick = index.sibling(index.row(), 0).data().toString();
  return nick;
}

BufferInfo NickView::bufferInfoFromModelIndex(const QModelIndex & index) {
  BufferInfo bufferInfo = index.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
  return bufferInfo;
}

void NickView::showContextMenu(const QPoint & pos ) {
  QModelIndex index = indexAt(pos);
  if(index.data(NetworkModel::ItemTypeRole) != NetworkModel::IrcUserItemType) return;

  QString nick = nickFromModelIndex(index);

  QMenu nickContextMenu(this);

  QAction *whoisAction = nickContextMenu.addAction(tr("WHOIS"));
  QAction *versionAction = nickContextMenu.addAction(tr("VERSION"));
  QAction *pingAction = nickContextMenu.addAction(tr("PING"));

  nickContextMenu.addSeparator();

  QMenu *modeMenu = nickContextMenu.addMenu(tr("Modes"));
  QAction *opAction = modeMenu->addAction(tr("Op %1").arg(nick));
  QAction *deOpAction = modeMenu->addAction(tr("Deop %1").arg(nick));
  QAction *voiceAction = modeMenu->addAction(tr("Voice %1").arg(nick));
  QAction *deVoiceAction = modeMenu->addAction(tr("Devoice %1").arg(nick));

  QMenu *kickBanMenu = nickContextMenu.addMenu(tr("Kick/Ban"));
  //TODO: add kick message from network identity (kick reason)
  QAction *kickAction = kickBanMenu->addAction(tr("Kick %1").arg(nick));
  QAction *kickBanAction = kickBanMenu->addAction(tr("Kickban %1").arg(nick));
  kickBanMenu->setEnabled(false);
  QAction *ignoreAction = nickContextMenu.addAction(tr("Ignore"));
  ignoreAction->setEnabled(false);

  nickContextMenu.addSeparator();

  QAction *queryAction = nickContextMenu.addAction(tr("Query"));
  QAction *dccChatAction = nickContextMenu.addAction(tr("DCC-Chat"));
  dccChatAction->setEnabled(false);
  QAction *sendFileAction = nickContextMenu.addAction(tr("Send file"));
  sendFileAction->setEnabled(false);

  QAction *action = nickContextMenu.exec(QCursor::pos());
  BufferInfo bufferInfo = bufferInfoFromModelIndex(index);

  if(action == whoisAction)         { executeCommand(bufferInfo, QString("/WHOIS %1 %1").arg(nick)); }
  else if(action == versionAction)  { executeCommand(bufferInfo, QString("/CTCP %1 VERSION").arg(nick)); }
  else if(action == pingAction)     { executeCommand(bufferInfo, QString("/CTCP %1 PING ").arg(nick)); }

  else if(action == opAction)       { executeCommand(bufferInfo, QString("/OP %1").arg(nick)); }
  else if(action == deOpAction)     { executeCommand(bufferInfo, QString("/DEOP %1").arg(nick)); }
  else if(action == voiceAction)    { executeCommand(bufferInfo, QString("/VOICE %1").arg(nick)); }
  else if(action == deVoiceAction)  { executeCommand(bufferInfo, QString("/DEVOICE %1").arg(nick)); }

  else if(action == kickAction)     { executeCommand(bufferInfo, QString("/KICK %1").arg(nick)); }
  else if(action == kickBanAction)  { executeCommand(bufferInfo, QString("/KICKBAN %1").arg(nick)); }
  else if(action == queryAction)    { executeCommand(bufferInfo, QString("/QUERY %1").arg(nick)); }

}

void NickView::startQuery(const QModelIndex & index) {
  QString nick = nickFromModelIndex(index);
  bool activated = false;

  if(QSortFilterProxyModel *nickviewFilter = qobject_cast<QSortFilterProxyModel *>(model())) {
    // rootIndex() is the channel, parent() is the corresponding network
    QModelIndex networkIndex = rootIndex().parent(); 
    QModelIndex source_networkIndex = nickviewFilter->mapToSource(networkIndex);
    for(int i = 0; i < Client::networkModel()->rowCount(source_networkIndex); i++) {
      QModelIndex childIndex = source_networkIndex.child( i, 0);
      if(nick.toLower() == childIndex.data().toString().toLower()) {
        QModelIndex queryIndex = Client::bufferModel()->mapFromSource(childIndex);
        Client::bufferModel()->setCurrentIndex(queryIndex);
        activated = true;
      }
    }
  }
  if(!activated) {
    BufferInfo bufferInfo = bufferInfoFromModelIndex(index);
    executeCommand(bufferInfo, QString("/QUERY %1").arg(nick));
  }
}

void NickView::executeCommand(const BufferInfo & bufferInfo, const QString & command) {
  Client::instance()->userInput(bufferInfo, command);
}

QSize NickView::sizeHint() const {
  Q_CHECK_PTR(_sizeHint);
  if(_sizeHint->needsUpdate && model()) {;
    int columnSize = 0;
    for(int i = 0; i < model()->columnCount(); i++) {
      if(!isColumnHidden(i))
	columnSize += sizeHintForColumn(i);
    }
    _sizeHint->size = QSize(columnSize, 50);
    _sizeHint->needsUpdate = false;
  }
  return _sizeHint->size;
}

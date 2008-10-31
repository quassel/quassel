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

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	  this, SLOT(showContextMenu(const QPoint&)));
  connect(this, SIGNAL(activated( const QModelIndex& )),
          this, SLOT(startQuery( const QModelIndex& )));
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
  QAction *kickAction = kickBanMenu->addAction(tr("Kick %1").arg(nick));
  QAction *banAction = kickBanMenu->addAction(tr("Ban %1").arg(nick));
  QAction *kickBanAction = kickBanMenu->addAction(tr("Kickban %1").arg(nick));
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
  else if(action == banAction)      { executeCommand(bufferInfo, QString("/BAN %1").arg(nick)); }
  else if(action == kickBanAction)  { executeCommand(bufferInfo, QString("/KICK %1").arg(nick)); 
                                      executeCommand(bufferInfo, QString("/BAN %1").arg(nick)); }
  else if(action == queryAction)    { startQuery(index); }

}

void NickView::startQuery(const QModelIndex &index) {
  if(index.data(NetworkModel::ItemTypeRole) != NetworkModel::IrcUserItemType)
    return;

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

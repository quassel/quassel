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
#include "nickviewfilter.h"
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

  QAction *whoisAction = nickContextMenu.addAction(tr("WHOIS"));
  QAction *versionAction = nickContextMenu.addAction(tr("VERSION"));
  QAction *pingAction = nickContextMenu.addAction(tr("PING"));

  nickContextMenu.addSeparator();

  QMenu *modeMenu = nickContextMenu.addMenu(tr("Modes"));
  QAction *opAction = modeMenu->addAction(tr("Op %1").arg(username));
  QAction *deOpAction = modeMenu->addAction(tr("Deop %1").arg(username));
  QAction *voiceAction = modeMenu->addAction(tr("Voice %1").arg(username));
  QAction *deVoiceAction = modeMenu->addAction(tr("Devoice %1").arg(username));

  QMenu *kickBanMenu = nickContextMenu.addMenu(tr("Kick/Ban"));
  QAction *kickAction = kickBanMenu->addAction(tr("Kick %1").arg(username));
  QAction *kickBanAction = kickBanMenu->addAction(tr("Kickban %1").arg(username));
  QAction *ignoreAction = nickContextMenu.addAction(tr("Ignore"));
  ignoreAction->setEnabled(false);

  nickContextMenu.addSeparator();

  QAction *queryAction = nickContextMenu.addAction(tr("Query"));
  queryAction->setEnabled(false);
  QAction *dccChatAction = nickContextMenu.addAction(tr("DCC-Chat"));
  dccChatAction->setEnabled(false);
  QAction *sendFileAction = nickContextMenu.addAction(tr("Send file"));
  sendFileAction->setEnabled(false);

  QAction *result = nickContextMenu.exec(QCursor::pos());

  if(result == whoisAction)         { Client::instance()->userInput(bufferInfo, QString("/WHOIS %1 %1").arg(username)); }
  else if(result == versionAction)  { Client::instance()->userInput(bufferInfo, QString("/CTCP %1 VERSION").arg(username)); }
  else if(result == pingAction)     { Client::instance()->userInput(bufferInfo, QString("/CTCP %1 PING ").arg(username)); }

  else if(result == opAction)       { Client::instance()->userInput(bufferInfo, "/OP " + username); }
  else if(result == deOpAction)     { Client::instance()->userInput(bufferInfo, "/DEOP " + username); }
  else if(result == voiceAction)    { Client::instance()->userInput(bufferInfo, "/VOICE " + username); }
  else if(result == deVoiceAction)  { Client::instance()->userInput(bufferInfo, "/DEVOICE " + username); }

  else if(result == kickAction)     { Client::instance()->userInput(bufferInfo, "/KICK " + username); }
  else if(result == kickBanAction)  { Client::instance()->userInput(bufferInfo, "/KICKBAN " + username); }
}

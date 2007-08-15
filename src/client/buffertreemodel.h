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

#ifndef _BUFFERTREEMODEL_H_
#define _BUFFERTREEMODEL_H_

#include <QtCore>

#include "treemodel.h"
#include "buffer.h"

/*****************************************
 *  Fancy Buffer Items
 *****************************************/
class BufferTreeItem : public TreeItem {
  Q_OBJECT
  
public:
  BufferTreeItem(Buffer *, TreeItem *parent = 0);

  virtual uint id() const;
  
  QVariant data(int column, int role) const;
  Buffer *buffer() const { return buf; }
  void setActivity(const Buffer::ActivityLevel &);
  
protected:
  QString text(int column) const;
  QColor foreground(int column) const;
  
  Buffer *buf;
  Buffer::ActivityLevel activity;
};

/*****************************************
 *  Network Items
 *****************************************/
class NetworkTreeItem : public TreeItem {
  Q_OBJECT
  
public:
  NetworkTreeItem(const QString &, TreeItem *parent = 0);

  virtual uint id() const;
  
private:
  QString net;
  
};

/*****************************************
 * BufferTreeModel
 *****************************************/
class BufferTreeModel : public TreeModel {
  Q_OBJECT
  
public:
  enum  myRoles {
    BufferTypeRole = Qt::UserRole,
    BufferActiveRole,
    BufferNameRole,
    BufferIdRole
  };
  
  BufferTreeModel(QObject *parent = 0);
  static QList<QVariant> defaultHeader();

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  
public slots:
  void bufferUpdated(Buffer *);    
  void changeCurrent(const QModelIndex &, const QModelIndex &);
  void selectBuffer(Buffer *buffer);
  void doubleClickReceived(const QModelIndex &);
  void bufferActivity(Buffer::ActivityLevel, Buffer *buffer);
  
signals:
  void bufferSelected(Buffer *);
  void invalidateFilter();
  void fakeUserInput(BufferId, QString);
  void selectionChanged(const QModelIndex &);
    
private:
  bool isBufferIndex(const QModelIndex &) const;
  Buffer *getBufferByIndex(const QModelIndex &) const;
  QModelIndex getOrCreateNetworkItemIndex(Buffer *buffer);
  QModelIndex getOrCreateBufferItemIndex(Buffer *buffer);

  QStringList mimeTypes() const;
  QMimeData *mimeData(const QModelIndexList &) const;
  bool dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &);
  
  Buffer *currentBuffer;
};

#endif

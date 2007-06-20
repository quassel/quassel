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

#ifndef _BUFFERVIEW_H_
#define _BUFFERVIEW_H_

#include <QtGui>
#include <QtCore>

#include "clientproxy.h"
#include "buffer.h"
#include "ui_bufferviewwidget.h"
#include "bufferview.h"

/*****************************************
 *  general item used in the Tree Model
 *****************************************/
class TreeItem {
public:
  TreeItem(QList<QVariant> &data, TreeItem *parent = 0);
  TreeItem(TreeItem *parent = 0);
  virtual ~TreeItem();
  
  void appendChild(TreeItem *child);
  void removeChild(int row);
                   
  TreeItem *child(int row);
  int childCount() const;
  int columnCount() const;
  virtual QVariant data(int column, int role) const;
  int row() const;
  TreeItem *parent();

  void setForeground(QColor);
    
private:
  QList<TreeItem*> childItems;
  TreeItem *parentItem;
  QList<QVariant> itemData;
  
  QColor foreground;
};


/*****************************************
 *  Fancy Buffer Items
 *****************************************/
class BufferTreeItem : public TreeItem{
public:
  BufferTreeItem(Buffer *, TreeItem *parent = 0);
  QVariant data(int column, int role) const;
  Buffer *buffer() const { return buf; }
  void setActivity(const Buffer::ActivityLevel &);
  
private:
    QString text(int column) const;
    QColor foreground(int column) const;
    Buffer *buf;
    Buffer::ActivityLevel activity;
};


/*****************************************
 * BufferTreeModel
 *****************************************/
class BufferTreeModel : public QAbstractItemModel {
  Q_OBJECT
  
public:
  enum  myRoles {
    BufferTypeRole = Qt::UserRole,
    BufferActiveRole
  };
  
  
  BufferTreeModel(QObject *parent = 0);
  ~BufferTreeModel();
  
  QVariant data(const QModelIndex &index, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  
  void clearActivity(Buffer *buffer);
  
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
  void updateSelection(const QModelIndex &, QItemSelectionModel::SelectionFlags);
    
private:
  bool isBufferIndex(const QModelIndex &) const;
  Buffer *getBufferByIndex(const QModelIndex &) const;
  QModelIndex getOrCreateNetworkItemIndex(Buffer *buffer);
  QModelIndex getOrCreateBufferItemIndex(Buffer *buffer);

  bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    
  QStringList mimeTypes() const;
  QMimeData *mimeData(const QModelIndexList &) const;
  bool dropMimeData(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &);
  TreeItem *rootItem;
  
  QHash<QString, TreeItem*> networkItem;
  QHash<Buffer *, BufferTreeItem*> bufferItem;
  Buffer *currentBuffer;
};




/*****************************************
 * This Widget Contains the BufferView
 *****************************************/
class BufferViewWidget : public QWidget {
  Q_OBJECT

public:
  BufferViewWidget(QWidget *parent = 0);
  virtual QSize sizeHint () const;
  BufferView *treeView() { return ui.treeView; }  

private:
  Ui::BufferViewWidget ui;
  
};


/*****************************************
 * Dock and API for the BufferViews
 *****************************************/
class BufferViewDock : public QDockWidget {
  Q_OBJECT

public:
  BufferViewDock(QAbstractItemModel *model, QString name, BufferViewFilter::Modes mode, QStringList nets = QStringList(), QWidget *parent = 0);
};


#endif

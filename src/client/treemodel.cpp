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
#include "treemodel.h"

/*****************************************
 *  Buffer Items stored in the Tree Model
 *****************************************/
TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent) : QObject(parent) {
  itemData = data;
  parentItem = parent;
}

TreeItem::TreeItem(TreeItem *parent) {
  itemData = QList<QVariant>();
  parentItem = parent;
}

TreeItem::~TreeItem() {
  qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *item) {
  childItems.append(item);
}

void TreeItem::removeChild(int row) {
  childItems.removeAt(row);
}

TreeItem *TreeItem::child(int row) {
  return childItems.value(row);
}

int TreeItem::childCount() const {
  return childItems.count();
}

int TreeItem::row() const {
  if(parentItem)
    return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
  else
    return 0;
}

TreeItem *TreeItem::parent() {
  return parentItem;
}

int TreeItem::columnCount() const {
  return itemData.count();
}

QVariant TreeItem::data(int column, int role) const {
  if(role == Qt::DisplayRole and column < itemData.count())
    return itemData[column];
  else
    return QVariant();
}


/*****************************************
 * TreeModel
 *****************************************/
TreeModel::TreeModel(const QList<QVariant> &data, QObject *parent) : QAbstractItemModel(parent) {
  rootItem = new TreeItem(data, 0);
}

TreeModel::~TreeModel() {
  delete rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const {
  if(!hasIndex(row, column, parent))
    return QModelIndex();
  
  TreeItem *parentItem;
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<TreeItem*>(parent.internalPointer());
  
  TreeItem *childItem = parentItem->child(row);
  if(childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const {
  if(!index.isValid())
    return QModelIndex();
  
  TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
  TreeItem *parentItem = childItem->parent();
  
  if(parentItem == rootItem)
    return QModelIndex();
  
  return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const {
  TreeItem *parentItem;
  if(parent.column() > 0)
    return 0;
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<TreeItem*>(parent.internalPointer());
  
  return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const {
  if(parent.isValid())
    return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
  else
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
  if(!index.isValid())
    return QVariant();

  TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
  return item->data(index.column(), role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const {
  if(!index.isValid())
    return 0;
  else
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return rootItem->data(section, role);
  else
    return QVariant();
}

bool TreeModel::removeRow(int row, const QModelIndex &parent) {
  if(row > rowCount(parent))
    return false;
  
  TreeItem *item;
  if(!parent.isValid())
    item = rootItem;
  else
    item = static_cast<TreeItem*>(parent.internalPointer());
  
  beginRemoveRows(parent, row, row);
  item->removeChild(row);
  endRemoveRows();
  return true;
}

bool TreeModel::removeRows(int row, int count, const QModelIndex &parent) {
  // check if there is work to be done
  if(count == 0)
    return true;

  // out of range check
  if(row + count - 1 > rowCount(parent) || row < 0 || count < 0) 
    return false;
  
  TreeItem *item;
  if(!parent.isValid())
    item = rootItem;
  else
    item = static_cast<TreeItem*>(parent.internalPointer());
  
  
  beginRemoveRows(parent, row, row + count - 1);
  for(int i = row + count - 1; i >= 0; i--) {
    item->removeChild(i);
  }
  endRemoveRows();
  return true;
}

void TreeModel::clear() {
  removeRows(0, rowCount());
}

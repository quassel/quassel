/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include "treemodel.h"

/*****************************************
 *  Buffer Items stored in the Tree Model
 *****************************************/
TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
  : QObject(parent),
    itemData(data),
    _parentItem(parent),
    _flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled)
{
}

TreeItem::TreeItem(TreeItem *parent)
  : QObject(parent),
    itemData(QList<QVariant>()),
    _parentItem(parent),
    _flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled)
{
}

TreeItem::~TreeItem() {
  qDeleteAll(_childItems);
}

uint TreeItem::id() const {
  return (uint)this;
}

void TreeItem::appendChild(TreeItem *item) {
  _childItems.append(item);
  _childHash[item->id()] = item;
  connect(item, SIGNAL(destroyed()),
	  this, SLOT(childDestroyed()));
}

void TreeItem::removeChild(int row) {
  if(row >= _childItems.size())
    return;
  TreeItem *treeitem = _childItems.value(row);
  _childItems.removeAt(row);
  _childHash.remove(_childHash.key(treeitem));
}

TreeItem *TreeItem::child(int row) const {
  if(row < _childItems.size())
    return _childItems.value(row);
  else
    return 0;
}

TreeItem *TreeItem::childById(const uint &id) const {
  if(_childHash.contains(id))
    return _childHash.value(id);
  else
    return 0;
}

int TreeItem::childCount() const {
  return _childItems.count();
}

int TreeItem::row() const {
  if(_parentItem)
    return _parentItem->_childItems.indexOf(const_cast<TreeItem*>(this));
  else
    return 0;
}

TreeItem *TreeItem::parent() {
  return _parentItem;
}

int TreeItem::columnCount() const {
  return itemData.count();
}

QVariant TreeItem::data(int column, int role) const {
  if(role == Qt::DisplayRole && column < itemData.count())
    return itemData[column];
  else
    return QVariant();
}

Qt::ItemFlags TreeItem::flags() const {
  return _flags;
}

void TreeItem::setFlags(Qt::ItemFlags flags) {
  _flags = flags;
}

void TreeItem::childDestroyed() {
  TreeItem *item = static_cast<TreeItem*>(sender());
  removeChild(item->row());
}
  


/*****************************************
 * TreeModel
 *****************************************/
TreeModel::TreeModel(const QList<QVariant> &data, QObject *parent)
  : QAbstractItemModel(parent)
{
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

QModelIndex TreeModel::indexById(uint id, const QModelIndex &parent) const {
  TreeItem *parentItem; 
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<TreeItem *>(parent.internalPointer());
  
  TreeItem *childItem = parentItem->childById(id);
  if(childItem)
    return createIndex(childItem->row(), 0, childItem);
  else
    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const {
  if(!index.isValid())
    return QModelIndex();
  
  TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
  TreeItem *parentItem = static_cast<TreeItem*>(childItem->parent());
  
  if(parentItem == rootItem)
    return QModelIndex();
  
  return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const {
  TreeItem *parentItem;
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
  TreeItem *item;
  if(!index.isValid())
    item = rootItem;
  else
    item = static_cast<TreeItem *>(index.internalPointer());
  return item->flags();
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
  reset();
}

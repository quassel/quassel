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

#include <QDebug>

/*****************************************
 *  Abstract Items of a TreeModel
 *****************************************/
AbstractTreeItem::AbstractTreeItem(AbstractTreeItem *parent)
  : QObject(parent),
    _parentItem(parent),
    _flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled)
{
}

AbstractTreeItem::~AbstractTreeItem() {
  foreach(int key, _childItems.keys()) {
    qDeleteAll(_childItems[key]);
  }
}

quint64 AbstractTreeItem::id() const {
  return (quint64)this;
}

int AbstractTreeItem::defaultColumn() const {
  // invalid QModelIndexes aka rootNodes get their Childs stuffed into column -1
  // all others to 0
  if(_parentItem == 0)
    return -1;
  else
    return 0;
}

void AbstractTreeItem::appendChild(int column, AbstractTreeItem *item) {
  if(!_childItems.contains(column)) {
    _childItems[column] = QList<AbstractTreeItem *>();
    _childHash[column] = QHash<quint64, AbstractTreeItem *>();
  }
  
  _childItems[column].append(item);
  _childHash[column][item->id()] = item;

  connect(item, SIGNAL(destroyed()), this, SLOT(childDestroyed()));
}

void AbstractTreeItem::appendChild(AbstractTreeItem *child) {
  appendChild(defaultColumn(), child);
}

void AbstractTreeItem::removeChild(int column, int row) {
  if(!_childItems.contains(column)
     || _childItems[column].size() <= row)
    return;
  
  AbstractTreeItem *treeitem = _childItems[column].value(row);
  _childItems[column].removeAt(row);
  _childHash[column].remove(_childHash[column].key(treeitem));
  treeitem->deleteLater();
}

void AbstractTreeItem::removeChild(int row) {
  removeChild(defaultColumn(), row);
}

AbstractTreeItem *AbstractTreeItem::child(int column, int row) const {
  if(!_childItems.contains(column)
     || _childItems[column].size() <= row)
    return 0;
  else
    return _childItems[column].value(row);
}

AbstractTreeItem *AbstractTreeItem::child(int row) const {
  return child(defaultColumn(), row);
}

AbstractTreeItem *AbstractTreeItem::childById(int column, const uint &id) const {
  if(!_childHash.contains(column)
     || !_childHash[column].contains(id))
    return 0;
  else
    return _childHash[column].value(id);
}

AbstractTreeItem *AbstractTreeItem::childById(const uint &id) const {
  return childById(defaultColumn(), id);
}

int AbstractTreeItem::childCount(int column) const {
  if(!_childItems.contains(column))
    return 0;
  else
    return _childItems[column].count();
}

int AbstractTreeItem::childCount() const {
  return childCount(defaultColumn());
}

int AbstractTreeItem::column() const {
  if(!_parentItem)
    return -1;

  QHash<int, QList<AbstractTreeItem*> >::const_iterator iter = _parentItem->_childItems.constBegin();
  int pos;
  while(iter != _parentItem->_childItems.constEnd()) {
    pos = iter.value().indexOf(const_cast<AbstractTreeItem *>(this));
    if(pos != -1)
      return iter.key();
    iter++;
  }

  // unable to find us o_O
  return _parentItem->defaultColumn();
}

int AbstractTreeItem::row() const {
  if(!_parentItem)
    return -1;
  else
    return _parentItem->_childItems[column()].indexOf(const_cast<AbstractTreeItem*>(this));
}

AbstractTreeItem *AbstractTreeItem::parent() {
  return _parentItem;
}

Qt::ItemFlags AbstractTreeItem::flags() const {
  return _flags;
}

void AbstractTreeItem::setFlags(Qt::ItemFlags flags) {
  _flags = flags;
}

void AbstractTreeItem::childDestroyed() {
  AbstractTreeItem *item = static_cast<AbstractTreeItem*>(sender());

  if(!item) {
    qWarning() << "AbstractTreeItem::childDestroyed() received null pointer!";
    return;
  }
  _childItems[item->column()].removeAt(item->row());
  _childHash[item->column()].remove(_childHash[item->column()].key(item));
}
  
/*****************************************
 * SimpleTreeItem
 *****************************************/
SimpleTreeItem::SimpleTreeItem(const QList<QVariant> &data, AbstractTreeItem *parent)
  : AbstractTreeItem(parent),
    _itemData(data)
{
}

SimpleTreeItem::~SimpleTreeItem() {
}

QVariant SimpleTreeItem::data(int column, int role) const {
  if(role == Qt::DisplayRole && column < _itemData.count())
    return _itemData[column];
  else
    return QVariant();
}

int SimpleTreeItem::columnCount() const {
  return _itemData.count();
}

/*****************************************
 * PropertyMapItem
 *****************************************/
PropertyMapItem::PropertyMapItem(const QStringList &propertyOrder, AbstractTreeItem *parent)
  : AbstractTreeItem(parent),
    _propertyOrder(propertyOrder)
{
}

PropertyMapItem::PropertyMapItem(AbstractTreeItem *parent)
  : AbstractTreeItem(parent),
    _propertyOrder(QStringList())
{
}


PropertyMapItem::~PropertyMapItem() {
}
  
QVariant PropertyMapItem::data(int column, int role) const {
  if(column >= columnCount())
    return QVariant();

  if(role != Qt::DisplayRole)
    return QVariant();

  return property(_propertyOrder[column].toAscii());
}

int PropertyMapItem::columnCount() const {
  return _propertyOrder.count();
}
  
void PropertyMapItem::appendProperty(const QString &property) {
  _propertyOrder << property;
}



/*****************************************
 * TreeModel
 *****************************************/
TreeModel::TreeModel(const QList<QVariant> &data, QObject *parent)
  : QAbstractItemModel(parent)
{
  rootItem = new SimpleTreeItem(data, 0);
}

TreeModel::~TreeModel() {
  delete rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const {
  if(!hasIndex(row, column, parent))
    return QModelIndex();
  
  AbstractTreeItem *parentItem;
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());
  
  AbstractTreeItem *childItem = parentItem->child(parent.column(), row);

  if(childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

QModelIndex TreeModel::indexById(uint id, const QModelIndex &parent) const {
  AbstractTreeItem *parentItem; 
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<AbstractTreeItem *>(parent.internalPointer());
  
  AbstractTreeItem *childItem = parentItem->childById(parent.column(), id);
  
  if(childItem)
    return createIndex(childItem->row(), 0, childItem);
  else
    return QModelIndex();
}

QModelIndex TreeModel::indexByItem(AbstractTreeItem *item) const {
  if(item == 0) {
    qWarning() << "TreeModel::indexByItem(AbstractTreeItem *item) received NULL-Pointer";
    return QModelIndex();
  }
  
  if(item == rootItem)
    return QModelIndex();
  else
    return createIndex(item->row(), 0, item);

}

QModelIndex TreeModel::parent(const QModelIndex &index) const {
  if(!index.isValid())
    return QModelIndex();
  
  AbstractTreeItem *childItem = static_cast<AbstractTreeItem *>(index.internalPointer());
  AbstractTreeItem *parentItem = static_cast<AbstractTreeItem *>(childItem->parent());
  
  if(parentItem == rootItem)
    return QModelIndex();
  
  return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const {
  AbstractTreeItem *parentItem;
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());

  return parentItem->childCount(parent.column());
}

int TreeModel::columnCount(const QModelIndex &parent) const {
  if(parent.isValid())
    return static_cast<AbstractTreeItem*>(parent.internalPointer())->columnCount();
  else
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
  if(!index.isValid())
    return QVariant();

  AbstractTreeItem *item = static_cast<AbstractTreeItem*>(index.internalPointer());
  return item->data(index.column(), role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const {
  AbstractTreeItem *item;
  if(!index.isValid())
    item = rootItem;
  else
    item = static_cast<AbstractTreeItem *>(index.internalPointer());
  return item->flags();
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return rootItem->data(section, role);
  else
    return QVariant();
}

void TreeModel::itemDataChanged(int column) {
  AbstractTreeItem *item = qobject_cast<AbstractTreeItem *>(sender());
  QModelIndex itemIndex;

  if(item == rootItem) 
    itemIndex = QModelIndex();
  else
    itemIndex = createIndex(item->row(), column, item);

  emit dataChanged(itemIndex, itemIndex);
}

void TreeModel::appendChild(AbstractTreeItem *parent, AbstractTreeItem *child) {
  if(parent == 0 or child == 0) {
    qWarning() << "TreeModel::apendChild(parent, child) parent and child have to be valid pointers!" << parent << child;
    return;
  }

  int nextRow = parent->childCount();
  beginInsertRows(indexByItem(parent), nextRow, nextRow);
  parent->appendChild(child);
  endInsertRows();

  connect(child, SIGNAL(dataChanged(int)),
	  this, SLOT(itemDataChanged(int)));
}


bool TreeModel::removeRow(int row, const QModelIndex &parent) {
  if(row > rowCount(parent))
    return false;
  
  AbstractTreeItem *item;
  if(!parent.isValid())
    item = rootItem;
  else
    item = static_cast<AbstractTreeItem*>(parent.internalPointer());
  
  beginRemoveRows(parent, row, row);
  item->removeChild(parent.column(), row);
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
  
  AbstractTreeItem *item;
  if(!parent.isValid())
    item = rootItem;
  else
    item = static_cast<AbstractTreeItem *>(parent.internalPointer());
  
  
  beginRemoveRows(parent, row, row + count - 1);

  for(int i = row + count - 1; i >= 0; i--) {
    item->removeChild(parent.column(), i);
  }
  endRemoveRows();
  return true;
}

void TreeModel::clear() {
  removeRows(0, rowCount());
  reset();
}

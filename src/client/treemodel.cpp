/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "treemodel.h"

#include <utility>

#include <QCoreApplication>
#include <QDebug>

#include "quassel.h"

class RemoveChildLaterEvent : public QEvent
{
public:
    RemoveChildLaterEvent(AbstractTreeItem* child)
        : QEvent(QEvent::User)
        , _child(child){};
    inline AbstractTreeItem* child() { return _child; }

private:
    AbstractTreeItem* _child;
};

/*****************************************
 *  Abstract Items of a TreeModel
 *****************************************/
AbstractTreeItem::AbstractTreeItem(AbstractTreeItem* parent)
    : QObject(parent)
    , _flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled)
    , _treeItemFlags(nullptr)
{}

bool AbstractTreeItem::newChild(AbstractTreeItem* item)
{
    int newRow = childCount();
    emit beginAppendChilds(newRow, newRow);
    _childItems.append(item);
    emit endAppendChilds();
    return true;
}

bool AbstractTreeItem::newChilds(const QList<AbstractTreeItem*>& items)
{
    if (items.isEmpty())
        return false;

    int nextRow = childCount();
    int lastRow = nextRow + items.count() - 1;

    emit beginAppendChilds(nextRow, lastRow);
    _childItems << items;
    emit endAppendChilds();

    return true;
}

bool AbstractTreeItem::removeChild(int row)
{
    if (row < 0 || childCount() <= row)
        return false;

    child(row)->removeAllChilds();
    emit beginRemoveChilds(row, row);
    AbstractTreeItem* treeitem = _childItems.takeAt(row);
    delete treeitem;
    emit endRemoveChilds();

    checkForDeletion();

    return true;
}

void AbstractTreeItem::removeAllChilds()
{
    const int numChilds = childCount();

    if (numChilds == 0)
        return;

    AbstractTreeItem* child;

    QList<AbstractTreeItem*>::iterator childIter;

    childIter = _childItems.begin();
    while (childIter != _childItems.end()) {
        child = *childIter;
        child->setTreeItemFlags(nullptr);  // disable self deletion, as this would only fuck up consitency and the child gets deleted anyways
        child->removeAllChilds();
        ++childIter;
    }

    emit beginRemoveChilds(0, numChilds - 1);
    childIter = _childItems.begin();
    while (childIter != _childItems.end()) {
        child = *childIter;
        childIter = _childItems.erase(childIter);
        delete child;
    }
    emit endRemoveChilds();

    checkForDeletion();
}

void AbstractTreeItem::removeChildLater(AbstractTreeItem* child)
{
    Q_ASSERT(child);
    QCoreApplication::postEvent(this, new RemoveChildLaterEvent(child));
}

void AbstractTreeItem::customEvent(QEvent* event)
{
    if (event->type() != QEvent::User)
        return;

    event->accept();

    auto* removeEvent = static_cast<RemoveChildLaterEvent*>(event);
    int childRow = _childItems.indexOf(removeEvent->child());
    if (childRow == -1)
        return;

    // since we are called asynchronously we have to recheck if the item in question still has no childs
    if (removeEvent->child()->childCount())
        return;

    removeChild(childRow);
}

bool AbstractTreeItem::reParent(AbstractTreeItem* newParent)
{
    // currently we support only re parenting if the child that's about to be
    // adopted does not have any children itself.
    if (childCount() != 0) {
        qDebug() << "AbstractTreeItem::reParent(): cannot reparent" << this << "with children.";
        return false;
    }

    int oldRow = row();
    if (oldRow == -1)
        return false;

    emit parent()->beginRemoveChilds(oldRow, oldRow);
    parent()->_childItems.removeAt(oldRow);
    emit parent()->endRemoveChilds();

    AbstractTreeItem* oldParent = parent();
    setParent(newParent);

    bool success = newParent->newChild(this);
    if (!success)
        qWarning() << "AbstractTreeItem::reParent(): failed to attach to new parent after removing from old parent! this:" << this
                   << "new parent:" << newParent;

    if (oldParent)
        oldParent->checkForDeletion();

    return success;
}

AbstractTreeItem* AbstractTreeItem::child(int row) const
{
    if (childCount() <= row)
        return nullptr;
    else
        return _childItems[row];
}

int AbstractTreeItem::childCount(int column) const
{
    if (column > 0)
        return 0;
    else
        return _childItems.count();
}

int AbstractTreeItem::row() const
{
    if (!parent()) {
        qWarning() << "AbstractTreeItem::row():" << this << "has no parent AbstractTreeItem as it's parent! parent is" << QObject::parent();
        return -1;
    }

    int row_ = parent()->_childItems.indexOf(const_cast<AbstractTreeItem*>(this));
    if (row_ == -1)
        qWarning() << "AbstractTreeItem::row():" << this << "is not in the child list of" << QObject::parent();
    return row_;
}

void AbstractTreeItem::dumpChildList()
{
    qDebug() << "==== Childlist for Item:" << this << "====";
    if (childCount() > 0) {
        AbstractTreeItem* child;
        QList<AbstractTreeItem*>::const_iterator childIter = _childItems.constBegin();
        while (childIter != _childItems.constEnd()) {
            child = *childIter;
            qDebug() << "Row:" << child->row() << child << child->data(0, Qt::DisplayRole);
            ++childIter;
        }
    }
    qDebug() << "==== End Of Childlist ====";
}

/*****************************************
 * SimpleTreeItem
 *****************************************/
SimpleTreeItem::SimpleTreeItem(QList<QVariant> data, AbstractTreeItem* parent)
    : AbstractTreeItem(parent)
    , _itemData(std::move(data))
{}

QVariant SimpleTreeItem::data(int column, int role) const
{
    if (column >= columnCount() || role != Qt::DisplayRole)
        return QVariant();
    else
        return _itemData[column];
}

bool SimpleTreeItem::setData(int column, const QVariant& value, int role)
{
    if (column > columnCount() || role != Qt::DisplayRole)
        return false;

    if (column == columnCount())
        _itemData.append(value);
    else
        _itemData[column] = value;

    emit dataChanged(column);
    return true;
}

int SimpleTreeItem::columnCount() const
{
    return _itemData.count();
}

/*****************************************
 * PropertyMapItem
 *****************************************/
PropertyMapItem::PropertyMapItem(AbstractTreeItem* parent)
    : AbstractTreeItem(parent)
{}

QVariant PropertyMapItem::data(int column, int role) const
{
    if (column >= columnCount())
        return QVariant();

    switch (role) {
    case Qt::ToolTipRole:
        return toolTip(column);
    case Qt::DisplayRole:
    case TreeModel::SortRole:  // fallthrough, since SortRole should default to DisplayRole
        return property(propertyOrder()[column].toLatin1());
    default:
        return QVariant();
    }
}

bool PropertyMapItem::setData(int column, const QVariant& value, int role)
{
    if (column >= columnCount() || role != Qt::DisplayRole)
        return false;

    setProperty(propertyOrder()[column].toLatin1(), value);
    emit dataChanged(column);
    return true;
}

int PropertyMapItem::columnCount() const
{
    return propertyOrder().count();
}

/*****************************************
 * TreeModel
 *****************************************/
TreeModel::TreeModel(const QList<QVariant>& data, QObject* parent)
    : QAbstractItemModel(parent)
    , _childStatus(QModelIndex(), 0, 0, 0)
    , _aboutToRemoveOrInsert(false)
{
    rootItem = new SimpleTreeItem(data, nullptr);
    connectItem(rootItem);

    if (Quassel::isOptionSet("debugmodel")) {
        connect(this, &QAbstractItemModel::rowsAboutToBeInserted, this, &TreeModel::debug_rowsAboutToBeInserted);
        connect(this, &QAbstractItemModel::rowsAboutToBeRemoved, this, &TreeModel::debug_rowsAboutToBeRemoved);
        connect(this, &QAbstractItemModel::rowsInserted, this, &TreeModel::debug_rowsInserted);
        connect(this, &QAbstractItemModel::rowsRemoved, this, &TreeModel::debug_rowsRemoved);
        connect(this, &QAbstractItemModel::dataChanged, this, &TreeModel::debug_dataChanged);
    }
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

AbstractTreeItem* TreeModel::root() const
{
    return rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
        return {};

    AbstractTreeItem* parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());

    AbstractTreeItem* childItem = parentItem->child(row);

    if (childItem)
        return createIndex(row, column, childItem);
    else
        return {};
}

QModelIndex TreeModel::indexByItem(AbstractTreeItem* item) const
{
    if (item == nullptr) {
        qWarning() << "TreeModel::indexByItem(AbstractTreeItem *item) received NULL-Pointer";
        return {};
    }

    if (item == rootItem)
        return {};
    else
        return createIndex(item->row(), 0, item);
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        // ModelTest does this
        // qWarning() << "TreeModel::parent(): has been asked for the rootItems Parent!";
        return {};
    }

    auto* childItem = static_cast<AbstractTreeItem*>(index.internalPointer());
    AbstractTreeItem* parentItem = childItem->parent();

    Q_ASSERT(parentItem);
    if (parentItem == rootItem)
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
    AbstractTreeItem* parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());

    return parentItem->childCount(parent.column());
}

int TreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return rootItem->columnCount();
    // since there the Qt Views don't draw more columns than the header has columns
    // we can be lazy and simply return the count of header columns
    // actually this gives us more freedom cause we don't have to ensure that a rows parent
    // has equal or more columns than that row

    //   AbstractTreeItem *parentItem;
    //   if(!parent.isValid())
    //     parentItem = rootItem;
    //   else
    //     parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());
    //   return parentItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto* item = static_cast<AbstractTreeItem*>(index.internalPointer());
    return item->data(index.column(), role);
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    auto* item = static_cast<AbstractTreeItem*>(index.internalPointer());
    return item->setData(index.column(), value, role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return rootItem->flags() & Qt::ItemIsDropEnabled;
    }
    else {
        auto* item = static_cast<AbstractTreeItem*>(index.internalPointer());
        return item->flags();
    }
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section, role);
    else
        return QVariant();
}

void TreeModel::itemDataChanged(int column)
{
    auto* item = qobject_cast<AbstractTreeItem*>(sender());
    QModelIndex leftIndex, rightIndex;

    if (item == rootItem)
        return;

    if (column == -1) {
        leftIndex = createIndex(item->row(), 0, item);
        rightIndex = createIndex(item->row(), item->columnCount() - 1, item);
    }
    else {
        leftIndex = createIndex(item->row(), column, item);
        rightIndex = leftIndex;
    }

    emit dataChanged(leftIndex, rightIndex);
}

void TreeModel::connectItem(AbstractTreeItem* item)
{
    connect(item, &AbstractTreeItem::dataChanged, this, &TreeModel::itemDataChanged);

    connect(item, &AbstractTreeItem::beginAppendChilds, this, &TreeModel::beginAppendChilds);
    connect(item, &AbstractTreeItem::endAppendChilds, this, &TreeModel::endAppendChilds);

    connect(item, &AbstractTreeItem::beginRemoveChilds, this, &TreeModel::beginRemoveChilds);
    connect(item, &AbstractTreeItem::endRemoveChilds, this, &TreeModel::endRemoveChilds);
}

void TreeModel::beginAppendChilds(int firstRow, int lastRow)
{
    auto* parentItem = qobject_cast<AbstractTreeItem*>(sender());
    if (!parentItem) {
        qWarning() << "TreeModel::beginAppendChilds(): cannot append Children to unknown parent";
        return;
    }

    QModelIndex parent = indexByItem(parentItem);
    Q_ASSERT(!_aboutToRemoveOrInsert);

    _aboutToRemoveOrInsert = true;
    _childStatus = ChildStatus(parent, rowCount(parent), firstRow, lastRow);
    beginInsertRows(parent, firstRow, lastRow);
}

void TreeModel::endAppendChilds()
{
    auto* parentItem = qobject_cast<AbstractTreeItem*>(sender());
    if (!parentItem) {
        qWarning() << "TreeModel::endAppendChilds(): cannot append Children to unknown parent";
        return;
    }
    Q_ASSERT(_aboutToRemoveOrInsert);
    ChildStatus cs = _childStatus;
#ifndef QT_NO_DEBUG
    QModelIndex parent = indexByItem(parentItem);
    Q_ASSERT(cs.parent == parent);
    Q_ASSERT(rowCount(parent) == cs.childCount + cs.end - cs.start + 1);
#endif
    _aboutToRemoveOrInsert = false;
    for (int i = cs.start; i <= cs.end; i++) {
        connectItem(parentItem->child(i));
    }
    endInsertRows();
}

void TreeModel::beginRemoveChilds(int firstRow, int lastRow)
{
    auto* parentItem = qobject_cast<AbstractTreeItem*>(sender());
    if (!parentItem) {
        qWarning() << "TreeModel::beginRemoveChilds(): cannot append Children to unknown parent";
        return;
    }

    for (int i = firstRow; i <= lastRow; i++) {
        disconnect(parentItem->child(i), nullptr, this, nullptr);
    }

    // consitency checks
    QModelIndex parent = indexByItem(parentItem);
    Q_ASSERT(firstRow <= lastRow);
    Q_ASSERT(parentItem->childCount() > lastRow);
    Q_ASSERT(!_aboutToRemoveOrInsert);
    _aboutToRemoveOrInsert = true;
    _childStatus = ChildStatus(parent, rowCount(parent), firstRow, lastRow);

    beginRemoveRows(parent, firstRow, lastRow);
}

void TreeModel::endRemoveChilds()
{
    auto* parentItem = qobject_cast<AbstractTreeItem*>(sender());
    if (!parentItem) {
        qWarning() << "TreeModel::endRemoveChilds(): cannot remove Children from unknown parent";
        return;
    }

    // concistency checks
    Q_ASSERT(_aboutToRemoveOrInsert);
#ifndef QT_NO_DEBUG
    ChildStatus cs = _childStatus;
    QModelIndex parent = indexByItem(parentItem);
    Q_ASSERT(cs.parent == parent);
    Q_ASSERT(rowCount(parent) == cs.childCount - cs.end + cs.start - 1);
#endif
    _aboutToRemoveOrInsert = false;

    endRemoveRows();
}

void TreeModel::clear()
{
    rootItem->removeAllChilds();
}

void TreeModel::debug_rowsAboutToBeInserted(const QModelIndex& parent, int start, int end)
{
    qDebug() << "debug_rowsAboutToBeInserted" << parent << parent.internalPointer() << parent.data().toString() << rowCount(parent) << start
             << end;
}

void TreeModel::debug_rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    AbstractTreeItem* parentItem;
    parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());
    if (!parentItem)
        parentItem = rootItem;
    qDebug() << "debug_rowsAboutToBeRemoved" << parent << parentItem << parent.data().toString() << rowCount(parent) << start << end;

    // Make sure model is valid first
    if (!parent.model()) {
        qDebug() << "Parent model is not valid!" << end;
        return;
    }

    QModelIndex child;
    for (int i = end; i >= start; i--) {
        child = parent.model()->index(i, 0, parent);
        Q_ASSERT(parentItem->child(i));
        qDebug() << ">>>" << i << child << child.data().toString();
    }
}

void TreeModel::debug_rowsInserted(const QModelIndex& parent, int start, int end)
{
    AbstractTreeItem* parentItem;
    parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());
    if (!parentItem)
        parentItem = rootItem;
    qDebug() << "debug_rowsInserted:" << parent << parentItem << parent.data().toString() << rowCount(parent) << start << end;

    // Make sure model is valid first
    if (!parent.model()) {
        qDebug() << "Parent model is not valid!" << end;
        return;
    }

    QModelIndex child;
    for (int i = start; i <= end; i++) {
        child = parent.model()->index(i, 0, parent);
        Q_ASSERT(parentItem->child(i));
        qDebug() << "<<<" << i << child << child.data().toString();
    }
}

void TreeModel::debug_rowsRemoved(const QModelIndex& parent, int start, int end)
{
    qDebug() << "debug_rowsRemoved" << parent << parent.internalPointer() << parent.data().toString() << rowCount(parent) << start << end;
}

void TreeModel::debug_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    qDebug() << "debug_dataChanged" << topLeft << bottomRight;
    QStringList displayData;
    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        displayData = QStringList();
        for (int column = topLeft.column(); column <= bottomRight.column(); column++) {
            displayData << data(topLeft.sibling(row, column), Qt::DisplayRole).toString();
        }
        qDebug() << "  row:" << row << displayData;
    }
}

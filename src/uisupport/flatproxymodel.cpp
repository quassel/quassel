/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "flatproxymodel.h"

#include <QItemSelection>
#include <QDebug>

FlatProxyModel::FlatProxyModel(QObject *parent)
    : QAbstractProxyModel(parent),
    _rootSourceItem(0)
{
}


QModelIndex FlatProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();

    SourceItem *sourceItem = sourceToInternal(sourceIndex);
    Q_ASSERT(sourceItem);
    return createIndex(sourceItem->pos(), sourceIndex.column(), sourceItem);
}


QModelIndex FlatProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();

    Q_ASSERT(proxyIndex.model() == this);
    Q_ASSERT(_rootSourceItem);

    int row = proxyIndex.row();
    QModelIndex sourceParent;
    SourceItem *sourceItem = _rootSourceItem->findChild(row);
    while (sourceItem) {
        if (sourceItem->pos() == row) {
            return sourceModel()->index(sourceItem->sourceRow(), proxyIndex.column(), sourceParent);
        }
        else {
            sourceParent = sourceModel()->index(sourceItem->sourceRow(), 0, sourceParent);
            sourceItem = sourceItem->findChild(row);
        }
    }

    qWarning() << "FlatProxyModel::mapToSource(): couldn't find source index for" << proxyIndex;
    Q_ASSERT(false);
    return QModelIndex(); // make compilers happy :)
}


bool FlatProxyModel::_RangeRect::operator<(const FlatProxyModel::_RangeRect &other) const
{
    if (left != other.left)
        return left < other.left;

    if (right != other.right)
        return right < other.right;

    if (top != other.top)
        return top < other.top;

    // top == other.top
    return bottom < other.bottom;
}


QItemSelection FlatProxyModel::mapSelectionFromSource(const QItemSelection &sourceSelection) const
{
    QList<_RangeRect> newRanges;
    QHash<QModelIndex, SourceItem *> itemLookup;
    // basics steps of the loop:
    // 1. convert each source ItemSelectionRange to a mapped Range (internal one for easier post processing)
    // 2. insert it into the list of previously mapped Ranges sorted by top location
    for (int i = 0; i < sourceSelection.count(); i++) {
        const QItemSelectionRange &currentRange = sourceSelection[i];
        QModelIndex currentParent = currentRange.topLeft().parent();
        Q_ASSERT(currentParent == currentRange.bottomRight().parent());

        SourceItem *parentItem = 0;
        if (!itemLookup.contains(currentParent)) {
            parentItem = sourceToInternal(currentParent);
            itemLookup[currentParent] = parentItem;
        }
        else {
            parentItem = itemLookup[currentParent];
        }

        _RangeRect newRange = { currentRange.topLeft().column(), currentRange.bottomRight().column(),
                                currentRange.topLeft().row(), currentRange.bottomRight().row(),
                                parentItem->child(currentRange.topLeft().row()), parentItem->child(currentRange.bottomRight().row()) };

        if (newRanges.isEmpty()) {
            newRanges << newRange;
            continue;
        }

        _RangeRect &first = newRanges[0];
        if (newRange < first) {
            newRanges.prepend(newRange);
            continue;
        }

        bool inserted = false;
        for (int j = 0; j < newRanges.count() - 1; j++) {
            _RangeRect &a = newRanges[j];
            _RangeRect &b = newRanges[j + 1];

            if (a < newRange && newRange < b) {
                newRanges[j + 1] = newRange;
                inserted = true;
                break;
            }
        }
        if (inserted)
            continue;

        _RangeRect &last = newRanges[newRanges.count() - 1];
        if (last < newRange) {
            newRanges.append(newRange);
            continue;
        }

        Q_ASSERT(false);
    }

    // we've got a sorted list of ranges now. so we can easily check if there is a possibility to merge
    for (int i = newRanges.count() - 1; i > 0; i--) {
        _RangeRect &a = newRanges[i - 1];
        _RangeRect &b = newRanges[i];
        if (a.left != b.left || a.right != b.right)
            continue;

        if (a.bottom < b.top - 1) {
            continue;
        }

        // all merge checks passed!
        if (b.bottom > a.bottom) {
            a.bottom = b.bottom;
            a.bottomItem = b.bottomItem;
        } // otherwise b is totally enclosed in a -> nothing to do but drop b.
        newRanges.removeAt(i);
    }

    QItemSelection proxySelection;
    for (int i = 0; i < newRanges.count(); i++) {
        _RangeRect &r = newRanges[i];
        proxySelection << QItemSelectionRange(createIndex(r.top, r.left, r.topItem), createIndex(r.bottom, r.right, r.bottomItem));
    }
    return proxySelection;
}


QItemSelection FlatProxyModel::mapSelectionToSource(const QItemSelection &proxySelection) const
{
    QItemSelection sourceSelection;

    for (int i = 0; i < proxySelection.count(); i++) {
        const QItemSelectionRange &range = proxySelection[i];

        SourceItem *topLeftItem = 0;
        SourceItem *bottomRightItem = 0;
        SourceItem *currentItem = static_cast<SourceItem *>(range.topLeft().internalPointer());
        int row = range.topLeft().row();
        int left = range.topLeft().column();
        int right = range.bottomRight().column();
        while (currentItem && row <= range.bottomRight().row()) {
            Q_ASSERT(currentItem->pos() == row);
            if (!topLeftItem)
                topLeftItem = currentItem;

            if (currentItem->parent() == topLeftItem->parent()) {
                bottomRightItem = currentItem;
            }
            else {
                Q_ASSERT(topLeftItem && bottomRightItem);
                sourceSelection << QItemSelectionRange(mapToSource(createIndex(topLeftItem->pos(), left, topLeftItem)), mapToSource(createIndex(bottomRightItem->pos(), right, bottomRightItem)));
                topLeftItem = 0;
                bottomRightItem = 0;
            }

            // update loop vars
            currentItem = currentItem->next();
            row++;
        }

        Q_ASSERT(topLeftItem && bottomRightItem); // there should be one range left.
        sourceSelection << QItemSelectionRange(mapToSource(createIndex(topLeftItem->pos(), left, topLeftItem)), mapToSource(createIndex(bottomRightItem->pos(), right, bottomRightItem)));
    }

    return sourceSelection;
}


void FlatProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (QAbstractProxyModel::sourceModel()) {
        disconnect(QAbstractProxyModel::sourceModel(), 0, this, 0);
    }

    QAbstractProxyModel::setSourceModel(sourceModel);

    emit layoutAboutToBeChanged();
    removeSubTree(QModelIndex(), false /* don't emit removeRows() */);
    insertSubTree(QModelIndex(), false /* don't emit insertRows() */);
    emit layoutChanged();

    if (sourceModel) {
        connect(sourceModel, SIGNAL(columnsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(on_columnsAboutToBeInserted(const QModelIndex &, int, int)));
        connect(sourceModel, SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(on_columnsAboutToBeRemoved(const QModelIndex &, int, int)));
        connect(sourceModel, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
            this, SLOT(on_columnsInserted(const QModelIndex &, int, int)));
        connect(sourceModel, SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
            this, SLOT(on_columnsRemoved(const QModelIndex &, int, int)));

        connect(sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(on_dataChanged(const QModelIndex &, const QModelIndex &)));
        // on_headerDataChanged(Qt::Orientation orientation, int first, int last)

        connect(sourceModel, SIGNAL(layoutAboutToBeChanged()),
            this, SLOT(on_layoutAboutToBeChanged()));
        connect(sourceModel, SIGNAL(layoutChanged()),
            this, SLOT(on_layoutChanged()));

        connect(sourceModel, SIGNAL(modelAboutToBeReset()),
            this, SLOT(on_modelAboutToBeReset()));
        // void on_modelReset()

        connect(sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(on_rowsAboutToBeInserted(const QModelIndex &, int, int)));
        connect(sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(on_rowsAboutToBeRemoved(const QModelIndex &, int, int)));
        connect(sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(on_rowsInserted(const QModelIndex &, int, int)));
        connect(sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this, SLOT(on_rowsRemoved(const QModelIndex &, int, int)));
    }
}


void FlatProxyModel::insertSubTree(const QModelIndex &source_idx, bool emitInsert)
{
    SourceItem *newSubTree = new SourceItem(source_idx.row(), sourceToInternal(sourceModel()->parent(source_idx)));

    if (newSubTree->parent()) {
        newSubTree->setPos(newSubTree->parent()->pos() + source_idx.row() + 1);
    }
    SourceItem *lastItem = insertSubTreeHelper(newSubTree, newSubTree, source_idx);

    Q_ASSERT(lastItem);
    Q_ASSERT(lastItem->next() == 0);

    if (emitInsert)
        beginInsertRows(QModelIndex(), newSubTree->pos(), lastItem->pos());

    if (newSubTree->parent()) {
        if (newSubTree->parent()->childCount() > source_idx.row()) {
            SourceItem *next = newSubTree->parent()->child(source_idx.row());
            lastItem->setNext(next);
            int nextPos = lastItem->pos() + 1;
            while (next) {
                next->setPos(nextPos);
                next = next->next();
                nextPos++;
            }
        }
        if (source_idx.row() > 0) {
            SourceItem *previous = newSubTree->parent()->child(source_idx.row() - 1);
            while (previous->childCount() > 0) {
                previous = previous->child(previous->childCount() - 1);
            }
            previous->setNext(newSubTree);
        }
        else {
            newSubTree->parent()->setNext(newSubTree);
        }
    }
    else {
        _rootSourceItem = newSubTree;
    }

    if (emitInsert)
        endInsertRows();
}


FlatProxyModel::SourceItem *FlatProxyModel::insertSubTreeHelper(SourceItem *parentItem, SourceItem *lastItem_, const QModelIndex &source_idx)
{
    SourceItem *lastItem = lastItem_;
    SourceItem *newItem = 0;
    for (int row = 0; row < sourceModel()->rowCount(source_idx); row++) {
        newItem = new SourceItem(row, parentItem);
        newItem->setPos(lastItem->pos() + 1);
        lastItem->setNext(newItem);
        lastItem = insertSubTreeHelper(newItem, newItem, sourceModel()->index(row, 0, source_idx));
    }
    return lastItem;
}


void FlatProxyModel::removeSubTree(const QModelIndex &source_idx, bool emitRemove)
{
    SourceItem *sourceItem = sourceToInternal(source_idx);
    if (!sourceItem)
        return;

    SourceItem *prevItem = sourceItem->parent();
    if (sourceItem->sourceRow() > 0) {
        prevItem = prevItem->child(sourceItem->sourceRow() - 1);
        while (prevItem->childCount() > 0) {
            prevItem = prevItem->child(prevItem->childCount() - 1);
        }
    }

    SourceItem *lastItem = sourceItem;
    while (lastItem->childCount() > 0) {
        lastItem = lastItem->child(lastItem->childCount() - 1);
    }

    if (emitRemove)
        beginRemoveRows(QModelIndex(), sourceItem->pos(), lastItem->pos());

    int nextPos = 0;
    if (prevItem) {
        prevItem->setNext(lastItem->next());
        nextPos = prevItem->pos() + 1;
    }

    SourceItem *nextItem = lastItem->next();
    while (nextItem) {
        nextItem->setPos(nextPos);
        nextPos++;
        nextItem = nextItem->next();
    }

    sourceItem->parent()->removeChild(sourceItem);
    delete sourceItem;

    if (emitRemove)
        endRemoveRows();
}


QModelIndex FlatProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        qWarning() << "FlatProxyModel::index() called with valid parent:" << parent;
        return QModelIndex();
    }

    if (!_rootSourceItem) {
        qWarning() << "FlatProxyModel::index() while model has no root Item";
        return QModelIndex();
    }

    SourceItem *item = _rootSourceItem;
    while (item->pos() != row) {
        item = item->findChild(row);
        if (!item) {
            qWarning() << "FlatProxyModel::index() no such row:" << row;
            return QModelIndex();
        }
    }

    Q_ASSERT(item->pos() == row);
    return createIndex(row, column, item);
}


QModelIndex FlatProxyModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    // this is a flat model
    return QModelIndex();
}


int FlatProxyModel::rowCount(const QModelIndex &index) const
{
    if (!_rootSourceItem)
        return 0;

    if (index.isValid())
        return 0;

    SourceItem *item = _rootSourceItem;
    while (item->childCount() > 0) {
        item = item->child(item->childCount() - 1);
    }
    return item->pos() + 1;
}


int FlatProxyModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index)
    if (!sourceModel()) {
        return 0;
    }
    else {
        return sourceModel()->columnCount(QModelIndex());
    }
}


FlatProxyModel::SourceItem *FlatProxyModel::sourceToInternal(const QModelIndex &sourceIndex) const
{
    QList<int> childPath;
    for (QModelIndex idx = sourceIndex; idx.isValid(); idx = sourceModel()->parent(idx)) {
        childPath.prepend(idx.row());
    }

    Q_ASSERT(!sourceIndex.isValid() || !childPath.isEmpty());

    SourceItem *item = _rootSourceItem;
    for (int i = 0; i < childPath.count(); i++) {
        item = item->child(childPath[i]);
    }
    return item;
}


void FlatProxyModel::on_columnsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);
    beginInsertColumns(QModelIndex(), start, end);
}


void FlatProxyModel::on_columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);
    beginRemoveColumns(QModelIndex(), start, end);
}


void FlatProxyModel::on_columnsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    endInsertColumns();
}


void FlatProxyModel::on_columnsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    endRemoveRows();
}


void FlatProxyModel::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_ASSERT(sourceModel());
    Q_ASSERT(sourceModel()->parent(topLeft) == sourceModel()->parent(bottomRight));

    SourceItem *topLeftItem = sourceToInternal(topLeft);
    Q_ASSERT(topLeftItem);
    Q_ASSERT(topLeftItem->parent()->childCount() > bottomRight.row());

    QModelIndex proxyTopLeft = createIndex(topLeftItem->pos(), topLeft.column(), topLeftItem);
    QModelIndex proxyBottomRight = createIndex(topLeftItem->pos() + bottomRight.row() - topLeft.row(), bottomRight.column(), topLeftItem->parent()->child(bottomRight.row()));
    emit dataChanged(proxyTopLeft, proxyBottomRight);
}


void FlatProxyModel::on_layoutAboutToBeChanged()
{
    emit layoutAboutToBeChanged();
    removeSubTree(QModelIndex(), false /* don't emit removeRows() */);
}


void FlatProxyModel::on_layoutChanged()
{
    insertSubTree(QModelIndex(), false /* don't emit insertRows() */);
    emit layoutChanged();
}


void FlatProxyModel::on_rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(sourceModel());
    Q_ASSERT(_rootSourceItem);

    SourceItem *sourceItem = sourceToInternal(parent);
    Q_ASSERT(sourceItem);

    beginInsertRows(QModelIndex(), sourceItem->pos() + start + 1, sourceItem->pos() + end + 1);

    SourceItem *prevItem = sourceItem;
    if (start > 0) {
        prevItem = sourceItem->child(start - 1);
        while (prevItem->childCount() > 0) {
            prevItem = prevItem->child(prevItem->childCount() - 1);
        }
    }
    Q_ASSERT(prevItem);

    SourceItem *nextItem = prevItem->next();

    SourceItem *newItem = 0;
    int newPos = prevItem->pos() + 1;
    for (int row = start; row <= end; row++) {
        newItem = new SourceItem(row, sourceItem);
        newItem->setPos(newPos);
        newPos++;
        prevItem->setNext(newItem);
        prevItem = newItem;
    }
    prevItem->setNext(nextItem);

    while (nextItem) {
        nextItem->setPos(newPos);
        newPos++;
        nextItem = nextItem->next();
    }
}


void FlatProxyModel::on_rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(sourceModel());
    Q_ASSERT(_rootSourceItem);

    SourceItem *sourceItem = sourceToInternal(parent);
    beginRemoveRows(QModelIndex(), sourceItem->pos() + start + 1, sourceItem->pos() + end + 1);

    // sanity check - if that check fails our indexes would be messed up
    for (int row = start; row <= end; row++) {
        if (sourceItem->child(row)->childCount() > 0) {
            qWarning() << "on_rowsAboutToBeRemoved(): sourceModel() removed rows which have children on their own!" << sourceModel()->index(row, 0, parent);
            Q_ASSERT(false);
        }
    }
}


void FlatProxyModel::on_rowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(sourceModel());
    Q_ASSERT(_rootSourceItem);

    SourceItem *sourceItem = sourceToInternal(parent);
    Q_ASSERT(sourceItem);
    Q_UNUSED(sourceItem);

    // sanity check - if that check fails our indexes would be messed up
    for (int row = start; row <= end; row++) {
        QModelIndex child = sourceModel()->index(row, 0, parent);
        if (sourceModel()->rowCount(child) > 0) {
            qWarning() << "on_rowsInserted(): sourceModel() inserted rows which already have children on their own!" << child;
            Q_ASSERT(false);
        }
    }

    endInsertRows();
}


void FlatProxyModel::on_rowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(sourceModel());
    Q_ASSERT(_rootSourceItem);

    SourceItem *sourceItem = sourceToInternal(parent);
    Q_ASSERT(sourceItem);

    SourceItem *prevItem = sourceItem;
    if (start > 0) {
        prevItem = sourceItem->child(start - 1);
        while (prevItem->childCount() > 0) {
            prevItem = prevItem->child(prevItem->childCount() - 1);
        }
    }
    Q_ASSERT(prevItem);

    SourceItem *nextItem = sourceItem->child(end)->next();

    int newPos = prevItem->pos() + 1;
    prevItem->setNext(nextItem);

    while (nextItem) {
        nextItem->setPos(newPos);
        newPos++;
        nextItem = nextItem->next();
    }

    SourceItem *childItem;
    for (int row = start; row <= end; row++) {
        childItem = sourceItem->_childs.takeAt(start);
        delete childItem;
    }

    endRemoveRows();
}


// integrity Tets
void FlatProxyModel::linkTest() const
{
    qDebug() << "Checking FlatProxyModel for linklist integrity";
    if (!_rootSourceItem)
        return;

    int pos = -1;
    SourceItem *item = _rootSourceItem;
    while (true) {
        qDebug() << item << ":" << item->pos() << "==" << pos;
        Q_ASSERT(item->pos() == pos);
        pos++;
        if (!item->next())
            break;
        item = item->next();
    }
    qDebug() << "Last item in linklist:" << item << item->pos();

    int lastPos = item->pos();
    item = _rootSourceItem;
    while (item->childCount() > 0) {
        item = item->child(item->childCount() - 1);
    }
    qDebug() << "Last item in tree:" << item << item->pos();
    Q_ASSERT(lastPos == item->pos());
    Q_UNUSED(lastPos);

    qDebug() << "success!";
}


void FlatProxyModel::completenessTest() const
{
    qDebug() << "Checking FlatProxyModel for Completeness:";
    int pos = -1;
    checkChildCount(QModelIndex(), _rootSourceItem, pos);
    qDebug() << "success!";
}


void FlatProxyModel::checkChildCount(const QModelIndex &index, const SourceItem *item, int &pos) const
{
    if (!sourceModel())
        return;

    qDebug() << index  << "(Item:" << item << "):" << sourceModel()->rowCount(index) << "==" << item->childCount();
    qDebug() << "ProxyPos:" << item->pos() << "==" << pos;
    Q_ASSERT(sourceModel()->rowCount(index) == item->childCount());

    for (int row = 0; row < sourceModel()->rowCount(index); row++) {
        pos++;
        checkChildCount(sourceModel()->index(row, 0, index), item->child(row), pos);
    }
}


// ========================================
//  SourceItem
// ========================================
FlatProxyModel::SourceItem::SourceItem(int row, SourceItem *parent)
    : _parent(parent),
    _pos(-1),
    _next(0)
{
    if (parent) {
        parent->_childs.insert(row, this);
    }
}


FlatProxyModel::SourceItem::~SourceItem()
{
    for (int i = 0; i < childCount(); i++) {
        delete child(i);
    }
    _childs.clear();
}


int FlatProxyModel::SourceItem::sourceRow() const
{
    if (!parent())
        return -1;
    else
        return parent()->_childs.indexOf(const_cast<FlatProxyModel::SourceItem *>(this));
}


FlatProxyModel::SourceItem *FlatProxyModel::SourceItem::findChild(int proxyPos) const
{
    Q_ASSERT(proxyPos > pos());
    Q_ASSERT(_childs.count() > 0);
    Q_ASSERT(proxyPos >= _childs[0]->pos());

    int start = 0;
    int end = _childs.count() - 1;
    int pivot;
    while (end - start > 1) {
        pivot = (end + start) / 2;
        if (_childs[pivot]->pos() > proxyPos)
            end = pivot;
        else
            start = pivot;
    }

    if (_childs[end]->pos() <= proxyPos)
        return _childs[end];
    else
        return _childs[start];
}

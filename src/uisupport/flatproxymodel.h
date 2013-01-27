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

#ifndef FLATPROXYMODEL_H
#define FLATPROXYMODEL_H

#include <QAbstractProxyModel>

class FlatProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    FlatProxyModel(QObject *parent = 0);

    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    virtual QItemSelection mapSelectionFromSource(const QItemSelection &sourceSelection) const;
    virtual QItemSelection mapSelectionToSource(const QItemSelection &proxySelection) const;

    virtual void setSourceModel(QAbstractItemModel *sourceModel);

    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &index) const;

    virtual int rowCount(const QModelIndex &index) const;
    virtual int columnCount(const QModelIndex &index) const;

public slots:
    void linkTest() const;
    void completenessTest() const;

private slots:
    void on_columnsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void on_columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void on_columnsInserted(const QModelIndex &parent, int start, int end);
    void on_columnsRemoved(const QModelIndex &parent, int start, int end);

    void on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
//   void on_headerDataChanged(Qt::Orientation orientation, int first, int last);

    void on_layoutAboutToBeChanged();
    void on_layoutChanged();

    inline void on_modelAboutToBeReset() { reset(); }
    // void on_modelReset();

    void on_rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void on_rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void on_rowsInserted(const QModelIndex &parent, int start, int end);
    void on_rowsRemoved(const QModelIndex &parent, int start, int end);

private:
    QList<int> _childCount;

    class SourceItem;
    SourceItem *_rootSourceItem;

    void insertSubTree(const QModelIndex &source_idx, bool emitInsert = true);
    SourceItem *insertSubTreeHelper(SourceItem *parentItem, SourceItem *lastItem_, const QModelIndex &source_idx);

    void removeSubTree(const QModelIndex &source_idx, bool emitRemove = true);

    SourceItem *sourceToInternal(const QModelIndex &sourceIndex) const;

    void checkChildCount(const QModelIndex &index, const SourceItem *item, int &pos) const;

    class _RangeRect
    {
public:
        int left, right, top, bottom;
        SourceItem *topItem, *bottomItem;
        bool operator<(const _RangeRect &other) const;
    };
};


class FlatProxyModel::SourceItem
{
public:
    SourceItem(int row = 0, SourceItem *parent = 0);
    ~SourceItem();

    inline SourceItem *parent() const { return _parent; }
    inline SourceItem *child(int i) const { return _childs[i]; }
    inline int childCount() const { return _childs.count(); }

    inline int pos() const { return _pos; }
    inline SourceItem *next() const { return _next; }

    int sourceRow() const;
    SourceItem *findChild(int proxyPos) const;

private:
    inline void removeChild(SourceItem *item) { _childs.removeAt(_childs.indexOf(item)); }
    inline void setPos(int i) { _pos = i; }
    inline void setNext(SourceItem *next) { _next = next; }

    SourceItem *_parent;
    QList<SourceItem *> _childs;
    int _pos;
    SourceItem *_next;

    friend class FlatProxyModel;
};


#endif //FLATPROXYMODEL_H

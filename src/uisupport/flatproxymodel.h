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

#pragma once

#include "uisupport-export.h"

#include <QAbstractProxyModel>

class UISUPPORT_EXPORT FlatProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    FlatProxyModel(QObject* parent = nullptr);

    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

    QItemSelection mapSelectionFromSource(const QItemSelection& sourceSelection) const override;
    QItemSelection mapSelectionToSource(const QItemSelection& proxySelection) const override;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;

public slots:
    void linkTest() const;
    void completenessTest() const;

private slots:
    void on_columnsAboutToBeInserted(const QModelIndex& parent, int start, int end);
    void on_columnsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void on_columnsInserted(const QModelIndex& parent, int start, int end);
    void on_columnsRemoved(const QModelIndex& parent, int start, int end);

    void on_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    //   void on_headerDataChanged(Qt::Orientation orientation, int first, int last);

    void on_layoutAboutToBeChanged();
    void on_layoutChanged();

    inline void on_modelAboutToBeReset()
    {
        beginResetModel();
        endResetModel();
    }
    // void on_modelReset();

    void on_rowsAboutToBeInserted(const QModelIndex& parent, int start, int end);
    void on_rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void on_rowsInserted(const QModelIndex& parent, int start, int end);
    void on_rowsRemoved(const QModelIndex& parent, int start, int end);

private:
    QList<int> _childCount;

    class SourceItem;
    SourceItem* _rootSourceItem{nullptr};

    void insertSubTree(const QModelIndex& source_idx, bool emitInsert = true);
    SourceItem* insertSubTreeHelper(SourceItem* parentItem, SourceItem* lastItem_, const QModelIndex& source_idx);

    void removeSubTree(const QModelIndex& source_idx, bool emitRemove = true);

    SourceItem* sourceToInternal(const QModelIndex& sourceIndex) const;

    void checkChildCount(const QModelIndex& index, const SourceItem* item, int& pos) const;

    class _RangeRect
    {
    public:
        int left, right, top, bottom;
        SourceItem *topItem, *bottomItem;
        bool operator<(const _RangeRect& other) const;
    };
};

class FlatProxyModel::SourceItem
{
public:
    SourceItem(int row = 0, SourceItem* parent = nullptr);
    ~SourceItem();

    inline SourceItem* parent() const { return _parent; }
    inline SourceItem* child(int i) const { return _childs[i]; }
    inline int childCount() const { return _childs.count(); }

    inline int pos() const { return _pos; }
    inline SourceItem* next() const { return _next; }

    int sourceRow() const;
    SourceItem* findChild(int proxyPos) const;

private:
    inline void removeChild(SourceItem* item) { _childs.removeAt(_childs.indexOf(item)); }
    inline void setPos(int i) { _pos = i; }
    inline void setNext(SourceItem* next) { _next = next; }

    SourceItem* _parent;
    QList<SourceItem*> _childs;
    int _pos{-1};
    SourceItem* _next{nullptr};

    friend class FlatProxyModel;
};

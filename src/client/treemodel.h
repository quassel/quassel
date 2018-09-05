/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "client-export.h"

#include <QList>
#include <QStringList>
#include <QVariant>
#include <QAbstractItemModel>

#include <QLinkedList> // needed for debug

/*****************************************
 *  general item used in the Tree Model
 *****************************************/
class CLIENT_EXPORT AbstractTreeItem : public QObject
{
    Q_OBJECT

public:
    enum TreeItemFlag {
        NoTreeItemFlag = 0x00,
        DeleteOnLastChildRemoved = 0x01
    };
    Q_DECLARE_FLAGS(TreeItemFlags, TreeItemFlag)

    AbstractTreeItem(AbstractTreeItem *parent = nullptr);

    bool newChild(AbstractTreeItem *child);
    bool newChilds(const QList<AbstractTreeItem *> &items);

    bool removeChild(int row);
    inline bool removeChild(AbstractTreeItem *child) { return removeChild(child->row()); }
    void removeAllChilds();

    bool reParent(AbstractTreeItem *newParent);

    AbstractTreeItem *child(int row) const;

    int childCount(int column = 0) const;

    virtual int columnCount() const = 0;

    virtual QVariant data(int column, int role) const = 0;
    virtual bool setData(int column, const QVariant &value, int role) = 0;

    virtual inline Qt::ItemFlags flags() const { return _flags; }
    virtual inline void setFlags(Qt::ItemFlags flags) { _flags = flags; }

    inline AbstractTreeItem::TreeItemFlags treeItemFlags() const { return _treeItemFlags; }
    inline void setTreeItemFlags(AbstractTreeItem::TreeItemFlags flags) { _treeItemFlags = flags; }
    int row() const;
    inline AbstractTreeItem *parent() const { return qobject_cast<AbstractTreeItem *>(QObject::parent()); }

    void dumpChildList();

signals:
    void dataChanged(int column = -1);

    void beginAppendChilds(int firstRow, int lastRow);
    void endAppendChilds();

    void beginRemoveChilds(int firstRow, int lastRow);
    void endRemoveChilds();

protected:
    void customEvent(QEvent *event) override;

private:
    QList<AbstractTreeItem *> _childItems;
    Qt::ItemFlags _flags;
    TreeItemFlags _treeItemFlags;

    void removeChildLater(AbstractTreeItem *child);
    inline void checkForDeletion()
    {
        if (treeItemFlags() & DeleteOnLastChildRemoved && childCount() == 0) parent()->removeChildLater(this);
    }
};


/*****************************************
 * SimpleTreeItem
 *****************************************/
class CLIENT_EXPORT SimpleTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    SimpleTreeItem(const QList<QVariant> &data, AbstractTreeItem *parent = nullptr);
    ~SimpleTreeItem() override;

    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &value, int role) override;

    int columnCount() const override;

private:
    QList<QVariant> _itemData;
};


/*****************************************
 * PropertyMapItem
 *****************************************/
class CLIENT_EXPORT PropertyMapItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    PropertyMapItem(AbstractTreeItem *parent = nullptr);

    virtual QStringList propertyOrder() const = 0;

    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &value, int role) override;

    virtual QString toolTip(int column) const { Q_UNUSED(column) return QString(); }
    int columnCount() const override;
};


/*****************************************
 * TreeModel
 *****************************************/
class CLIENT_EXPORT TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum myRoles {
        SortRole = Qt::UserRole,
        UserRole
    };

    TreeModel(const QList<QVariant> &, QObject *parent = nullptr);
    ~TreeModel() override;

    AbstractTreeItem *root() const;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex indexByItem(AbstractTreeItem *item) const;

    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual void clear();

private slots:
    void itemDataChanged(int column = -1);

    void beginAppendChilds(int firstRow, int lastRow);
    void endAppendChilds();

    void beginRemoveChilds(int firstRow, int lastRow);
    void endRemoveChilds();

protected:
    AbstractTreeItem *rootItem;

private:
    void connectItem(AbstractTreeItem *item);

    struct ChildStatus {
        QModelIndex parent;
        int childCount;
        int start;
        int end;
        inline ChildStatus(QModelIndex parent_, int cc_, int s_, int e_) : parent(parent_), childCount(cc_), start(s_), end(e_) {};
    };
    ChildStatus _childStatus;
    int _aboutToRemoveOrInsert;

private slots:
    void debug_rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void debug_rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void debug_rowsInserted(const QModelIndex &parent, int start, int end);
    void debug_rowsRemoved(const QModelIndex &parent, int start, int end);
    void debug_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
};

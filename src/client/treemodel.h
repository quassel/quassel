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

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QList>
#include <QStringList>
#include <QVariant>
#include <QAbstractItemModel>

#include <QLinkedList> // needed for debug

/*****************************************
 *  general item used in the Tree Model
 *****************************************/
class AbstractTreeItem : public QObject
{
    Q_OBJECT

public:
    enum TreeItemFlag {
        NoTreeItemFlag = 0x00,
        DeleteOnLastChildRemoved = 0x01
    };
    Q_DECLARE_FLAGS(TreeItemFlags, TreeItemFlag)

    AbstractTreeItem(AbstractTreeItem *parent = 0);

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
    void customEvent(QEvent *event);

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
class SimpleTreeItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    SimpleTreeItem(const QList<QVariant> &data, AbstractTreeItem *parent = 0);
    virtual ~SimpleTreeItem();

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role);

    virtual int columnCount() const;

private:
    QList<QVariant> _itemData;
};


/*****************************************
 * PropertyMapItem
 *****************************************/
class PropertyMapItem : public AbstractTreeItem
{
    Q_OBJECT

public:
    PropertyMapItem(const QStringList &propertyOrder, AbstractTreeItem *parent = 0);
    PropertyMapItem(AbstractTreeItem *parent = 0);

    virtual ~PropertyMapItem();

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role);

    virtual QString toolTip(int column) const { Q_UNUSED(column) return QString(); }
    virtual int columnCount() const;

    void appendProperty(const QString &property);

private:
    QStringList _propertyOrder;
};


/*****************************************
 * TreeModel
 *****************************************/
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum myRoles {
        SortRole = Qt::UserRole,
        UserRole
    };

    TreeModel(const QList<QVariant> &, QObject *parent = 0);
    virtual ~TreeModel();

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex indexByItem(AbstractTreeItem *item) const;

    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

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


#endif

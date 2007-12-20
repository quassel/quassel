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

#ifndef _TREEMODEL_H_
#define _TREEMODEL_H_

#include <QList>
#include <QStringList>
#include <QVariant>
#include <QHash>
#include <QAbstractItemModel>

/*****************************************
 *  general item used in the Tree Model
 *****************************************/
class AbstractTreeItem : public QObject {
  Q_OBJECT
  Q_PROPERTY(uint id READ id)

public:
  AbstractTreeItem(AbstractTreeItem *parent = 0);
  virtual ~AbstractTreeItem();

  void appendChild(int column, AbstractTreeItem *child);
  void appendChild(AbstractTreeItem *child);
  
  void removeChild(int column, int row);
  void removeChild(int row);

  virtual quint64 id() const;

  AbstractTreeItem *child(int column, int row) const;
  AbstractTreeItem *child(int row) const;
  
  AbstractTreeItem *childById(int column, const uint &id) const;
  AbstractTreeItem *childById(const uint &id) const;

  int childCount(int column) const;
  int childCount() const;

  virtual int columnCount() const = 0;

  virtual QVariant data(int column, int role) const = 0;

  virtual Qt::ItemFlags flags() const;
  virtual void setFlags(Qt::ItemFlags);

  int column() const;
  int row() const;
  AbstractTreeItem *parent();

signals:
  void dataChanged(int column);
				       
private slots:
  void childDestroyed();

private:
  QHash<int, QList<AbstractTreeItem *> > _childItems;
  QHash<int, QHash<quint64, AbstractTreeItem *> > _childHash; // uint to be compatible to qHash functions
  AbstractTreeItem *_parentItem;
  Qt::ItemFlags _flags;

  int defaultColumn() const;
};


/*****************************************
 * SimpleTreeItem
 *****************************************/
class SimpleTreeItem : public AbstractTreeItem {
  Q_OBJECT

public:
  SimpleTreeItem(const QList<QVariant> &data, AbstractTreeItem *parent = 0);
  virtual ~SimpleTreeItem();
  virtual QVariant data(int column, int role) const;
  virtual int columnCount() const;

private:
  QList<QVariant> _itemData;
};

/*****************************************
 * PropertyMapItem
 *****************************************/
class PropertyMapItem : public AbstractTreeItem {
  Q_OBJECT

public:
  PropertyMapItem(const QStringList &propertyOrder, AbstractTreeItem *parent = 0);
  PropertyMapItem(AbstractTreeItem *parent = 0);

  virtual ~PropertyMapItem();
  
  virtual QVariant data(int column, int role) const;
  virtual int columnCount() const;
  
  void appendProperty(const QString &property);

private:
  QStringList _propertyOrder;
};


/*****************************************
 * TreeModel
 *****************************************/
class TreeModel : public QAbstractItemModel {
  Q_OBJECT

public:
  TreeModel(const QList<QVariant> &, QObject *parent = 0);
  virtual ~TreeModel();

  QVariant data(const QModelIndex &index, int role) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex indexById(uint id, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex indexByItem(AbstractTreeItem *item) const;

  QModelIndex parent(const QModelIndex &index) const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual void clear();

private slots:
  void itemDataChanged(int column);

protected:
  void appendChild(AbstractTreeItem *parent, AbstractTreeItem *child);

  bool removeRow(int row, const QModelIndex &parent = QModelIndex());
  bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
  
  AbstractTreeItem *rootItem;
};

#endif

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
#include <QVariant>
#include <QHash>
#include <QAbstractItemModel>

/*****************************************
 *  general item used in the Tree Model
 *****************************************/
class TreeItem : public QObject {
  Q_OBJECT
  Q_PROPERTY(uint id READ id)

public:
  TreeItem(const QList<QVariant> &data, TreeItem *parent = 0);
  TreeItem(TreeItem *parent = 0);
  virtual ~TreeItem();

  void appendChild(TreeItem *child);
  void removeChild(int row);

  virtual uint id() const;

  TreeItem *child(int row) const;
  TreeItem *childById(const uint &) const;

  int childCount() const;
  int columnCount() const;

  virtual QVariant data(int column, int role) const;

  Qt::ItemFlags flags() const;
  void setFlags(Qt::ItemFlags);

  int row() const;
  TreeItem *parent();


private slots:
  void childDestroyed();


protected:
  QList<QVariant> itemData;

private:
  QList<TreeItem *> _childItems;
  QHash<uint, TreeItem *> _childHash; // uint to be compatible to qHash functions
  TreeItem *_parentItem;
  Qt::ItemFlags _flags;
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
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

  virtual void clear();

protected:
  bool removeRow(int row, const QModelIndex &parent = QModelIndex());
  bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
  
  TreeItem *rootItem;
};

#endif

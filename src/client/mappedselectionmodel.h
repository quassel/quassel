/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef _MAPPEDSELECTIONMODEL_H_
#define _MAPPEDSELECTIONMODEL_H_

#include <QObject>
#include <QModelIndex>
#include <QItemSelection>
#include <QItemSelectionModel>

class QAbstractProxyModel;

class MappedSelectionModel : public QItemSelectionModel {
  Q_OBJECT

public:
  MappedSelectionModel(QAbstractItemModel *model = 0);
  virtual ~MappedSelectionModel();

  inline bool isProxyModel() const { return _isProxyModel; }

  const QAbstractItemModel *baseModel() const;
  const QAbstractProxyModel *proxyModel() const;
  
  QModelIndex mapFromSource(const QModelIndex &sourceIndex);
  QItemSelection mapFromSource(const QItemSelection &sourceSelection);
				    
  QModelIndex mapToSource(const QModelIndex &proxyIndex);
  QItemSelection mapToSource(const QItemSelection &proxySelection);
									
public slots:
  void mappedSelect(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
  void mappedSelect(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command);
  void mappedSetCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);

private slots:
  void _currentChanged(const QModelIndex &current, const QModelIndex &previous);
  void _selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
  
signals:
  void mappedCurrentChanged(const QModelIndex &current);
  void mappedSelectionChanged(const QItemSelection &selected);

private:
  bool _isProxyModel;

};

#endif

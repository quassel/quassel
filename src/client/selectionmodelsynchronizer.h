/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _SELECTIONMODELSYNCHRONIZER_H_
#define _SELECTIONMODELSYNCHRONIZER_H_

#include <QObject>
#include <QItemSelectionModel>

class QAbstractItemModel;
class MappedSelectionModel;

class SelectionModelSynchronizer : public QObject {
  Q_OBJECT

public:
  SelectionModelSynchronizer(QAbstractItemModel *parent = 0);
  virtual ~SelectionModelSynchronizer();

  void addSelectionModel(MappedSelectionModel *model);
  void removeSelectionModel(MappedSelectionModel *model);

  inline QAbstractItemModel *model() { return _model; }

private slots:
  void _mappedCurrentChanged(const QModelIndex &current);
  void _mappedSelectionChanged(const QItemSelection &selected);

signals:
  void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command);
  void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
  
private:
  QAbstractItemModel *_model;
};

#endif

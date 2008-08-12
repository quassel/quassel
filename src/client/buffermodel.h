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

#ifndef BUFFERMODEL_H
#define BUFFERMODEL_H

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

#include "types.h"
#include "selectionmodelsynchronizer.h"

class NetworkModel;
class MappedSelectionModel;
class QAbstractItemView;

class BufferModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  BufferModel(NetworkModel *parent = 0);

  bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const;
  
  inline const SelectionModelSynchronizer *selectionModelSynchronizer() const { return &_selectionModelSynchronizer; }
  inline QItemSelectionModel *standardSelectionModel() const { return _selectionModelSynchronizer.selectionModel(); }
  
  void synchronizeSelectionModel(MappedSelectionModel *selectionModel);
  void synchronizeView(QAbstractItemView *view);

  inline QModelIndex currentIndex() { return standardSelectionModel()->currentIndex(); }
  void setCurrentIndex(const QModelIndex &newCurrent);

private slots:
  void debug_currentChanged(QModelIndex current, QModelIndex previous);
    
private:
  SelectionModelSynchronizer _selectionModelSynchronizer;
};

#endif // BUFFERMODEL_H

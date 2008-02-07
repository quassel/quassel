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

#include "mappedselectionmodel.h"

#include <QItemSelectionModel>
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QDebug>

MappedSelectionModel::MappedSelectionModel(QAbstractItemModel *model)
  : QItemSelectionModel(model)
{
  _isProxyModel = (bool)proxyModel();
  connect(this, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(_currentChanged(QModelIndex, QModelIndex)));
  connect(this, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	  this, SLOT(_selectionChanged(QItemSelection, QItemSelection)));
}

MappedSelectionModel::~MappedSelectionModel() {
}

const QAbstractItemModel *MappedSelectionModel::baseModel() const {
  if(isProxyModel())
    return proxyModel()->sourceModel();
  else
    return model();
}

const QAbstractProxyModel *MappedSelectionModel::proxyModel() const {
  return qobject_cast<const QAbstractProxyModel *>(model());
}

QModelIndex MappedSelectionModel::mapFromSource(const QModelIndex &sourceIndex) {
  if(isProxyModel())
    return proxyModel()->mapFromSource(sourceIndex);
  else
    return sourceIndex;
}

QItemSelection MappedSelectionModel::mapSelectionFromSource(const QItemSelection &sourceSelection) {
  if(isProxyModel())
    return proxyModel()->mapSelectionFromSource(sourceSelection);
  else
    return sourceSelection;
}
				    
QModelIndex MappedSelectionModel::mapToSource(const QModelIndex &proxyIndex) {
  if(isProxyModel())
    return proxyModel()->mapToSource(proxyIndex);
  else
    return proxyIndex;
}

QItemSelection MappedSelectionModel::mapSelectionToSource(const QItemSelection &proxySelection) {
  if(isProxyModel())
    return proxyModel()->mapSelectionToSource(proxySelection);
  else
    return proxySelection;
}
									
void MappedSelectionModel::mappedSelect(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  QModelIndex mappedIndex = mapFromSource(index);
  if(!isSelected(mappedIndex))
    select(mappedIndex, command);
}

void MappedSelectionModel::mappedSelect(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) {
  QItemSelection mappedSelection = mapSelectionFromSource(selection);
  if(mappedSelection != QItemSelectionModel::selection())
    select(mappedSelection, command);  
}

void MappedSelectionModel::mappedSetCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  QModelIndex mappedIndex = mapFromSource(index);
  if(mappedIndex == currentIndex())
    return;
  setCurrentIndex(mappedIndex, command);
}


void MappedSelectionModel::_currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous)
  if(current.isValid())
    emit mappedCurrentChanged(mapToSource(current));
}

void MappedSelectionModel::_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(selected)
  Q_UNUSED(deselected)
  emit mappedSelectionChanged(mapSelectionToSource(QItemSelectionModel::selection()));
}


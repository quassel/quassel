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

#include "buffermodel.h"

#include "networkmodel.h"
#include "mappedselectionmodel.h"
#include "buffer.h"
#include <QAbstractItemView>

BufferModel::BufferModel(NetworkModel *parent)
  : QSortFilterProxyModel(parent),
    _selectionModelSynchronizer(new SelectionModelSynchronizer(this)),
    _propertyMapper(new ModelPropertyMapper(this))
{
  setSourceModel(parent);

  // initialize the Property Mapper
  _propertyMapper->setModel(this);
  delete _propertyMapper->selectionModel();
  MappedSelectionModel *mappedSelectionModel = new MappedSelectionModel(this);
  _propertyMapper->setSelectionModel(mappedSelectionModel);
  synchronizeSelectionModel(mappedSelectionModel);
  
  connect(_selectionModelSynchronizer, SIGNAL(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)),
	  this, SLOT(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)));
}

BufferModel::~BufferModel() {
}

bool BufferModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const {
  Q_UNUSED(sourceRow)
    
  if(parent.data(NetworkModel::ItemTypeRole) == NetworkModel::BufferItemType)
    return false;
  else
    return true;
}

void BufferModel::synchronizeSelectionModel(MappedSelectionModel *selectionModel) {
  selectionModelSynchronizer()->addSelectionModel(selectionModel);
}

void BufferModel::synchronizeView(QAbstractItemView *view) {
  MappedSelectionModel *mappedSelectionModel = new MappedSelectionModel(view->model());
  selectionModelSynchronizer()->addSelectionModel(mappedSelectionModel);
  Q_ASSERT(mappedSelectionModel);
  delete view->selectionModel();
  view->setSelectionModel(mappedSelectionModel);
}

void BufferModel::mapProperty(int column, int role, QObject *target, const QByteArray &property) {
  propertyMapper()->addMapping(column, role, target, property);
}

// This Slot indicates that the user has selected a different buffer in the gui
void BufferModel::setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  Q_UNUSED(command)

  BufferId newCurrentBuffer;
  if(index.data(NetworkModel::ItemTypeRole) == NetworkModel::BufferItemType && currentBuffer != (newCurrentBuffer = index.data(NetworkModel::BufferIdRole).toUInt())) {
    currentBuffer = newCurrentBuffer;
    // FIXME: to something like: index.setData(ActivitRole, NoActivity);
    // networkModel->bufferActivity(BufferItem::NoActivity, currentBuffer);
    emit selectionChanged(index);
  }
}

QModelIndex BufferModel::currentIndex() {
  return propertyMapper()->selectionModel()->currentIndex();
}

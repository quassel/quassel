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
    _selectionModelSynchronizer(this),
    _propertyMapper(this)
{
  setSourceModel(parent);

  // initialize the Property Mapper
  _propertyMapper.setModel(this);
  _selectionModelSynchronizer.addRegularSelectionModel(_propertyMapper.selectionModel());
  connect(_propertyMapper.selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(currentChanged(QModelIndex, QModelIndex)));
}

BufferModel::~BufferModel() {
}

bool BufferModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const {
  Q_UNUSED(sourceRow);
  // only networks and buffers are allowed
  if(!parent.isValid())
    return true;
  if(parent.data(NetworkModel::ItemTypeRole) == NetworkModel::NetworkItemType)
    return true;

  return false;
}

void BufferModel::synchronizeSelectionModel(MappedSelectionModel *selectionModel) {
  _selectionModelSynchronizer.addSelectionModel(selectionModel);
}

void BufferModel::synchronizeView(QAbstractItemView *view) {
  MappedSelectionModel *mappedSelectionModel = new MappedSelectionModel(view->model());
  _selectionModelSynchronizer.addSelectionModel(mappedSelectionModel);
  Q_ASSERT(mappedSelectionModel);
  delete view->selectionModel();
  view->setSelectionModel(mappedSelectionModel);
}

void BufferModel::mapProperty(int column, int role, QObject *target, const QByteArray &property) {
  _propertyMapper.addMapping(column, role, target, property);
}

QModelIndex BufferModel::currentIndex() {
  return propertyMapper()->selectionModel()->currentIndex();
}

void BufferModel::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(current);
  setData(current, QDateTime::currentDateTime(), NetworkModel::LastSeenRole);
  setData(previous, QDateTime::currentDateTime(), NetworkModel::LastSeenRole);
  setData(previous, qVariantFromValue((int)BufferItem::NoActivity), NetworkModel::BufferActivityRole);
}

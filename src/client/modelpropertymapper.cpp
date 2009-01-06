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

#include "modelpropertymapper.h"

#include <QItemSelectionModel>
#include <QDebug>

ModelPropertyMapper::ModelPropertyMapper(QObject *parent)
  : QObject(parent),
    _model(0),
    _selectionModel(0)
{
}

ModelPropertyMapper::~ModelPropertyMapper() {
}

void ModelPropertyMapper::setModel(QAbstractItemModel *model) {
  if(_model) {
    disconnect(_model, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	       this, SLOT(dataChanged(QModelIndex, QModelIndex)));
  }
  _model = model;
  connect(_model, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	  this, SLOT(dataChanged(QModelIndex, QModelIndex)));
  setSelectionModel(new QItemSelectionModel(model));
}

QAbstractItemModel *ModelPropertyMapper::model() const {
  return _model;
}

void ModelPropertyMapper::setSelectionModel(QItemSelectionModel *selectionModel) {
  if(selectionModel->model() != model()) {
    qWarning() << "cannot set itemSelectionModel" << selectionModel << "which uses different basemodel than" << model();
    return;
  }
  if(_selectionModel)
    disconnect(_selectionModel, 0, this, 0);
  _selectionModel = selectionModel;
  connect(_selectionModel, SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
	  this, SLOT(setCurrentRow(QModelIndex, QModelIndex)));
  connect(_selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(setCurrentIndex(QModelIndex, QModelIndex)));
  
  setCurrentRow(selectionModel->currentIndex(), QModelIndex());
}

QItemSelectionModel *ModelPropertyMapper::selectionModel() const {
  return _selectionModel;
}

void ModelPropertyMapper::addMapping(int column, int role, QObject *target, const QByteArray &property) {
  Mapping mapping(column, role, target, property);
  if(!_mappings.contains(mapping))
    _mappings.append(mapping);
}

void ModelPropertyMapper::removeMapping(int column, int role, QObject *target, const QByteArray &property) {
  if(column == 0 && role == 0 && target == 0 && !property.isNull()) {
    _mappings.clear();
    return;
  }
  
  if(column == 0 && role == 0 && !property.isNull()) {
    QList<Mapping>::iterator iter = _mappings.begin();
    while(iter != _mappings.end()) {
      if((*iter).target == target)
	iter = _mappings.erase(iter);
      else
	iter++;
    }
    return;
  }
  _mappings.removeAll(Mapping(column, role, target, property));
}

void ModelPropertyMapper::setCurrentIndex(const QModelIndex &current, const QModelIndex &previous) {
  if(current.row() == previous.row() && current.parent() != previous.parent())
    setCurrentRow(current, previous);
}

void ModelPropertyMapper::setCurrentRow(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous)
  foreach(Mapping mapping, _mappings) {
    QModelIndex index = current.sibling(current.row(), mapping.column);
    mapping.target->setProperty(mapping.property, index.data(mapping.role));
  }
}

void ModelPropertyMapper::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QItemSelectionRange changedRange(topLeft, bottomRight);
  foreach(Mapping mapping, _mappings) {
    QModelIndex index = _selectionModel->currentIndex().sibling(_selectionModel->currentIndex().row(), mapping.column);
    if(changedRange.contains(index)) {
      mapping.target->setProperty(mapping.property, index.data(mapping.role));
    }
  }
}

void ModelPropertyMapper::targetDestroyed() {
  removeMapping(0, 0, sender(), QByteArray());
}

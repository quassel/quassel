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

#include "selectionmodelsynchronizer.h"


#include <QAbstractItemModel>
#include "mappedselectionmodel.h"

#include <QDebug>

SelectionModelSynchronizer::SelectionModelSynchronizer(QAbstractItemModel *parent)
  : QObject(parent),
    _model(parent)
{
}

SelectionModelSynchronizer::~SelectionModelSynchronizer() {
}

void SelectionModelSynchronizer::addSelectionModel(MappedSelectionModel *selectionmodel) {
  if(selectionmodel->model() == model()) {
    addRegularSelectionModel(selectionmodel);
    return;
  }
  
  if(selectionmodel->baseModel() != model()) {
    qWarning() << "cannot Syncronize SelectionModel" << selectionmodel << "which has a different baseModel()";
    return;
  }

  connect(selectionmodel, SIGNAL(mappedCurrentChanged(QModelIndex)),
	  this, SLOT(_mappedCurrentChanged(QModelIndex)));
  connect(selectionmodel, SIGNAL(mappedSelectionChanged(QItemSelection)),
	  this, SLOT(_mappedSelectionChanged(QItemSelection)));
  
  connect(this, SIGNAL(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)),
	  selectionmodel, SLOT(mappedSetCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)));
  connect(this, SIGNAL(select(QItemSelection, QItemSelectionModel::SelectionFlags)),
 	  selectionmodel, SLOT(mappedSelect(QItemSelection, QItemSelectionModel::SelectionFlags)));
}

void SelectionModelSynchronizer::addRegularSelectionModel(QItemSelectionModel *selectionmodel) {
  if(selectionmodel->model() != model()) {
    qWarning() << "cannot Syncronize QItemSelectionModel" << selectionmodel << "which has a different model()";    
    return;
  }
  connect(selectionmodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(_regularCurrentChanged(QModelIndex, QModelIndex)));
  connect(selectionmodel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	  this, SLOT(_regularSelectionChanged(QItemSelection, QItemSelection)));
  
  connect(this, SIGNAL(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)),
	  selectionmodel, SLOT(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)));
  connect(this, SIGNAL(select(QItemSelection, QItemSelectionModel::SelectionFlags)),
 	  selectionmodel, SLOT(select(QItemSelection, QItemSelectionModel::SelectionFlags)));
  
}

void SelectionModelSynchronizer::removeSelectionModel(MappedSelectionModel *model) {
  disconnect(model, 0, this, 0);
  disconnect(this, 0, model, 0);
}

void SelectionModelSynchronizer::_mappedCurrentChanged(const QModelIndex &current) {
  emit setCurrentIndex(current, QItemSelectionModel::ClearAndSelect);
}

void SelectionModelSynchronizer::_mappedSelectionChanged(const QItemSelection &selected) {
  emit select(selected, QItemSelectionModel::ClearAndSelect);
}

void SelectionModelSynchronizer::_regularCurrentChanged(const QModelIndex &newCurrent, const QModelIndex &oldCurrent) {
  Q_UNUSED(oldCurrent)
  emit setCurrentIndex(newCurrent, QItemSelectionModel::ClearAndSelect);
}

void SelectionModelSynchronizer::_regularSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(selected)
  Q_UNUSED(deselected)
  QItemSelectionModel *selectionModel = qobject_cast<QItemSelectionModel *>(sender());
  emit select(selectionModel->selection(), QItemSelectionModel::ClearAndSelect);
}

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
#include <QAbstractProxyModel>

#include <QDebug>

SelectionModelSynchronizer::SelectionModelSynchronizer(QAbstractItemModel *parent)
  : QObject(parent),
    _model(parent),
    _selectionModel(parent)
{
  connect(&_selectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
	  this, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
  connect(&_selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	  this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));
}

bool SelectionModelSynchronizer::checkBaseModel(QItemSelectionModel *selectionModel) {
  if(!selectionModel)
    return false;

  const QAbstractItemModel *baseModel = selectionModel->model();
  const QAbstractProxyModel *proxyModel = 0;
  while((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
    baseModel = proxyModel->sourceModel();
    if(baseModel == model())
      break;
  }
  return baseModel == model();
}

void SelectionModelSynchronizer::addSelectionModel(QItemSelectionModel *selectionModel) {
  if(!checkBaseModel(selectionModel)) {
    qWarning() << "cannot Syncronize SelectionModel" << selectionModel << "which has a different baseModel()";
    return;
  }

  connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
	  this, SLOT(mappedCurrentChanged(QModelIndex, QModelIndex)));
  connect(selectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	  this, SLOT(mappedSelectionChanged(QItemSelection, QItemSelection)));

  if(qobject_cast<MappedSelectionModel *>(selectionModel)) {
    connect(this, SIGNAL(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)),
	    selectionModel, SLOT(mappedSetCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)));
    connect(this, SIGNAL(select(QItemSelection, QItemSelectionModel::SelectionFlags)),
	    selectionModel, SLOT(mappedSelect(QItemSelection, QItemSelectionModel::SelectionFlags)));
  } else {
    connect(this, SIGNAL(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)),
	    selectionModel, SLOT(setCurrentIndex(QModelIndex, QItemSelectionModel::SelectionFlags)));
    connect(this, SIGNAL(select(QItemSelection, QItemSelectionModel::SelectionFlags)),
	    selectionModel, SLOT(select(QItemSelection, QItemSelectionModel::SelectionFlags)));
  }
}

void SelectionModelSynchronizer::removeSelectionModel(QItemSelectionModel *model) {
  disconnect(model, 0, this, 0);
  disconnect(this, 0, model, 0);
}

void SelectionModelSynchronizer::mappedCurrentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous);
  QItemSelectionModel *selectionModel = qobject_cast<QItemSelectionModel *>(sender());
  Q_ASSERT(selectionModel);
  QModelIndex newSourceCurrent = mapToSource(current, selectionModel);
  if(newSourceCurrent.isValid() && newSourceCurrent != currentIndex())
    setCurrentIndex(newSourceCurrent);
}

void SelectionModelSynchronizer::mappedSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  QItemSelectionModel *selectionModel = qobject_cast<QItemSelectionModel *>(sender());
  Q_ASSERT(selectionModel);
  QItemSelection newSourceSelection = mapSelectionToSource(selectionModel->selection(), selectionModel);
  QItemSelection currentContainsSelection = newSourceSelection;
  currentContainsSelection.merge(currentSelection(), QItemSelectionModel::Deselect);
  if(!currentContainsSelection.isEmpty())
    setCurrentSelection(newSourceSelection);
}

QModelIndex SelectionModelSynchronizer::mapToSource(const QModelIndex &index, QItemSelectionModel *selectionModel) {
  Q_ASSERT(selectionModel);

  QModelIndex sourceIndex = index;
  const QAbstractItemModel *baseModel = selectionModel->model();
  const QAbstractProxyModel *proxyModel = 0;
  while((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
    sourceIndex = proxyModel->mapToSource(sourceIndex);
    baseModel = proxyModel->sourceModel();
    if(baseModel == model())
      break;
  }
  return sourceIndex;
}

QItemSelection SelectionModelSynchronizer::mapSelectionToSource(const QItemSelection &selection, QItemSelectionModel *selectionModel) {
  Q_ASSERT(selectionModel);

  QItemSelection sourceSelection = selection;
  const QAbstractItemModel *baseModel = selectionModel->model();
  const QAbstractProxyModel *proxyModel = 0;
  while((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
    sourceSelection = proxyModel->mapSelectionToSource(sourceSelection);
    baseModel = proxyModel->sourceModel();
    if(baseModel == model())
      break;
  }
  return sourceSelection;
}

void SelectionModelSynchronizer::setCurrentIndex(const QModelIndex &index) {
  _selectionModel.setCurrentIndex(index, QItemSelectionModel::Current);
}
void SelectionModelSynchronizer::setCurrentSelection(const QItemSelection &selection) {
  _selectionModel.select(selection, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
}

void SelectionModelSynchronizer::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  Q_UNUSED(previous);
  emit setCurrentIndex(current, QItemSelectionModel::Current);
}

void SelectionModelSynchronizer::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  emit select(_selectionModel.selection(), QItemSelectionModel::ClearAndSelect);
}

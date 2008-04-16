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
#include <QLinkedList>
#include <QDebug>

MappedSelectionModel::MappedSelectionModel(QAbstractItemModel *model)
  : QItemSelectionModel(model)
{
}

QModelIndex MappedSelectionModel::mapFromSource(const QModelIndex &sourceIndex) {
  QModelIndex proxyIndex = sourceIndex;
  QLinkedList<const QAbstractProxyModel *> proxies;
  const QAbstractItemModel *baseModel = model();
  const QAbstractProxyModel *proxyModel = 0;
  while((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
    proxies.push_back(proxyModel);
    baseModel = proxyModel->sourceModel();
    if(baseModel == sourceIndex.model())
      break;
  }

  while(!proxies.isEmpty()) {
    proxyModel = proxies.takeLast();
    proxyIndex = proxyModel->mapFromSource(proxyIndex);
  }
  return proxyIndex;
}

QItemSelection MappedSelectionModel::mapSelectionFromSource(const QItemSelection &sourceSelection) {
  if(sourceSelection.isEmpty())
    return sourceSelection;

  QItemSelection proxySelection = sourceSelection;
  QLinkedList<const QAbstractProxyModel *> proxies;
  const QAbstractItemModel *baseModel = model();
  const QAbstractProxyModel *proxyModel = 0;
  while((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
    proxies.push_back(proxyModel);
    baseModel = proxyModel->sourceModel();
    if(baseModel == sourceSelection.first().model())
      break;
  }

  while(!proxies.isEmpty()) {
    proxyModel = proxies.takeLast();
    proxySelection = proxyModel->mapSelectionFromSource(proxySelection);
  }
  return proxySelection;
}
				    
void MappedSelectionModel::mappedSetCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
  setCurrentIndex(mapFromSource(index), command);
}

void MappedSelectionModel::mappedSelect(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) {
  select(mapSelectionFromSource(selection), command);
}


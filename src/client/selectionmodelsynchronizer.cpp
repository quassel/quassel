/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "selectionmodelsynchronizer.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>

#include <QDebug>

SelectionModelSynchronizer::SelectionModelSynchronizer(QAbstractItemModel *parent)
    : QObject(parent),
    _model(parent),
    _selectionModel(parent),
    _changeCurrentEnabled(true),
    _changeSelectionEnabled(true)
{
    connect(&_selectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));
    connect(&_selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));
}


bool SelectionModelSynchronizer::checkBaseModel(QItemSelectionModel *selectionModel)
{
    if (!selectionModel)
        return false;

    const QAbstractItemModel *baseModel = selectionModel->model();
    const QAbstractProxyModel *proxyModel = 0;
    while ((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
        baseModel = proxyModel->sourceModel();
        if (baseModel == model())
            break;
    }
    return baseModel == model();
}


void SelectionModelSynchronizer::synchronizeSelectionModel(QItemSelectionModel *selectionModel)
{
    if (!checkBaseModel(selectionModel)) {
        qWarning() << "cannot Synchronize SelectionModel" << selectionModel << "which has a different baseModel()";
        return;
    }

    if (_selectionModels.contains(selectionModel)) {
        selectionModel->setCurrentIndex(mapFromSource(currentIndex(), selectionModel), QItemSelectionModel::Current);
        selectionModel->select(mapSelectionFromSource(currentSelection(), selectionModel), QItemSelectionModel::ClearAndSelect);
        return;
    }

    connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
        this, SLOT(syncedCurrentChanged(QModelIndex, QModelIndex)));
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
        this, SLOT(syncedSelectionChanged(QItemSelection, QItemSelection)));

    connect(selectionModel, SIGNAL(destroyed(QObject *)), this, SLOT(selectionModelDestroyed(QObject *)));

    _selectionModels << selectionModel;
}


void SelectionModelSynchronizer::removeSelectionModel(QItemSelectionModel *model)
{
    disconnect(model, 0, this, 0);
    disconnect(this, 0, model, 0);
    selectionModelDestroyed(model);
}


void SelectionModelSynchronizer::selectionModelDestroyed(QObject *object)
{
    QItemSelectionModel *model = static_cast<QItemSelectionModel *>(object);
    QSet<QItemSelectionModel *>::iterator iter = _selectionModels.begin();
    while (iter != _selectionModels.end()) {
        if (*iter == model) {
            iter = _selectionModels.erase(iter);
        }
        else {
            iter++;
        }
    }
}


void SelectionModelSynchronizer::syncedCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    if (!_changeCurrentEnabled)
        return;

    QItemSelectionModel *selectionModel = qobject_cast<QItemSelectionModel *>(sender());
    Q_ASSERT(selectionModel);
    QModelIndex newSourceCurrent = mapToSource(current, selectionModel);
    if (newSourceCurrent.isValid() && newSourceCurrent != currentIndex())
        setCurrentIndex(newSourceCurrent);
}


void SelectionModelSynchronizer::syncedSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    if (!_changeSelectionEnabled)
        return;

    QItemSelectionModel *selectionModel = qobject_cast<QItemSelectionModel *>(sender());
    Q_ASSERT(selectionModel);

    QItemSelection mappedSelection = selectionModel->selection();
    QItemSelection currentSelectionMapped = mapSelectionFromSource(currentSelection(), selectionModel);

    QItemSelection checkSelection = currentSelectionMapped;
    checkSelection.merge(mappedSelection, QItemSelectionModel::Deselect);
    if (checkSelection.isEmpty()) {
        // that means the new selection contains the current selection (currentSel - newSel = {})
        checkSelection = mappedSelection;
        checkSelection.merge(currentSelectionMapped, QItemSelectionModel::Deselect);
        if (checkSelection.isEmpty()) {
            // that means the current selection contains the new selection (newSel - currentSel = {})
            // -> currentSel == newSel
            return;
        }
    }
    setCurrentSelection(mapSelectionToSource(mappedSelection, selectionModel));
}


QModelIndex SelectionModelSynchronizer::mapFromSource(const QModelIndex &sourceIndex, const QItemSelectionModel *selectionModel)
{
    Q_ASSERT(selectionModel);

    QModelIndex mappedIndex = sourceIndex;

    // make a list of all involved proxies, wie have to traverse backwards
    QList<const QAbstractProxyModel *> proxyModels;
    const QAbstractItemModel *baseModel = selectionModel->model();
    const QAbstractProxyModel *proxyModel = 0;
    while ((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
        if (baseModel == model())
            break;
        proxyModels << proxyModel;
        baseModel = proxyModel->sourceModel();
    }

    // now traverse it;
    for (int i = proxyModels.count() - 1; i >= 0; i--) {
        mappedIndex = proxyModels[i]->mapFromSource(mappedIndex);
    }

    return mappedIndex;
}


QItemSelection SelectionModelSynchronizer::mapSelectionFromSource(const QItemSelection &sourceSelection, const QItemSelectionModel *selectionModel)
{
    Q_ASSERT(selectionModel);

    QItemSelection mappedSelection = sourceSelection;

    // make a list of all involved proxies, wie have to traverse backwards
    QList<const QAbstractProxyModel *> proxyModels;
    const QAbstractItemModel *baseModel = selectionModel->model();
    const QAbstractProxyModel *proxyModel = 0;
    while ((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
        if (baseModel == model())
            break;
        proxyModels << proxyModel;
        baseModel = proxyModel->sourceModel();
    }

    // now traverse it;
    for (int i = proxyModels.count() - 1; i >= 0; i--) {
        mappedSelection = proxyModels[i]->mapSelectionFromSource(mappedSelection);
    }
    return mappedSelection;
}


QModelIndex SelectionModelSynchronizer::mapToSource(const QModelIndex &index, QItemSelectionModel *selectionModel)
{
    Q_ASSERT(selectionModel);

    QModelIndex sourceIndex = index;
    const QAbstractItemModel *baseModel = selectionModel->model();
    const QAbstractProxyModel *proxyModel = 0;
    while ((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
        sourceIndex = proxyModel->mapToSource(sourceIndex);
        baseModel = proxyModel->sourceModel();
        if (baseModel == model())
            break;
    }
    return sourceIndex;
}


QItemSelection SelectionModelSynchronizer::mapSelectionToSource(const QItemSelection &selection, QItemSelectionModel *selectionModel)
{
    Q_ASSERT(selectionModel);

    QItemSelection sourceSelection = selection;
    const QAbstractItemModel *baseModel = selectionModel->model();
    const QAbstractProxyModel *proxyModel = 0;
    while ((proxyModel = qobject_cast<const QAbstractProxyModel *>(baseModel)) != 0) {
        sourceSelection = proxyModel->mapSelectionToSource(sourceSelection);
        baseModel = proxyModel->sourceModel();
        if (baseModel == model())
            break;
    }
    return sourceSelection;
}


void SelectionModelSynchronizer::setCurrentIndex(const QModelIndex &index)
{
    _selectionModel.setCurrentIndex(index, QItemSelectionModel::Current);
}


void SelectionModelSynchronizer::setCurrentSelection(const QItemSelection &selection)
{
    _selectionModel.select(selection, QItemSelectionModel::ClearAndSelect);
}


void SelectionModelSynchronizer::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    _changeCurrentEnabled = false;
    QSet<QItemSelectionModel *>::iterator iter = _selectionModels.begin();
    while (iter != _selectionModels.end()) {
        (*iter)->setCurrentIndex(mapFromSource(current, (*iter)), QItemSelectionModel::Current);
        iter++;
    }
    _changeCurrentEnabled = true;
}


void SelectionModelSynchronizer::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    _changeSelectionEnabled = false;
    QSet<QItemSelectionModel *>::iterator iter = _selectionModels.begin();
    while (iter != _selectionModels.end()) {
        (*iter)->select(mapSelectionFromSource(currentSelection(), (*iter)), QItemSelectionModel::ClearAndSelect);
        iter++;
    }
    _changeSelectionEnabled = true;
}

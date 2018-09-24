/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "abstractitemview.h"

AbstractItemView::AbstractItemView(QWidget* parent)
    : QWidget(parent)
    , _model(nullptr)
    , _selectionModel(nullptr)
{}

void AbstractItemView::setModel(QAbstractItemModel* model)
{
    if (_model) {
        disconnect(_model, nullptr, this, nullptr);
    }
    _model = model;
    connect(model, &QAbstractItemModel::dataChanged, this, &AbstractItemView::dataChanged);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &AbstractItemView::rowsAboutToBeRemoved);
    connect(model, &QAbstractItemModel::rowsInserted, this, &AbstractItemView::rowsInserted);
}

void AbstractItemView::setSelectionModel(QItemSelectionModel* selectionModel)
{
    if (_selectionModel) {
        disconnect(_selectionModel, nullptr, this, nullptr);
    }
    _selectionModel = selectionModel;
    connect(selectionModel, &QItemSelectionModel::currentChanged, this, &AbstractItemView::currentChanged);
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &AbstractItemView::selectionChanged);
}

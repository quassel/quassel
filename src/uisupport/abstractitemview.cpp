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

#include "abstractitemview.h"

AbstractItemView::AbstractItemView(QWidget *parent)
    : QWidget(parent),
    _model(0),
    _selectionModel(0)
{
}


void AbstractItemView::setModel(QAbstractItemModel *model)
{
    if (_model) {
        disconnect(_model, 0, this, 0);
    }
    _model = model;
    connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
        this, SLOT(dataChanged(QModelIndex, QModelIndex)));
    connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),
        this, SLOT(rowsAboutToBeRemoved(QModelIndex, int, int)));
    connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)),
        this, SLOT(rowsInserted(QModelIndex, int, int)));
}


void AbstractItemView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    if (_selectionModel) {
        disconnect(_selectionModel, 0, this, 0);
    }
    _selectionModel = selectionModel;
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
        this, SLOT(currentChanged(QModelIndex, QModelIndex)));
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
        this, SLOT(selectionChanged(QItemSelection, QItemSelection)));
}

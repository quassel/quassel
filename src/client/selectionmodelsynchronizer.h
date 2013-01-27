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

#ifndef _SELECTIONMODELSYNCHRONIZER_H_
#define _SELECTIONMODELSYNCHRONIZER_H_

#include <QObject>
#include <QItemSelectionModel>

class QAbstractItemModel;

class SelectionModelSynchronizer : public QObject
{
    Q_OBJECT

public:
    SelectionModelSynchronizer(QAbstractItemModel *parent = 0);

    void synchronizeSelectionModel(QItemSelectionModel *selectionModel);
    void removeSelectionModel(QItemSelectionModel *selectionModel);

    inline QAbstractItemModel *model() { return _model; }
    inline QItemSelectionModel *selectionModel() const { return const_cast<QItemSelectionModel *>(&_selectionModel); }
    inline QModelIndex currentIndex() const { return _selectionModel.currentIndex(); }
    inline QItemSelection currentSelection() const { return _selectionModel.selection(); }

private slots:
    void syncedCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void syncedSelectionChanged(const QItemSelection &selected, const QItemSelection &previous);

    void setCurrentIndex(const QModelIndex &index);
    void setCurrentSelection(const QItemSelection &selection);

    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void selectionModelDestroyed(QObject *object);

private:
    QAbstractItemModel *_model;
    QItemSelectionModel _selectionModel;
    bool _changeCurrentEnabled;
    bool _changeSelectionEnabled;

    bool checkBaseModel(QItemSelectionModel *model);
    QModelIndex mapFromSource(const QModelIndex &sourceIndex, const QItemSelectionModel *selectionModel);
    QItemSelection mapSelectionFromSource(const QItemSelection &sourceSelection, const QItemSelectionModel *selectionModel);
    QModelIndex mapToSource(const QModelIndex &index, QItemSelectionModel *selectionModel);
    QItemSelection mapSelectionToSource(const QItemSelection &selection, QItemSelectionModel *selectionModel);

    QSet<QItemSelectionModel *> _selectionModels;
};


#endif

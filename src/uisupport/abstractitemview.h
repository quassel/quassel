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

#ifndef ABSTRACTITEMVIEW_H
#define ABSTRACTITEMVIEW_H

#include <QWidget>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QItemSelection>
#include <QAbstractItemDelegate>
#include <QPointer>

class AbstractItemView : public QWidget
{
    Q_OBJECT

public:
    AbstractItemView(QWidget *parent = 0);

    inline QAbstractItemModel *model() { return _model; }
    void setModel(QAbstractItemModel *model);

    inline QItemSelectionModel *selectionModel() const { return _selectionModel; }
    void setSelectionModel(QItemSelectionModel *selectionModel);

    inline QModelIndex currentIndex() const { return _selectionModel->currentIndex(); }

protected slots:
    virtual void closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint) {};
    virtual void commitData(QWidget *) {};
    virtual void currentChanged(const QModelIndex &, const QModelIndex &) {};
    virtual void dataChanged(const QModelIndex &, const QModelIndex &) {};
    virtual void editorDestroyed(QObject *) {};
    virtual void rowsAboutToBeRemoved(const QModelIndex &, int, int) {};
    virtual void rowsInserted(const QModelIndex &, int, int) {};
    virtual void selectionChanged(const QItemSelection &, const QItemSelection &) {};

protected:
    QPointer<QAbstractItemModel> _model;
    QPointer<QItemSelectionModel> _selectionModel;
};


#endif // ABSTRACTITEMVIEW_H

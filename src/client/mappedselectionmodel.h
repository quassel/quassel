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

#ifndef _MAPPEDSELECTIONMODEL_H_
#define _MAPPEDSELECTIONMODEL_H_

#include <QObject>
#include <QModelIndex>
#include <QItemSelection>
#include <QItemSelectionModel>

class QAbstractProxyModel;

class MappedSelectionModel : public QItemSelectionModel {
  Q_OBJECT

public:
  MappedSelectionModel(QAbstractItemModel *model = 0);

  QModelIndex mapFromSource(const QModelIndex &sourceIndex);
  QItemSelection mapSelectionFromSource(const QItemSelection &sourceSelection);
									
public slots:
  void mappedSelect(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command);
  void mappedSetCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
};

#endif

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

#ifndef BUFFERMODEL_H
#define BUFFERMODEL_H

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QPointer>

class NetworkModel;
//class SelectionModelSynchronizer;
#include "selectionmodelsynchronizer.h"
//class ModelPropertyMapper;
#include "modelpropertymapper.h"
class MappedSelectionModel;
class QAbstractItemView;
class Buffer;

class BufferModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  BufferModel(NetworkModel *parent = 0);
  virtual ~BufferModel();

  bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const;
  
  inline SelectionModelSynchronizer *selectionModelSynchronizer() { return _selectionModelSynchronizer; }
  inline ModelPropertyMapper *propertyMapper() { return _propertyMapper; }

  void synchronizeSelectionModel(MappedSelectionModel *selectionModel);
  void synchronizeView(QAbstractItemView *view);
  void mapProperty(int column, int role, QObject *target, const QByteArray &property);

public slots:
  void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command);
  void selectBuffer(Buffer *buffer);

signals:
  void bufferSelected(Buffer *);
  void selectionChanged(const QModelIndex &);

private:
  QPointer<SelectionModelSynchronizer> _selectionModelSynchronizer;
  QPointer<ModelPropertyMapper> _propertyMapper;
  Buffer *currentBuffer;
};

#endif // BUFFERMODEL_H

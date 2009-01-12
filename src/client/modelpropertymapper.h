/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef _MODELPROPERTYMAPPER_H_
#define _MODELPROPERTYMAPPER_H_

#include <QPointer>
#include <QModelIndex>
#include <QObject>

class QAbstractItemModel;
class QItemSelectionModel;


class ModelPropertyMapper : public QObject {
  Q_OBJECT

public:
  ModelPropertyMapper(QObject *parent = 0);
  virtual ~ModelPropertyMapper();

  void setModel(QAbstractItemModel *model);
  QAbstractItemModel *model() const;

  void setSelectionModel(QItemSelectionModel *selectionModel);
  QItemSelectionModel *selectionModel() const;

  void addMapping(int column, int role, QObject *target, const QByteArray &property);
  void removeMapping(int column, int role, QObject *target, const QByteArray &property);
										      
public slots:
  void setCurrentIndex(const QModelIndex &current, const QModelIndex &previous);
  void setCurrentRow(const QModelIndex &current, const QModelIndex &previous);
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private slots:
  void targetDestroyed();
  
private:
  struct Mapping {
    int column;
    int role;
    QPointer<QObject> target;
    QByteArray property;

    Mapping(int _column, int _role, QObject *_target, const QByteArray &_property)
      : column(_column), role(_role), target(_target), property(_property) {};
    inline bool operator==(const Mapping &other) {
      return (column == other.column && role == other.role && target == other.target && property == other.property); }
  };
    
  QPointer<QAbstractItemModel> _model;
  QPointer<QItemSelectionModel> _selectionModel;
  QList<Mapping> _mappings;
  
};

#endif

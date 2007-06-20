/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _BUFFERVIEWFILTER_H_
#define _BUFFERVIEWFILTER_H_

#include <QFlags>
#include <QSortFilterProxyModel>
#include "buffer.h"
#include "buffertreemodel.h"

/*****************************************
 * Buffer View Filter
 *****************************************/
class BufferViewFilter : public QSortFilterProxyModel {
  Q_OBJECT
  
public:
  enum Mode {
    NoActive = 0x01,
    NoInactive = 0x02,
    SomeNets = 0x04,
    AllNets = 0x08,
    NoChannels = 0x10,
    NoQueries = 0x20,
    NoServers = 0x40
  };
  Q_DECLARE_FLAGS(Modes, Mode)

  BufferViewFilter(QAbstractItemModel *model, Modes mode, QStringList nets, QObject *parent = 0);
  
public slots:
  void invalidateMe();
  void changeCurrent(const QModelIndex &, const QModelIndex &);
  void doubleClickReceived(const QModelIndex &);
  void select(const QModelIndex &, QItemSelectionModel::SelectionFlags);
  
signals:
  void currentChanged(const QModelIndex &, const QModelIndex &);
  void doubleClicked(const QModelIndex &);
  void updateSelection(const QModelIndex &, QItemSelectionModel::SelectionFlags);
  
private:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

  Modes mode;
  QStringList networks;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(BufferViewFilter::Modes)    

#endif


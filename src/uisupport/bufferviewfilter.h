/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _BUFFERVIEWFILTER_H_
#define _BUFFERVIEWFILTER_H_

#include <QFlags>
#include <QDropEvent>
#include <QSortFilterProxyModel>
#include <QSet>
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
    NoServers = 0x40,
    FullCustom = 0x80
  };
  Q_DECLARE_FLAGS(Modes, Mode);

  BufferViewFilter(QAbstractItemModel *model, const Modes &mode, const QList<uint> &nets);
  
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
  
public slots:
  void removeBuffer(const QModelIndex &);
  void invalidateFilter_();
  
protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
  bool lessThan(const QModelIndex &, const QModelIndex &) const;
  
private:
  Modes mode;
  QSet<uint> networks;
  QSet<uint> buffers;

  bool filterAcceptBuffer(const QModelIndex &) const;
  bool filterAcceptNetwork(const QModelIndex &) const;
  void addBuffer(const uint &);

};
Q_DECLARE_OPERATORS_FOR_FLAGS(BufferViewFilter::Modes)    

#endif


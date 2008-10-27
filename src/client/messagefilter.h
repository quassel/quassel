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

#ifndef MESSAGEFILTER_H_
#define MESSAGEFILTER_H_

#include <QSortFilterProxyModel>

#include "messagemodel.h"
#include "types.h"

class MessageFilter : public QSortFilterProxyModel {
  Q_OBJECT

protected:
  MessageFilter(QAbstractItemModel *source, QObject *parent = 0);

public:
  MessageFilter(MessageModel *, const QList<BufferId> &buffers = QList<BufferId>(), QObject *parent = 0);

  virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
  virtual QString idString() const;
  inline bool isSingleBufferFilter() const { return _validBuffers.count() == 1; }
  inline bool containsBuffer(const BufferId &id) const { return _validBuffers.contains(id); }

public slots:
  void messageTypeFilterChanged();
  void requestBacklog();

private:
  void init();

  QSet<BufferId> _validBuffers;
  int _messageTypeFilter;
};

#endif

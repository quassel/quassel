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

#include "messagefilter.h"

MessageFilter::MessageFilter(QAbstractItemModel *source, QObject *parent) : QSortFilterProxyModel(parent) {
  setSourceModel(source);
}

MessageFilter::MessageFilter(MessageModel *source, const QList<BufferId> &buffers, QObject *parent)
  : QSortFilterProxyModel(parent),
    _bufferList(buffers)
{
  setSourceModel(source);
}

QString MessageFilter::idString() const {
  if(_bufferList.isEmpty()) return "*";
  QString idstr;
  QStringList bufids;
  foreach(BufferId id, _bufferList) bufids << QString::number(id.toInt());
  bufids.sort();
  foreach(QString id, bufids) idstr += id + '|';
  idstr.chop(1);
  return idstr;
}

bool MessageFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  Q_UNUSED(sourceParent);
  if(_bufferList.isEmpty()) return true;
  BufferId id = sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::BufferIdRole).value<BufferId>();
  return _bufferList.contains(id);
}

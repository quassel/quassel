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

MessageFilter::MessageFilter(QAbstractItemModel *source, QObject *parent)
  : QSortFilterProxyModel(parent)
{
  setSourceModel(source);
}

MessageFilter::MessageFilter(MessageModel *source, const QList<BufferId> &buffers, QObject *parent)
  : QSortFilterProxyModel(parent),
    _validBuffers(buffers.toSet())
{
  setSourceModel(source);
}

QString MessageFilter::idString() const {
  if(_validBuffers.isEmpty())
    return "*";

  QList<BufferId> bufferIds = _validBuffers.toList();;
  qSort(bufferIds);
  
  QStringList bufferIdStrings;
  foreach(BufferId id, bufferIds)
    bufferIdStrings << QString::number(id.toInt());

  return bufferIdStrings.join("|");
}

bool MessageFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  Q_UNUSED(sourceParent);
  if(_validBuffers.isEmpty())
    return true;

  BufferId id = sourceModel()->data(sourceModel()->index(sourceRow, 0), MessageModel::BufferIdRole).value<BufferId>();
  if(!id.isValid()) {
    return true;
  }
  return _validBuffers.contains(id);
}

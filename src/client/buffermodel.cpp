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

#include "buffermodel.h"

#include <QAbstractItemView>

#include "client.h"
#include "networkmodel.h"
#include "quassel.h"

BufferModel::BufferModel(NetworkModel *parent)
  : QSortFilterProxyModel(parent),
    _selectionModelSynchronizer(this)
{
  setSourceModel(parent);
  if(Quassel::isOptionSet("debugbufferswitches")) {
    connect(_selectionModelSynchronizer.selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
	    this, SLOT(debug_currentChanged(const QModelIndex &, const QModelIndex &)));
  }
  connect(Client::instance(), SIGNAL(networkCreated(NetworkId)), this, SLOT(newNetwork(NetworkId)));
}

bool BufferModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const {
  Q_UNUSED(sourceRow);
  // only networks and buffers are allowed
  if(!parent.isValid())
    return true;
  if(parent.data(NetworkModel::ItemTypeRole) == NetworkModel::NetworkItemType)
    return true;

  return false;
}

void BufferModel::newNetwork(NetworkId id) {
  const Network *net = Client::network(id);
  Q_ASSERT(net);
  connect(net, SIGNAL(connectionStateSet(Network::ConnectionState)),
	  this, SLOT(networkConnectionChanged(Network::ConnectionState)));
}

void BufferModel::networkConnectionChanged(Network::ConnectionState state) {
  switch(state) {
  case Network::Connecting:
  case Network::Initializing:
    if(currentIndex().isValid())
      return;
    {
      Network *net = qobject_cast<Network *>(sender());
      Q_ASSERT(net);
      setCurrentIndex(mapFromSource(Client::networkModel()->networkIndex(net->networkId())));
    }
    break;
  default:
    return;
  }
}

void BufferModel::synchronizeView(QAbstractItemView *view) {
  _selectionModelSynchronizer.synchronizeSelectionModel(view->selectionModel());
}

void BufferModel::setCurrentIndex(const QModelIndex &newCurrent) {
  _selectionModelSynchronizer.selectionModel()->setCurrentIndex(newCurrent, QItemSelectionModel::Current);
  _selectionModelSynchronizer.selectionModel()->select(newCurrent, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void BufferModel::switchToBuffer(const BufferId &bufferId) {
  QModelIndex source_index = Client::networkModel()->bufferIndex(bufferId);
  setCurrentIndex(mapFromSource(source_index));
}

void BufferModel::switchToBufferIndex(const QModelIndex &bufferIdx) {
  // we accept indexes that directly belong to us or our parent - nothing else
  if(bufferIdx.model() == this) {
    setCurrentIndex(bufferIdx);
    return;
  }

  if(bufferIdx.model() == sourceModel()) {
    setCurrentIndex(mapFromSource(bufferIdx));
    return;
  }

  qWarning() << "BufferModel::switchToBufferIndex(const QModelIndex &):" << bufferIdx << "does not belong to BufferModel or NetworkModel";
}

void BufferModel::debug_currentChanged(QModelIndex current, QModelIndex previous) {
  Q_UNUSED(previous);
  qDebug() << "Switched current Buffer: " << current << current.data().toString() << "Buffer:" << current.data(NetworkModel::BufferIdRole).value<BufferId>();
}

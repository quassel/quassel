/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include "buffersyncer.h"

BufferSyncer::BufferSyncer(QObject *parent) : SyncableObject(parent) {


}


QDateTime BufferSyncer::lastSeen(BufferId buffer) const {
  if(_lastSeen.contains(buffer)) return _lastSeen[buffer];
  return QDateTime();
}

bool BufferSyncer::setLastSeen(BufferId buffer, const QDateTime &time) {
  if(!time.isValid()) return false;
  if(!lastSeen(buffer).isValid() || lastSeen(buffer) < time) {
    _lastSeen[buffer] = time;
    emit lastSeenSet(buffer, time);
    return true;
  }
  return false;
}

QVariantList BufferSyncer::initLastSeen() const {
  QVariantList list;
  foreach(BufferId id, _lastSeen.keys()) {
    list << QVariant::fromValue<BufferId>(id) << _lastSeen[id];
  }
  return list;
}

void BufferSyncer::initSetLastSeen(const QVariantList &list) {
  _lastSeen.clear();
  Q_ASSERT(list.count() % 2 == 0);
  for(int i = 0; i < list.count(); i += 2) {
    setLastSeen(list[i].value<BufferId>(), list[i+1].toDateTime());
  }
}

void BufferSyncer::requestSetLastSeen(BufferId buffer, const QDateTime &time) {
  if(setLastSeen(buffer, time)) emit setLastSeenRequested(buffer, time);
}


void BufferSyncer::requestRemoveBuffer(BufferId buffer) {
  emit removeBufferRequested(buffer);
}

void BufferSyncer::removeBuffer(BufferId buffer) {
  if(_lastSeen.contains(buffer))
    _lastSeen.remove(buffer);
  emit bufferRemoved(buffer);
}

void BufferSyncer::renameBuffer(BufferId buffer, QString newName) {
  emit bufferRenamed(buffer, newName);
}

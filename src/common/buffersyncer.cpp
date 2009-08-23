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

#include "buffersyncer.h"

INIT_SYNCABLE_OBJECT(BufferSyncer)
BufferSyncer::BufferSyncer(QObject *parent)
  : SyncableObject(parent)
{
}

BufferSyncer::BufferSyncer(const QHash<BufferId, MsgId> &lastSeenMsg, QObject *parent)
  : SyncableObject(parent),
    _lastSeenMsg(lastSeenMsg)
{
}

MsgId BufferSyncer::lastSeenMsg(BufferId buffer) const {
  if(_lastSeenMsg.contains(buffer))
    return _lastSeenMsg[buffer];
  return MsgId();
}

bool BufferSyncer::setLastSeenMsg(BufferId buffer, const MsgId &msgId) {
  if(!msgId.isValid())
    return false;

  const MsgId oldLastSeenMsg = lastSeenMsg(buffer);
  if(!oldLastSeenMsg.isValid() || oldLastSeenMsg < msgId) {
    _lastSeenMsg[buffer] = msgId;
    SYNC(ARG(buffer), ARG(msgId))
    emit lastSeenMsgSet(buffer, msgId);
    return true;
  }
  return false;
}

QVariantList BufferSyncer::initLastSeenMsg() const {
  QVariantList list;
  QHash<BufferId, MsgId>::const_iterator iter = _lastSeenMsg.constBegin();
  while(iter != _lastSeenMsg.constEnd()) {
    list << QVariant::fromValue<BufferId>(iter.key())
	 << QVariant::fromValue<MsgId>(iter.value());
    iter++;
  }
  return list;
}

void BufferSyncer::initSetLastSeenMsg(const QVariantList &list) {
  _lastSeenMsg.clear();
  Q_ASSERT(list.count() % 2 == 0);
  for(int i = 0; i < list.count(); i += 2) {
    setLastSeenMsg(list[i].value<BufferId>(), list[i+1].value<MsgId>());
  }
}

void BufferSyncer::removeBuffer(BufferId buffer) {
  if(_lastSeenMsg.contains(buffer))
    _lastSeenMsg.remove(buffer);
  SYNC(ARG(buffer))
  emit bufferRemoved(buffer);
}

void BufferSyncer::mergeBuffersPermanently(BufferId buffer1, BufferId buffer2) {
  if(_lastSeenMsg.contains(buffer2))
    _lastSeenMsg.remove(buffer2);
  SYNC(ARG(buffer1), ARG(buffer2))
  emit buffersPermanentlyMerged(buffer1, buffer2);
}

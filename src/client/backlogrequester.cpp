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

#include "backlogrequester.h"

#include <QObject>

#include "backlogsettings.h"
#include "clientbacklogmanager.h"

BacklogRequester::BacklogRequester(bool buffering, ClientBacklogManager *backlogManager)
  : backlogManager(backlogManager),
    _isBuffering(buffering)
{
  Q_ASSERT(backlogManager);
}

bool BacklogRequester::buffer(BufferId bufferId, const MessageList &messages) {
  _bufferedMessages << messages;
  _buffersWaiting.remove(bufferId);
  return !_buffersWaiting.isEmpty();
}

// ========================================
//  FIXED BACKLOG REQUESTER
// ========================================
FixedBacklogRequester::FixedBacklogRequester(ClientBacklogManager *backlogManager)
  : BacklogRequester(true, backlogManager)
{
  BacklogSettings backlogSettings;
  _backlogCount = backlogSettings.fixedBacklogAmount();
}

void FixedBacklogRequester::requestBacklog() {
  QList<BufferId> allBuffers = allBufferIds();
  setWaitingBuffers(allBuffers);
  backlogManager->emitMessagesRequested(QObject::tr("Requesting a total of up to %1 backlog messages for %2 buffers").arg(_backlogCount * allBuffers.count()).arg(allBuffers.count()));
  foreach(BufferId bufferId, allBuffers) {
    backlogManager->requestBacklog(bufferId, -1, -1, _backlogCount);
  }
}

// ========================================
//  GLOBAL UNREAD BACKLOG REQUESTER
// ========================================
GlobalUnreadBacklogRequester::GlobalUnreadBacklogRequester(ClientBacklogManager *backlogManager)
  : BacklogRequester(false, backlogManager)
{
  BacklogSettings backlogSettings;
  _limit = backlogSettings.globalUnreadBacklogLimit();
  _additional = backlogSettings.globalUnreadBacklogAdditional();
}

void GlobalUnreadBacklogRequester::requestBacklog() {
  MsgId oldestUnreadMessage;
  foreach(BufferId bufferId, allBufferIds()) {
    MsgId msgId = Client::networkModel()->lastSeenMsgId(bufferId);
    if(!oldestUnreadMessage.isValid() || oldestUnreadMessage > msgId)
      oldestUnreadMessage = msgId;
  }
  backlogManager->emitMessagesRequested(QObject::tr("Requesting up to %1 of all unread backlog messages (plus additional %2)").arg(_limit).arg(_additional));
  backlogManager->requestBacklogAll(oldestUnreadMessage, -1, _limit, _additional);
}

// ========================================
//  PER BUFFER UNREAD BACKLOG REQUESTER
// ========================================
PerBufferUnreadBacklogRequester::PerBufferUnreadBacklogRequester(ClientBacklogManager *backlogManager)
  : BacklogRequester(true, backlogManager)
{
  BacklogSettings backlogSettings;
  _limit = backlogSettings.perBufferUnreadBacklogLimit();
  _additional = backlogSettings.perBufferUnreadBacklogAdditional();
}

void PerBufferUnreadBacklogRequester::requestBacklog() {
  QList<BufferId> allBuffers = allBufferIds();
  setWaitingBuffers(allBuffers);
  backlogManager->emitMessagesRequested(QObject::tr("Requesting a total of up to %1 unread backlog messages for %2 buffers").arg((_limit + _additional) * allBuffers.count()).arg(allBuffers.count()));
  foreach(BufferId bufferId, allBuffers) {
    backlogManager->requestBacklog(bufferId, Client::networkModel()->lastSeenMsgId(bufferId), -1, _limit, _additional);
  }
}

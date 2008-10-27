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

#include <QDebug>

#include "backlogmanager.h"

BacklogRequester::BacklogRequester(bool buffering, BacklogManager *backlogManager)
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
FixedBacklogRequester::FixedBacklogRequester(BacklogManager *backlogManager)
  : BacklogRequester(true, backlogManager),
    _backlogCount(500)
{
}

void FixedBacklogRequester::requestBacklog() {
  QList<BufferId> allBuffers = allBufferIds();
  setWaitingBuffers(allBuffers);
  foreach(BufferId bufferId, allBuffers) {
    backlogManager->requestBacklog(bufferId, _backlogCount, -1);
  }
}

/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "backlogrequester.h"

#include <QObject>

#include "backlogsettings.h"
#include "bufferviewoverlay.h"
#include "clientbacklogmanager.h"

BacklogRequester::BacklogRequester(bool buffering, RequesterType requesterType, ClientBacklogManager* backlogManager)
    : backlogManager(backlogManager)
    , _isBuffering(buffering)
    , _requesterType(requesterType)
    , _totalBuffers(0)
{
    Q_ASSERT(backlogManager);
}

void BacklogRequester::setWaitingBuffers(const BufferIdList& buffers)
{
    _buffersWaiting = {buffers.begin(), buffers.end()};
    _totalBuffers = int(_buffersWaiting.size());
}

bool BacklogRequester::buffer(BufferId bufferId, const MessageList& messages)
{
    _bufferedMessages << messages;
    _buffersWaiting.erase(bufferId);
    return !_buffersWaiting.empty();
}

BufferIdList BacklogRequester::allBufferIds() const
{
    QSet<BufferId> bufferIds = Client::bufferViewOverlay()->bufferIds();
    bufferIds += Client::bufferViewOverlay()->tempRemovedBufferIds();
    return bufferIds.values();
}

void BacklogRequester::flushBuffer()
{
    if (!_buffersWaiting.empty()) {
        qWarning() << Q_FUNC_INFO << "was called before all backlog was received:" << _buffersWaiting.size() << "buffers are waiting.";
    }
    _bufferedMessages.clear();
    _totalBuffers = 0;
    _buffersWaiting.clear();
}

// ========================================
//  FIXED BACKLOG REQUESTER
// ========================================
FixedBacklogRequester::FixedBacklogRequester(ClientBacklogManager* backlogManager)
    : BacklogRequester(true, BacklogRequester::PerBufferFixed, backlogManager)
{
    BacklogSettings backlogSettings;
    _backlogCount = backlogSettings.fixedBacklogAmount();
}

void FixedBacklogRequester::requestBacklog(const BufferIdList& bufferIds)
{
    setWaitingBuffers(bufferIds);
    backlogManager->emitMessagesRequested(QObject::tr("Requesting a total of up to %1 backlog messages for %2 buffers")
                                              .arg(_backlogCount * bufferIds.count())
                                              .arg(bufferIds.count()));
    foreach (BufferId bufferId, bufferIds) {
        backlogManager->requestBacklog(bufferId, -1, -1, _backlogCount);
    }
}

// ========================================
//  GLOBAL UNREAD BACKLOG REQUESTER
// ========================================
GlobalUnreadBacklogRequester::GlobalUnreadBacklogRequester(ClientBacklogManager* backlogManager)
    : BacklogRequester(false, BacklogRequester::GlobalUnread, backlogManager)
{
    BacklogSettings backlogSettings;
    _limit = backlogSettings.globalUnreadBacklogLimit();
    _additional = backlogSettings.globalUnreadBacklogAdditional();
}

void GlobalUnreadBacklogRequester::requestInitialBacklog()
{
    MsgId oldestUnreadMessage;
    foreach (BufferId bufferId, allBufferIds()) {
        MsgId msgId = Client::networkModel()->lastSeenMsgId(bufferId);
        if (!oldestUnreadMessage.isValid() || oldestUnreadMessage > msgId)
            oldestUnreadMessage = msgId;
    }
    backlogManager->emitMessagesRequested(
        QObject::tr("Requesting up to %1 of all unread backlog messages (plus additional %2)").arg(_limit).arg(_additional));
    backlogManager->requestBacklogAll(oldestUnreadMessage, -1, _limit, _additional);
}

// ========================================
//  PER BUFFER UNREAD BACKLOG REQUESTER
// ========================================
PerBufferUnreadBacklogRequester::PerBufferUnreadBacklogRequester(ClientBacklogManager* backlogManager)
    : BacklogRequester(true, BacklogRequester::PerBufferUnread, backlogManager)
{
    BacklogSettings backlogSettings;
    _limit = backlogSettings.perBufferUnreadBacklogLimit();
    _additional = backlogSettings.perBufferUnreadBacklogAdditional();
}

void PerBufferUnreadBacklogRequester::requestBacklog(const BufferIdList& bufferIds)
{
    setWaitingBuffers(bufferIds);
    backlogManager->emitMessagesRequested(QObject::tr("Requesting a total of up to %1 unread backlog messages for %2 buffers")
                                              .arg((_limit + _additional) * bufferIds.count())
                                              .arg(bufferIds.count()));
    foreach (BufferId bufferId, bufferIds) {
        backlogManager->requestBacklog(bufferId, Client::networkModel()->lastSeenMsgId(bufferId), -1, _limit, _additional);
    }
}

// ========================================
//  AS NEEDED BACKLOG REQUESTER
// ========================================
AsNeededBacklogRequester::AsNeededBacklogRequester(ClientBacklogManager* backlogManager)
    : BacklogRequester(false, BacklogRequester::AsNeeded, backlogManager)
{
    BacklogSettings backlogSettings;
    _legacyBacklogCount = backlogSettings.asNeededLegacyBacklogAmount();
}

void AsNeededBacklogRequester::requestBacklog(const BufferIdList& bufferIds)
{
    // Check if the core supports activity tracking
    if (Client::isCoreFeatureEnabled(Quassel::Feature::BufferActivitySync)) {
        // Don't fetch any backlog, the core will track buffer activity for us
        return;
    }

    setWaitingBuffers(bufferIds);
    backlogManager->emitMessagesRequested(QObject::tr("Requesting a total of up to %1 backlog messages for %2 buffers")
                                              .arg(_legacyBacklogCount * bufferIds.count())
                                              .arg(bufferIds.count()));
    foreach (BufferId bufferId, bufferIds) {
        backlogManager->requestBacklog(bufferId, -1, -1, _legacyBacklogCount);
    }
}

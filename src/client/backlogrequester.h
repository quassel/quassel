/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#pragma once

#include <set>

#include <QList>

#include "client.h"
#include "message.h"
#include "networkmodel.h"
#include "types.h"

class ClientBacklogManager;

class BacklogRequester
{
public:
    enum RequesterType
    {
        InvalidRequester = 0,
        PerBufferFixed,
        PerBufferUnread,
        GlobalUnread
    };

    BacklogRequester(bool buffering, RequesterType requesterType, ClientBacklogManager* backlogManger);
    virtual ~BacklogRequester() = default;

    inline bool isBuffering() { return _isBuffering; }
    inline RequesterType type() { return _requesterType; }
    inline const QList<Message>& bufferedMessages() { return _bufferedMessages; }

    inline int buffersWaiting() const { return int(_buffersWaiting.size()); }
    inline int totalBuffers() const { return _totalBuffers; }

    bool buffer(BufferId bufferId, const MessageList& messages);  //! returns false if it was the last missing backlogpart

    virtual void requestBacklog(const BufferIdList& bufferIds) = 0;
    virtual inline void requestInitialBacklog() { requestBacklog(allBufferIds()); }

    virtual void flushBuffer();

protected:
    BufferIdList allBufferIds() const;
    void setWaitingBuffers(const BufferIdList& buffers);

    ClientBacklogManager* backlogManager;

private:
    bool _isBuffering;
    RequesterType _requesterType;
    MessageList _bufferedMessages;
    int _totalBuffers;
    std::set<BufferId> _buffersWaiting;
};

// ========================================
//  FIXED BACKLOG REQUESTER
// ========================================
class FixedBacklogRequester : public BacklogRequester
{
public:
    FixedBacklogRequester(ClientBacklogManager* backlogManager);
    void requestBacklog(const BufferIdList& bufferIds) override;

private:
    int _backlogCount;
};

// ========================================
//  GLOBAL UNREAD BACKLOG REQUESTER
// ========================================
class GlobalUnreadBacklogRequester : public BacklogRequester
{
public:
    GlobalUnreadBacklogRequester(ClientBacklogManager* backlogManager);
    void requestInitialBacklog() override;
    void requestBacklog(const BufferIdList&) override {}

private:
    int _limit;
    int _additional;
};

// ========================================
//  PER BUFFER UNREAD BACKLOG REQUESTER
// ========================================
class PerBufferUnreadBacklogRequester : public BacklogRequester
{
public:
    PerBufferUnreadBacklogRequester(ClientBacklogManager* backlogManager);
    void requestBacklog(const BufferIdList& bufferIds) override;

private:
    int _limit;
    int _additional;
};

/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef BACKLOGREQUESTER_H
#define BACKLOGREQUESTER_H

#include <QList>

#include "client.h"
#include "message.h"
#include "networkmodel.h"
#include "types.h"

class ClientBacklogManager;

class BacklogRequester
{
public:
    enum RequesterType {
        InvalidRequester = 0,
        PerBufferFixed,
        PerBufferUnread,
        GlobalUnread
    };

    BacklogRequester(bool buffering, RequesterType requesterType, ClientBacklogManager *backlogManger);
    virtual inline ~BacklogRequester() {}

    inline bool isBuffering() { return _isBuffering; }
    inline RequesterType type() { return _requesterType; }
    inline const QList<Message> &bufferedMessages() { return _bufferedMessages; }

    inline int buffersWaiting() const { return _buffersWaiting.count(); }
    inline int totalBuffers() const { return _totalBuffers; }

    bool buffer(BufferId bufferId, const MessageList &messages); //! returns false if it was the last missing backlogpart

    virtual void requestBacklog(const BufferIdList &bufferIds) = 0;
    virtual inline void requestInitialBacklog() { requestBacklog(allBufferIds()); }

    virtual void flushBuffer();

protected:
    BufferIdList allBufferIds() const;
    inline void setWaitingBuffers(const QList<BufferId> &buffers) { setWaitingBuffers(buffers.toSet()); }
    void setWaitingBuffers(const QSet<BufferId> &buffers);
    void addWaitingBuffer(BufferId buffer);

    ClientBacklogManager *backlogManager;

private:
    bool _isBuffering;
    RequesterType _requesterType;
    MessageList _bufferedMessages;
    int _totalBuffers;
    QSet<BufferId> _buffersWaiting;
};


// ========================================
//  FIXED BACKLOG REQUESTER
// ========================================
class FixedBacklogRequester : public BacklogRequester
{
public:
    FixedBacklogRequester(ClientBacklogManager *backlogManager);
    virtual void requestBacklog(const BufferIdList &bufferIds);

private:
    int _backlogCount;
};


// ========================================
//  GLOBAL UNREAD BACKLOG REQUESTER
// ========================================
class GlobalUnreadBacklogRequester : public BacklogRequester
{
public:
    GlobalUnreadBacklogRequester(ClientBacklogManager *backlogManager);
    virtual void requestInitialBacklog();
    virtual void requestBacklog(const BufferIdList &) {}

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
    PerBufferUnreadBacklogRequester(ClientBacklogManager *backlogManager);
    virtual void requestBacklog(const BufferIdList &bufferIds);

private:
    int _limit;
    int _additional;
};


#endif //BACKLOGREQUESTER_H

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

#ifndef BACKLOGREQUESTER_H
#define BACKLOGREQUESTER_H

#include <QList>

#include "client.h"
#include "message.h"
#include "networkmodel.h"
#include "types.h"

class ClientBacklogManager;

class BacklogRequester {
public:
  enum RequesterTypes {
    InvalidRequester = 0,
    PerBufferFixed,
    PerBufferUnread,
    GlobalUnread
  };

  BacklogRequester(bool buffering, ClientBacklogManager *backlogManger);
  virtual inline ~BacklogRequester() {}

  inline bool isBuffering() { return _isBuffering; }
  inline const QList<Message> &bufferedMessages() { return _bufferedMessages; }

  //! returns false if it was the last missing backlogpart
  bool buffer(BufferId bufferId, const MessageList &messages);
  
  virtual void requestBacklog() = 0;

protected:
  inline QList<BufferId> allBufferIds() const { return Client::networkModel()->allBufferIds(); }
  inline void setWaitingBuffers(const QList<BufferId> &buffers) { _buffersWaiting = buffers.toSet(); }
  inline void setWaitingBuffers(const QSet<BufferId> &buffers) { _buffersWaiting = buffers; }
  inline void addWaitingBuffer(BufferId buffer) { _buffersWaiting << buffer; }

  ClientBacklogManager *backlogManager;

private:
  bool _isBuffering;
  MessageList _bufferedMessages;
  QSet<BufferId> _buffersWaiting;
};

// ========================================
//  FIXED BACKLOG REQUESTER
// ========================================
class FixedBacklogRequester : public BacklogRequester {
public:
  FixedBacklogRequester(ClientBacklogManager *backlogManager);
  virtual void requestBacklog();

private:
  int _backlogCount;
};

// ========================================
//  GLOBAL UNREAD BACKLOG REQUESTER
// ========================================
class GlobalUnreadBacklogRequester : public BacklogRequester {
public:
  GlobalUnreadBacklogRequester(ClientBacklogManager *backlogManager);
  virtual void requestBacklog();

private:
  int _limit;
  int _additional;
};

// ========================================
//  PER BUFFER UNREAD BACKLOG REQUESTER
// ========================================
class PerBufferUnreadBacklogRequester : public BacklogRequester {
public:
  PerBufferUnreadBacklogRequester(ClientBacklogManager *backlogManager);
  virtual void requestBacklog();

private:
  int _limit;
  int _additional;
};

#endif //BACKLOGREQUESTER_H

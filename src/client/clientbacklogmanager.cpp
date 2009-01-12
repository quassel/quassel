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

#include "clientbacklogmanager.h"

#include "abstractmessageprocessor.h"
#include "backlogsettings.h"
#include "backlogrequester.h"
#include "client.h"

#include <ctime>

#include <QDebug>

ClientBacklogManager::ClientBacklogManager(QObject *parent)
  : BacklogManager(parent),
    _requester(0)
{
}

void ClientBacklogManager::receiveBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional, QVariantList msgs) {
  Q_UNUSED(first) Q_UNUSED(last) Q_UNUSED(limit) Q_UNUSED(additional)

  emit messagesReceived(bufferId, msgs.count());

  MessageList msglist;
  foreach(QVariant v, msgs) {
    Message msg = v.value<Message>();
    msg.setFlags(msg.flags() | Message::Backlog);
    msglist << msg;
  }

  if(isBuffering()) {
    if(!_requester->buffer(bufferId, msglist)) {
      // this was the last part to buffer
      stopBuffering();
      reset();
    }
  } else {
    dispatchMessages(msglist);
  }
}

void ClientBacklogManager::receiveBacklogAll(MsgId first, MsgId last, int limit, int additional, QVariantList msgs) {
  Q_UNUSED(first) Q_UNUSED(last) Q_UNUSED(limit) Q_UNUSED(additional)

  MessageList msglist;
  foreach(QVariant v, msgs) {
    Message msg = v.value<Message>();
    msg.setFlags(msg.flags() | Message::Backlog);
    msglist << msg;
  }

  dispatchMessages(msglist);
  reset();
}

void ClientBacklogManager::requestInitialBacklog() {
  if(_requester) {
    qWarning() << "ClientBacklogManager::requestInitialBacklog() called twice in the same session! (Backlog has already been requested)";
    return;
  }

  BacklogSettings settings;
  switch(settings.requesterType()) {
  case BacklogRequester::GlobalUnread:
    _requester = new GlobalUnreadBacklogRequester(this);
    break;
  case BacklogRequester::PerBufferUnread:
    _requester = new PerBufferUnreadBacklogRequester(this);
    break;
  case BacklogRequester::PerBufferFixed:
  default:
    _requester = new FixedBacklogRequester(this);
  };

  _requester->requestBacklog();
}

void ClientBacklogManager::stopBuffering() {
  Q_ASSERT(_requester);

  dispatchMessages(_requester->bufferedMessages(), true);
}

bool ClientBacklogManager::isBuffering() {
  return _requester && _requester->isBuffering();
}

void ClientBacklogManager::dispatchMessages(const MessageList &messages, bool sort) {
  if(messages.isEmpty())
    return;

  MessageList msgs = messages;

  clock_t start_t = clock();
  if(sort)
    qSort(msgs);
  Client::messageProcessor()->process(msgs);
  clock_t end_t = clock();

  emit messagesProcessed(tr("Processed %1 messages in %2 seconds.").arg(messages.count()).arg((float)(end_t - start_t) / CLOCKS_PER_SEC));
}

void ClientBacklogManager::reset() {
  delete _requester;
  _requester = 0;
}

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

#include "clientbacklogmanager.h"

#include "abstractmessageprocessor.h"
#include "backlogsettings.h"
#include "backlogrequester.h"
#include "client.h"

#include <ctime>

#include <QDebug>

INIT_SYNCABLE_OBJECT(ClientBacklogManager)
ClientBacklogManager::ClientBacklogManager(QObject *parent)
    : BacklogManager(parent),
    _requester(0),
    _initBacklogRequested(false)
{
}


QVariantList ClientBacklogManager::requestBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional)
{
    _buffersRequested << bufferId;
    return BacklogManager::requestBacklog(bufferId, first, last, limit, additional);
}


void ClientBacklogManager::receiveBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional, QVariantList msgs)
{
    Q_UNUSED(first) Q_UNUSED(last) Q_UNUSED(limit) Q_UNUSED(additional)

    emit messagesReceived(bufferId, msgs.count());

    MessageList msglist;
    foreach(QVariant v, msgs) {
        Message msg = v.value<Message>();
        msg.setFlags(msg.flags() | Message::Backlog);
        msglist << msg;
    }

    if (isBuffering()) {
        bool lastPart = !_requester->buffer(bufferId, msglist);
        updateProgress(_requester->totalBuffers() - _requester->buffersWaiting(), _requester->totalBuffers());
        if (lastPart) {
            dispatchMessages(_requester->bufferedMessages(), true);
            _requester->flushBuffer();
        }
    }
    else {
        dispatchMessages(msglist);
    }
}


void ClientBacklogManager::receiveBacklogAll(MsgId first, MsgId last, int limit, int additional, QVariantList msgs)
{
    Q_UNUSED(first) Q_UNUSED(last) Q_UNUSED(limit) Q_UNUSED(additional)

    MessageList msglist;
    foreach(QVariant v, msgs) {
        Message msg = v.value<Message>();
        msg.setFlags(msg.flags() | Message::Backlog);
        msglist << msg;
    }

    dispatchMessages(msglist);
}


void ClientBacklogManager::requestInitialBacklog()
{
    if (_initBacklogRequested) {
        Q_ASSERT(_requester);
        qWarning() << "ClientBacklogManager::requestInitialBacklog() called twice in the same session! (Backlog has already been requested)";
        return;
    }

    BacklogSettings settings;
    switch (settings.requesterType()) {
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

    _requester->requestInitialBacklog();
    _initBacklogRequested = true;
    if (_requester->isBuffering()) {
        updateProgress(0, _requester->totalBuffers());
    }
}


BufferIdList ClientBacklogManager::filterNewBufferIds(const BufferIdList &bufferIds)
{
    BufferIdList newBuffers;
    QSet<BufferId> availableBuffers = Client::networkModel()->allBufferIds().toSet();
    foreach(BufferId bufferId, bufferIds) {
        if (_buffersRequested.contains(bufferId) || !availableBuffers.contains(bufferId))
            continue;
        newBuffers << bufferId;
    }
    return newBuffers;
}


void ClientBacklogManager::checkForBacklog(const QList<BufferId> &bufferIds)
{
    // we ingore all backlogrequests until we had our initial request
    if (!_initBacklogRequested) {
        return;
    }

    if (!_requester) {
        // during client start up this message is to be expected in some situations.
        qDebug() << "ClientBacklogManager::checkForBacklog(): no active backlog requester.";
        return;
    }
    switch (_requester->type()) {
    case BacklogRequester::GlobalUnread:
        break;
    case BacklogRequester::PerBufferUnread:
    case BacklogRequester::PerBufferFixed:
    default:
    {
        BufferIdList buffers = filterNewBufferIds(bufferIds);
        if (!buffers.isEmpty())
            _requester->requestBacklog(buffers);
    }
    };
}


bool ClientBacklogManager::isBuffering()
{
    return _requester && _requester->isBuffering();
}


void ClientBacklogManager::dispatchMessages(const MessageList &messages, bool sort)
{
    if (messages.isEmpty())
        return;

    MessageList msgs = messages;

    clock_t start_t = clock();
    if (sort)
        qSort(msgs);
    Client::messageProcessor()->process(msgs);
    clock_t end_t = clock();

    emit messagesProcessed(tr("Processed %1 messages in %2 seconds.").arg(messages.count()).arg((float)(end_t - start_t) / CLOCKS_PER_SEC));
}


void ClientBacklogManager::reset()
{
    delete _requester;
    _requester = 0;
    _initBacklogRequested = false;
    _buffersRequested.clear();
}

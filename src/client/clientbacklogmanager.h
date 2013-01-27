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

#ifndef CLIENTBACKLOGMANAGER_H
#define CLIENTBACKLOGMANAGER_H

#include "backlogmanager.h"
#include "message.h"

class BacklogRequester;

class ClientBacklogManager : public BacklogManager
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    ClientBacklogManager(QObject *parent = 0);

    // helper for the backlogRequester, as it isn't a QObject and can't emit itself
    inline void emitMessagesRequested(const QString &msg) const { emit messagesRequested(msg); }

    void reset();

public slots:
    virtual QVariantList requestBacklog(BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0);
    virtual void receiveBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional, QVariantList msgs);
    virtual void receiveBacklogAll(MsgId first, MsgId last, int limit, int additional, QVariantList msgs);

    void requestInitialBacklog();

    void checkForBacklog(BufferId bufferId);
    void checkForBacklog(const BufferIdList &bufferIds);

signals:
    void messagesReceived(BufferId bufferId, int count) const;
    void messagesRequested(const QString &) const;
    void messagesProcessed(const QString &) const;

    void updateProgress(int, int);

private:
    bool isBuffering();
    BufferIdList filterNewBufferIds(const BufferIdList &bufferIds);

    void dispatchMessages(const MessageList &messages, bool sort = false);

    BacklogRequester *_requester;
    bool _initBacklogRequested;
    QSet<BufferId> _buffersRequested;
};


// inlines
inline void ClientBacklogManager::checkForBacklog(BufferId bufferId)
{
    checkForBacklog(BufferIdList() << bufferId);
}


#endif // CLIENTBACKLOGMANAGER_H

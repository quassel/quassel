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

#include "corebacklogmanager.h"
#include "core.h"
#include "coresession.h"

#include <QDebug>

INIT_SYNCABLE_OBJECT(CoreBacklogManager)
CoreBacklogManager::CoreBacklogManager(CoreSession *coreSession)
    : BacklogManager(coreSession),
    _coreSession(coreSession)
{
}


QVariantList CoreBacklogManager::requestBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional)
{
    QVariantList backlog;
    QList<Message> msgList;
    msgList = Core::requestMsgs(coreSession()->user(), bufferId, first, last, limit);

    QList<Message>::const_iterator msgIter = msgList.constBegin();
    QList<Message>::const_iterator msgListEnd = msgList.constEnd();
    while (msgIter != msgListEnd) {
        backlog << qVariantFromValue(*msgIter);
        msgIter++;
    }

    if (additional && limit != 0) {
        MsgId oldestMessage = first;
        if (!msgList.isEmpty()) {
            if (msgList.first().msgId() < msgList.last().msgId())
                oldestMessage = msgList.first().msgId();
            else
                oldestMessage = msgList.last().msgId();
        }

        if (first != -1) {
            last = first;
        }
        else {
            last = oldestMessage;
        }

        // only fetch additional messages if they continue seemlessly
        // that is, if the list of messages is not truncated by the limit
        if (last == oldestMessage) {
            msgList = Core::requestMsgs(coreSession()->user(), bufferId, -1, last, additional);
            msgIter = msgList.constBegin();
            msgListEnd = msgList.constEnd();
            while (msgIter != msgListEnd) {
                backlog << qVariantFromValue(*msgIter);
                msgIter++;
            }
        }
    }

    return backlog;
}


QVariantList CoreBacklogManager::requestBacklogAll(MsgId first, MsgId last, int limit, int additional)
{
    QVariantList backlog;
    QList<Message> msgList;
    msgList = Core::requestAllMsgs(coreSession()->user(), first, last, limit);

    QList<Message>::const_iterator msgIter = msgList.constBegin();
    QList<Message>::const_iterator msgListEnd = msgList.constEnd();
    while (msgIter != msgListEnd) {
        backlog << qVariantFromValue(*msgIter);
        msgIter++;
    }

    if (additional) {
        if (first != -1) {
            last = first;
        }
        else {
            last = -1;
            if (!msgList.isEmpty()) {
                if (msgList.first().msgId() < msgList.last().msgId())
                    last = msgList.first().msgId();
                else
                    last = msgList.last().msgId();
            }
        }
        msgList = Core::requestAllMsgs(coreSession()->user(), -1, last, additional);
        msgIter = msgList.constBegin();
        msgListEnd = msgList.constEnd();
        while (msgIter != msgListEnd) {
            backlog << qVariantFromValue(*msgIter);
            msgIter++;
        }
    }

    return backlog;
}

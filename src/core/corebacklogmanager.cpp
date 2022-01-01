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

#include "corebacklogmanager.h"

#include <algorithm>
#include <iterator>

#include <QDebug>

#include "core.h"
#include "coresession.h"

CoreBacklogManager::CoreBacklogManager(CoreSession* coreSession)
    : BacklogManager(coreSession)
    , _coreSession(coreSession)
{}

QVariantList CoreBacklogManager::requestBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional)
{
    QVariantList backlog;
    auto msgList = Core::requestMsgs(coreSession()->user(), bufferId, first, last, limit);

    std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
        return QVariant::fromValue(msg);
    });

    if (additional && limit != 0) {
        MsgId oldestMessage = first;
        if (!msgList.empty()) {
            if (msgList.front().msgId() < msgList.back().msgId())
                oldestMessage = msgList.front().msgId();
            else
                oldestMessage = msgList.back().msgId();
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
            std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
                return QVariant::fromValue(msg);
            });
        }
    }

    return backlog;
}

QVariantList CoreBacklogManager::requestBacklogFiltered(BufferId bufferId, MsgId first, MsgId last, int limit, int additional, int type, int flags)
{
    QVariantList backlog;
    auto msgList = Core::requestMsgsFiltered(coreSession()->user(), bufferId, first, last, limit, Message::Types{type}, Message::Flags{flags});

    std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
        return QVariant::fromValue(msg);
    });

    if (additional && limit != 0) {
        MsgId oldestMessage = first;
        if (!msgList.empty()) {
            if (msgList.front().msgId() < msgList.back().msgId())
                oldestMessage = msgList.front().msgId();
            else
                oldestMessage = msgList.back().msgId();
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
            msgList = Core::requestMsgsFiltered(coreSession()->user(), bufferId, -1, last, additional, Message::Types{type}, Message::Flags{flags});
            std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
                return QVariant::fromValue(msg);
            });
        }
    }

    return backlog;
}

QVariantList CoreBacklogManager::requestBacklogForward(BufferId bufferId, MsgId first, MsgId last, int limit, int type, int flags)
{
    QVariantList backlog;
    auto msgList = Core::requestMsgsForward(coreSession()->user(), bufferId, first, last, limit, Message::Types{type}, Message::Flags{flags});

    std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
        return QVariant::fromValue(msg);
    });

    return backlog;
}

QVariantList CoreBacklogManager::requestBacklogAll(MsgId first, MsgId last, int limit, int additional)
{
    QVariantList backlog;
    auto msgList = Core::requestAllMsgs(coreSession()->user(), first, last, limit);

    std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
        return QVariant::fromValue(msg);
    });

    if (additional) {
        if (first != -1) {
            last = first;
        }
        else {
            last = -1;
            if (!msgList.empty()) {
                if (msgList.front().msgId() < msgList.back().msgId())
                    last = msgList.front().msgId();
                else
                    last = msgList.back().msgId();
            }
        }
        msgList = Core::requestAllMsgs(coreSession()->user(), -1, last, additional);
        std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
            return QVariant::fromValue(msg);
        });
    }

    return backlog;
}

QVariantList CoreBacklogManager::requestBacklogAllFiltered(MsgId first, MsgId last, int limit, int additional, int type, int flags)
{
    QVariantList backlog;
    auto msgList = Core::requestAllMsgsFiltered(coreSession()->user(), first, last, limit, Message::Types{type}, Message::Flags{flags});

    std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
        return QVariant::fromValue(msg);
    });

    if (additional) {
        if (first != -1) {
            last = first;
        }
        else {
            last = -1;
            if (!msgList.empty()) {
                if (msgList.front().msgId() < msgList.back().msgId())
                    last = msgList.front().msgId();
                else
                    last = msgList.back().msgId();
            }
        }
        msgList = Core::requestAllMsgsFiltered(coreSession()->user(), -1, last, additional, Message::Types{type}, Message::Flags{flags});
        std::transform(msgList.cbegin(), msgList.cend(), std::back_inserter(backlog), [](auto&& msg) {
            return QVariant::fromValue(msg);
        });
    }

    return backlog;
}

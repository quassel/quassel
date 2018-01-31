/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "awaylogfilter.h"
#include "clientignorelistmanager.h"

AwayLogFilter::AwayLogFilter(MessageModel *model, QObject *parent)
    : ChatMonitorFilter(model, parent)
{
}


bool AwayLogFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    QModelIndex source_index = sourceModel()->index(sourceRow, 0);

    Message::Flags flags = (Message::Flags)sourceModel()->data(source_index, MessageModel::FlagsRole).toInt();
    // Only show highlights from the backlog
    if (!(flags & Message::Backlog && flags & Message::Highlight)) {
        return false;
    }

    // do not use invalid buffers
    BufferId bufferId = sourceModel()->data(source_index, MessageModel::BufferIdRole).value<BufferId>();
    if (!bufferId.isValid()) {
        return false;
    }

    // only show messages that were sent after the user detached
    if (Client::networkModel()->lastSeenMsgId(bufferId) >= sourceModel()->data(source_index, MessageModel::MsgIdRole).value<MsgId>()) {
        return false;
    }

    // ignorelist handling
    // only match if message is not flagged as server msg
    if (!(flags & Message::ServerMsg) && Client::ignoreListManager()
            && Client::ignoreListManager()->match(source_index.data(MessageModel::MessageRole).value<Message>(), Client::networkModel()->networkName(bufferId))) {
        return false;
    }

    return true;
}


QVariant AwayLogFilter::data(const QModelIndex &index, int role) const
{
    if (role != MessageModel::FlagsRole)
        return ChatMonitorFilter::data(index, role);

    QModelIndex source_index = mapToSource(index);
    Message::Flags flags = (Message::Flags)sourceModel()->data(source_index, MessageModel::FlagsRole).toInt();
    flags &= ~Message::Highlight;
    return (int)flags;
}

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

#include "clientirclisthelper.h"

#include <QStringList>

#include "client.h"
#include "irclistmodel.h"

INIT_SYNCABLE_OBJECT(ClientIrcListHelper)
QVariantList ClientIrcListHelper::requestChannelList(const NetworkId &netId, const QStringList &channelFilters)
{
    _netId = netId;
    return IrcListHelper::requestChannelList(netId, channelFilters);
}


void ClientIrcListHelper::receiveChannelList(const NetworkId &netId, const QStringList &channelFilters, const QVariantList &channels)
{
    QVariantList::const_iterator iter = channels.constBegin();
    QVariantList::const_iterator iterEnd = channels.constEnd();

    QList<ChannelDescription> channelList;
    while (iter != iterEnd) {
        QVariantList channelVar = iter->toList();
        ChannelDescription channelDescription(channelVar[0].toString(), channelVar[1].toUInt(), channelVar[2].toString());
        channelList << channelDescription;
        iter++;
    }

    emit channelListReceived(netId, channelFilters, channelList);
}


void ClientIrcListHelper::reportFinishedList(const NetworkId &netId)
{
    if (_netId == netId) {
        requestChannelList(netId, QStringList());
        emit finishedListReported(netId);
    }
}

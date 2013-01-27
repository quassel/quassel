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

#include "coreirclisthelper.h"

#include "corenetwork.h"
#include "coreuserinputhandler.h"

INIT_SYNCABLE_OBJECT(CoreIrcListHelper)
QVariantList CoreIrcListHelper::requestChannelList(const NetworkId &netId, const QStringList &channelFilters)
{
    if (_finishedChannelLists.contains(netId))
        return _finishedChannelLists.take(netId);

    if (_channelLists.contains(netId)) {
        _queuedQuery[netId] = channelFilters.join(",");
    }
    else {
        dispatchQuery(netId, channelFilters.join(","));
    }
    return QVariantList();
}


bool CoreIrcListHelper::addChannel(const NetworkId &netId, const QString &channelName, quint32 userCount, const QString &topic)
{
    if (!_channelLists.contains(netId))
        return false;

    _channelLists[netId] << ChannelDescription(channelName, userCount, topic);
    return true;
}


bool CoreIrcListHelper::dispatchQuery(const NetworkId &netId, const QString &query)
{
    CoreNetwork *network = coreSession()->network(netId);
    if (network) {
        _channelLists[netId] = QList<ChannelDescription>();
        network->userInputHandler()->handleList(BufferInfo(), query);
        _queryTimeout[startTimer(10000)] = netId;
        return true;
    }
    else {
        return false;
    }
}


bool CoreIrcListHelper::endOfChannelList(const NetworkId &netId)
{
    if (_queuedQuery.contains(netId)) {
        // we're no longer interessted in the current data. drop it and issue a new request.
        return dispatchQuery(netId, _queuedQuery.take(netId));
    }
    else if (_channelLists.contains(netId)) {
        QVariantList channelList;
        foreach(ChannelDescription channel, _channelLists[netId]) {
            QVariantList channelVariant;
            channelVariant << channel.channelName
                           << channel.userCount
                           << channel.topic;
            channelList << qVariantFromValue<QVariant>(channelVariant);
        }
        _finishedChannelLists[netId] = channelList;
        _channelLists.remove(netId);
        reportFinishedList(netId);
        return true;
    }
    else {
        return false;
    }
}


void CoreIrcListHelper::timerEvent(QTimerEvent *event)
{
    int timerId = event->timerId();
    killTimer(timerId);
    NetworkId netId = _queryTimeout.take(timerId);
    endOfChannelList(netId);
}

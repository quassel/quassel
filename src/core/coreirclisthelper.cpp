/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

constexpr auto kTimeoutMs = 5000;

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
    if (_queryTimeoutByNetId.contains(netId))
        _queryTimeoutByNetId[netId]->start(kTimeoutMs, this);

    return true;
}


bool CoreIrcListHelper::dispatchQuery(const NetworkId &netId, const QString &query)
{
    CoreNetwork *network = coreSession()->network(netId);
    if (network) {
        _channelLists[netId] = QList<ChannelDescription>();
        network->userInputHandler()->handleList(BufferInfo(), query);

        auto timer = std::make_shared<QBasicTimer>();
        timer->start(kTimeoutMs, this);
        _queryTimeoutByNetId[netId] = timer;
        _queryTimeoutByTimerId[timer->timerId()] = netId;

        return true;
    }
    else {
        return false;
    }
}


bool CoreIrcListHelper::endOfChannelList(const NetworkId &netId)
{
    if (_queryTimeoutByNetId.contains(netId)) {
        // If we recieved an actual RPL_LISTEND, remove the timer
        int timerId = _queryTimeoutByNetId.take(netId)->timerId();
        _queryTimeoutByTimerId.remove(timerId);
    }

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
    if (!_queryTimeoutByTimerId.contains(event->timerId())) {
        IrcListHelper::timerEvent(event);
        return;
    }

    NetworkId netId = _queryTimeoutByTimerId.take(event->timerId());
    _queryTimeoutByNetId.remove(netId);

    event->accept();
    endOfChannelList(netId);
}

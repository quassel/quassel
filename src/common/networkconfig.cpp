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

#include "networkconfig.h"

INIT_SYNCABLE_OBJECT(NetworkConfig)
NetworkConfig::NetworkConfig(const QString &objectName, QObject *parent)
    : SyncableObject(objectName, parent),
    _pingTimeoutEnabled(true),
    _pingInterval(30),
    _maxPingCount(6),
    _autoWhoEnabled(true),
    _autoWhoInterval(90),
    _autoWhoNickLimit(200),
    _autoWhoDelay(5),
    _standardCtcp(false)
{
}


void NetworkConfig::setPingTimeoutEnabled(bool enabled)
{
    if (_pingTimeoutEnabled == enabled)
        return;

    _pingTimeoutEnabled = enabled;
    SYNC(ARG(enabled))
    emit pingTimeoutEnabledSet(enabled);
}


void NetworkConfig::setPingInterval(int interval)
{
    if (_pingInterval == interval)
        return;

    _pingInterval = interval;
    SYNC(ARG(interval))
    emit pingIntervalSet(interval);
}


void NetworkConfig::setMaxPingCount(int count)
{
    if (_maxPingCount == count)
        return;

    _maxPingCount = count;
    SYNC(ARG(count))
}


void NetworkConfig::setAutoWhoEnabled(bool enabled)
{
    if (_autoWhoEnabled == enabled)
        return;

    _autoWhoEnabled = enabled;
    SYNC(ARG(enabled))
    emit autoWhoEnabledSet(enabled);
}


void NetworkConfig::setAutoWhoInterval(int interval)
{
    if (_autoWhoInterval == interval)
        return;

    _autoWhoInterval = interval;
    SYNC(ARG(interval))
    emit autoWhoIntervalSet(interval);
}


void NetworkConfig::setAutoWhoNickLimit(int nickLimit)
{
    if (_autoWhoNickLimit == nickLimit)
        return;

    _autoWhoNickLimit = nickLimit;
    SYNC(ARG(nickLimit))
}


void NetworkConfig::setAutoWhoDelay(int delay)
{
    if (_autoWhoDelay == delay)
        return;

    _autoWhoDelay = delay;
    SYNC(ARG(delay))
    emit autoWhoDelaySet(delay);
}


void NetworkConfig::setStandardCtcp(bool enabled)
{
    if (_standardCtcp == enabled)
        return;

    _standardCtcp = enabled;
    SYNC(ARG(enabled))
    emit standardCtcpSet(enabled);
}

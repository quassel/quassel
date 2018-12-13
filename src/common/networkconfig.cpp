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

#include "networkconfig.h"

NetworkConfig::NetworkConfig(const QString& objectName, QObject* parent)
    : SyncableObject(objectName, parent)
{}

void NetworkConfig::setPingTimeoutEnabled(bool enabled)
{
    if (_pingTimeoutEnabled != enabled) {
        _pingTimeoutEnabled = enabled;
        emit pingTimeoutEnabledSet(enabled);
    }
}

void NetworkConfig::setPingInterval(int interval)
{
    if (_pingInterval != interval) {
        _pingInterval = interval;
        emit pingIntervalSet(interval);
    }
}

void NetworkConfig::setMaxPingCount(int count)
{
    if (_maxPingCount != count) {
        _maxPingCount = count;
        emit maxPingCountSet(count);
    }
}

void NetworkConfig::setAutoWhoEnabled(bool enabled)
{
    if (_autoWhoEnabled != enabled) {
        _autoWhoEnabled = enabled;
        emit autoWhoEnabledSet(enabled);
    }
}

void NetworkConfig::setAutoWhoInterval(int interval)
{
    if (_autoWhoInterval != interval) {
        _autoWhoInterval = interval;
        emit autoWhoIntervalSet(interval);
    }
}

void NetworkConfig::setAutoWhoNickLimit(int nickLimit)
{
    if (_autoWhoNickLimit != nickLimit) {
        _autoWhoNickLimit = nickLimit;
        emit autoWhoNickLimitSet(nickLimit);
    }
}

void NetworkConfig::setAutoWhoDelay(int delay)
{
    if (_autoWhoDelay != delay) {
        _autoWhoDelay = delay;
        emit autoWhoDelaySet(delay);
    }
}

void NetworkConfig::setStandardCtcp(bool enabled)
{
    if (_standardCtcp != enabled) {
        _standardCtcp = enabled;
        emit standardCtcpSet(enabled);
    }
}

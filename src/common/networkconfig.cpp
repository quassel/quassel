/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "networkconfig.h"

NetworkConfig::NetworkConfig(const QString &objectName, QObject *parent)
: SyncableObject(objectName, parent),
  _pingTimeoutEnabled(true),
  _pingInterval(30),
  _maxPingCount(6),
  _autoWhoEnabled(true),
  _autoWhoInterval(90),
  _autoWhoNickLimit(200),
  _autoWhoDelay(5)
{

}

void NetworkConfig::setPingTimeoutEnabled(bool enabled) {
  if(_pingTimeoutEnabled == enabled)
    return;

  _pingTimeoutEnabled = enabled;
  emit pingTimeoutEnabledSet(enabled);
}

void NetworkConfig::setPingInterval(int interval) {
  if(_pingInterval == interval)
    return;

  _pingInterval = interval;
  emit pingIntervalSet(interval);
}

void NetworkConfig::setMaxPingCount(int count) {
  if(_maxPingCount == count)
    return;

  _maxPingCount = count;
  emit maxPingCountSet(count);
}

void NetworkConfig::setAutoWhoEnabled(bool enabled) {
  if(_autoWhoEnabled == enabled)
    return;

  _autoWhoEnabled = enabled;
  emit autoWhoEnabledSet(enabled);
}

void NetworkConfig::setAutoWhoInterval(int interval) {
  if(_autoWhoInterval == interval)
    return;

  _autoWhoInterval = interval;
  emit autoWhoIntervalSet(interval);
}

void NetworkConfig::setAutoWhoNickLimit(int nickLimit) {
  if(_autoWhoNickLimit == nickLimit)
    return;

  _autoWhoNickLimit = nickLimit;
  emit autoWhoNickLimitSet(nickLimit);
}

void NetworkConfig::setAutoWhoDelay(int delay) {
  if(_autoWhoDelay == delay)
    return;

  _autoWhoDelay = delay;
  emit autoWhoDelaySet(delay);
}

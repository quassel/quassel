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

#pragma once

#include "networkconfig.h"

class CoreSession;

class CoreNetworkConfig : public NetworkConfig
{
    Q_OBJECT

public:
    CoreNetworkConfig(const QString &objectName, CoreSession *parent);

    void save();

public slots:
    inline void requestSetPingTimeoutEnabled(bool enabled) override { setPingTimeoutEnabled(enabled); }
    inline void requestSetPingInterval(int interval) override { setPingInterval(interval); }
    inline void requestSetMaxPingCount(int count) override { setMaxPingCount(count); }
    inline void requestSetAutoWhoEnabled(bool enabled) override { setAutoWhoEnabled(enabled); }
    inline void requestSetAutoWhoInterval(int interval) override { setAutoWhoInterval(interval); }
    inline void requestSetAutoWhoNickLimit(int nickLimit) override { setAutoWhoNickLimit(nickLimit); }
    inline void requestSetAutoWhoDelay(int delay) override { setAutoWhoDelay(delay); }
    inline void requestSetStandardCtcp(bool enabled) override { setStandardCtcp(enabled); }
};

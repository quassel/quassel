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

#ifndef CORENETWORKCONFIG_H_
#define CORENETWORKCONFIG_H_

#include "networkconfig.h"

class CoreSession;

class CoreNetworkConfig : public NetworkConfig
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreNetworkConfig(const QString &objectName, CoreSession *parent);

    inline virtual const QMetaObject *syncMetaObject() const { return &NetworkConfig::staticMetaObject; }

    void save();

public slots:
    virtual inline void requestSetPingTimeoutEnabled(bool enabled) { setPingTimeoutEnabled(enabled); }
    virtual inline void requestSetPingInterval(int interval) { setPingInterval(interval); }
    virtual inline void requestSetMaxPingCount(int count) { setMaxPingCount(count); }
    virtual inline void requestSetAutoWhoEnabled(bool enabled) { setAutoWhoEnabled(enabled); }
    virtual inline void requestSetAutoWhoInterval(int interval) { setAutoWhoInterval(interval); }
    virtual inline void requestSetAutoWhoNickLimit(int nickLimit) { setAutoWhoNickLimit(nickLimit); }
    virtual inline void requestSetAutoWhoDelay(int delay) { setAutoWhoDelay(delay); }
    virtual inline void requestSetStandardCtcp(bool enabled) { setStandardCtcp(enabled); }
};


#endif

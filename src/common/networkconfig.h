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

#ifndef NETWORKCONFIG_H_
#define NETWORKCONFIG_H_

#include "syncableobject.h"

class NetworkConfig : public SyncableObject
{
    SYNCABLE_OBJECT
    Q_OBJECT

    Q_PROPERTY(bool pingTimeoutEnabled READ pingTimeoutEnabled WRITE setPingTimeoutEnabled)
    Q_PROPERTY(int pingInterval READ pingInterval WRITE setPingInterval)
    Q_PROPERTY(int maxPingCount READ maxPingCount WRITE setMaxPingCount)
    Q_PROPERTY(bool autoWhoEnabled READ autoWhoEnabled WRITE setAutoWhoEnabled)
    Q_PROPERTY(int autoWhoInterval READ autoWhoInterval WRITE setAutoWhoInterval)
    Q_PROPERTY(int autoWhoNickLimit READ autoWhoNickLimit WRITE setAutoWhoNickLimit)
    Q_PROPERTY(int autoWhoDelay READ autoWhoDelay WRITE setAutoWhoDelay)
    Q_PROPERTY(bool standardCtcp READ standardCtcp WRITE setStandardCtcp)

public :
        NetworkConfig(const QString &objectName = "GlobalNetworkConfig", QObject *parent = 0);

    inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

public slots:
    inline bool pingTimeoutEnabled() const { return _pingTimeoutEnabled; }
    void setPingTimeoutEnabled(bool);
    virtual inline void requestSetPingTimeoutEnabled(bool b) { REQUEST(ARG(b)) }

    inline int pingInterval() const { return _pingInterval; }
    void setPingInterval(int);
    virtual inline void requestSetPingInterval(int i) { REQUEST(ARG(i)) }

    inline int maxPingCount() const { return _maxPingCount; }
    void setMaxPingCount(int);
    virtual inline void requestSetMaxPingCount(int i) { REQUEST(ARG(i)) }

    inline bool autoWhoEnabled() const { return _autoWhoEnabled; }
    void setAutoWhoEnabled(bool);
    virtual inline void requestSetAutoWhoEnabled(bool b) { REQUEST(ARG(b)) }

    inline int autoWhoInterval() const { return _autoWhoInterval; }
    void setAutoWhoInterval(int);
    virtual inline void requestSetAutoWhoInterval(int i) { REQUEST(ARG(i)) }

    inline int autoWhoNickLimit() const { return _autoWhoNickLimit; }
    void setAutoWhoNickLimit(int);
    virtual inline void requestSetAutoWhoNickLimit(int i) { REQUEST(ARG(i)) }

    inline int autoWhoDelay() const { return _autoWhoDelay; }
    void setAutoWhoDelay(int);
    virtual inline void requestSetAutoWhoDelay(int i) { REQUEST(ARG(i)) }

    inline bool standardCtcp() const { return _standardCtcp; }
    void setStandardCtcp(bool);
    virtual inline void requestSetStandardCtcp(bool b) { REQUEST(ARG(b)) }

signals:
    void pingTimeoutEnabledSet(bool);
    void pingIntervalSet(int);
//   void maxPingCountSet(int);
    void autoWhoEnabledSet(bool);
    void autoWhoIntervalSet(int);
//   void autoWhoNickLimitSet(int);
    void autoWhoDelaySet(int);
    void standardCtcpSet(bool);

//   void setPingTimeoutEnabledRequested(bool);
//   void setPingIntervalRequested(int);
//   void setMaxPingCountRequested(int);
//   void setAutoWhoEnabledRequested(bool);
//   void setAutoWhoIntervalRequested(int);
//   void setAutoWhoNickLimitRequested(int);
//   void setAutoWhoDelayRequested(int);

private:
    bool _pingTimeoutEnabled;
    int _pingInterval;
    int _maxPingCount;

    bool _autoWhoEnabled;
    int _autoWhoInterval;
    int _autoWhoNickLimit;
    int _autoWhoDelay;

    bool _standardCtcp;
};


#endif

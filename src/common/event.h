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

#ifndef EVENT_H
#define EVENT_H

#include <QDateTime>
#include <QDebug>

#include "eventmanager.h"

class Network;

class Event
{
public:
    explicit Event(EventManager::EventType type = EventManager::Invalid);
    virtual ~Event() {}

    inline EventManager::EventType type() const { return _type; }

    inline void setFlag(EventManager::EventFlag flag) { _flags |= flag; }
    inline void setFlags(EventManager::EventFlags flags) { _flags = flags; }
    inline bool testFlag(EventManager::EventFlag flag) { return _flags.testFlag(flag); }
    inline EventManager::EventFlags flags() const { return _flags; }

    inline bool isValid() const { return _valid; }
    inline void stop() { setFlag(EventManager::Stopped); }
    inline bool isStopped() { return _flags.testFlag(EventManager::Stopped); }

    inline void setTimestamp(const QDateTime &time) { _timestamp = time; }
    inline QDateTime timestamp() const { return _timestamp; }

    //inline void setData(const QVariant &data) { _data = data; }
    //inline QVariant data() const { return _data; }

    // call EventManager::createEvent(map) instead!
    static Event *fromVariantMap(QVariantMap &map, Network *network);
    QVariantMap toVariantMap() const;

protected:
    virtual inline QString className() const { return "Event"; }
    virtual inline void debugInfo(QDebug &dbg) const { Q_UNUSED(dbg); }

    explicit Event(EventManager::EventType type, QVariantMap &map);

    // must only use primitive types: string, int, double, list, hash
    // we want to convert this to JSON in the future!
    virtual void toVariantMap(QVariantMap &map) const;

    inline void setValid(bool valid) { _valid = valid; }

private:
    EventManager::EventType _type;
    EventManager::EventFlags _flags;
    QDateTime _timestamp;
    //QVariant _data;
    bool _valid;

    friend QDebug operator<<(QDebug dbg, Event *e);
};


QDebug operator<<(QDebug dbg, Event *e);

/*******/

#endif

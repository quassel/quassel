/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <QDateTime>
#include <QDebug>
#include <QObject>

#include "eventmanager.h"

class Network;

class COMMON_EXPORT Event : public QObject
{
    Q_OBJECT
public:
    explicit Event(EventManager::EventType type = EventManager::Invalid, QObject* parent = nullptr);
    virtual ~Event() override = default;

    inline EventManager::EventType type() const { return _type; }

    inline void setFlag(EventManager::EventFlag flag) { _flags |= flag; }
    inline void setFlags(EventManager::EventFlags flags) { _flags = flags; }
    inline bool testFlag(EventManager::EventFlag flag) { return _flags.testFlag(flag); }
    inline EventManager::EventFlags flags() const { return _flags; }

    inline bool isValid() const { return _valid; }
    inline void stop() { setFlag(EventManager::Stopped); }
    inline bool isStopped() { return _flags.testFlag(EventManager::Stopped); }

    inline void setTimestamp(const QDateTime& time) { _timestamp = time; }
    inline QDateTime timestamp() const { return _timestamp; }

    static Event* fromVariantMap(QVariantMap& map, Network* network);
    QVariantMap toVariantMap() const;

protected:
    virtual inline QString className() const { return "Event"; }
    virtual inline void debugInfo(QDebug& dbg) const { Q_UNUSED(dbg); }

    explicit Event(EventManager::EventType type, QVariantMap& map, QObject* parent = nullptr);

    virtual void toVariantMap(QVariantMap& map) const;

    inline void setValid(bool valid) { _valid = valid; }

private:
    EventManager::EventType _type;
    EventManager::EventFlags _flags;
    QDateTime _timestamp;
    bool _valid{true};

    friend COMMON_EXPORT QDebug operator<<(QDebug dbg, Event* e);
};

COMMON_EXPORT QDebug operator<<(QDebug dbg, Event* e);

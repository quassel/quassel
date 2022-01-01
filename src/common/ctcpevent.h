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

#include "common-export.h"

#include <utility>

#include <QUuid>

#include "ircevent.h"

class COMMON_EXPORT CtcpEvent : public IrcEvent
{
public:
    enum CtcpType
    {
        Query,
        Reply
    };

    explicit CtcpEvent(EventManager::EventType type,
                       Network* network,
                       QHash<IrcTagKey, QString> tags,
                       QString prefix,
                       QString target,
                       CtcpType ctcpType,
                       QString ctcpCmd,
                       QString param,
                       const QDateTime& timestamp = QDateTime(),
                       const QUuid& uuid = QUuid())
        : IrcEvent(type, network, std::move(tags), std::move(prefix))
        , _ctcpType(ctcpType)
        , _ctcpCmd(std::move(ctcpCmd))
        , _target(std::move(target))
        , _param(std::move(param))
        , _uuid(uuid)
    {
        setTimestamp(timestamp);
    }

    inline CtcpType ctcpType() const { return _ctcpType; }
    inline void setCtcpType(CtcpType type) { _ctcpType = type; }

    inline QString ctcpCmd() const { return _ctcpCmd; }
    inline void setCtcpCmd(const QString& ctcpCmd) { _ctcpCmd = ctcpCmd; }

    inline QString target() const { return _target; }
    inline void setTarget(const QString& target) { _target = target; }

    inline QString param() const { return _param; }
    inline void setParam(const QString& param) { _param = param; }

    inline QString reply() const { return _reply; }
    inline void setReply(const QString& reply) { _reply = reply; }

    inline QUuid uuid() const { return _uuid; }
    inline void setUuid(const QUuid& uuid) { _uuid = uuid; }

    static Event* create(EventManager::EventType type, QVariantMap& map, Network* network);

protected:
    explicit CtcpEvent(EventManager::EventType type, QVariantMap& map, Network* network);
    void toVariantMap(QVariantMap& map) const override;

    inline QString className() const override { return "CtcpEvent"; }
    inline void debugInfo(QDebug& dbg) const override
    {
        NetworkEvent::debugInfo(dbg);
        dbg << ", prefix = " << qPrintable(prefix())
            << ", target = " << qPrintable(target())
            << ", ctcptype = " << (ctcpType() == Query ? "query" : "reply")
            << ", cmd = " << qPrintable(ctcpCmd())
            << ", param = " << qPrintable(param())
            << ", reply = " << qPrintable(reply());
    }

private:
    CtcpType _ctcpType;
    QString _ctcpCmd;
    QString _target, _param, _reply;
    QUuid _uuid;
};

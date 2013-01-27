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

#ifndef IRCEVENT_H
#define IRCEVENT_H

#include "networkevent.h"
#include "util.h"

class IrcEvent : public NetworkEvent
{
public:
    explicit IrcEvent(EventManager::EventType type, Network *network, const QString &prefix, const QStringList &params = QStringList())
        : NetworkEvent(type, network),
        _prefix(prefix),
        _params(params)
    {}

    inline QString prefix() const { return _prefix; }
    inline void setPrefix(const QString &prefix) { _prefix = prefix; }

    inline QString nick() const { return nickFromMask(prefix()); }

    inline QStringList params() const { return _params; }
    inline void setParams(const QStringList &params) { _params = params; }

    static Event *create(EventManager::EventType type, QVariantMap &map, Network *network);

protected:
    explicit IrcEvent(EventManager::EventType type, QVariantMap &map, Network *network);
    void toVariantMap(QVariantMap &map) const;

    virtual inline QString className() const { return "IrcEvent"; }
    virtual inline void debugInfo(QDebug &dbg) const
    {
        NetworkEvent::debugInfo(dbg);
        dbg << ", prefix = " << qPrintable(prefix())
            << ", params = " << params();
    }


private:
    QString _prefix;
    QStringList _params;
};


class IrcEventNumeric : public IrcEvent
{
public:
    explicit IrcEventNumeric(uint number, Network *network, const QString &prefix, const QString &target, const QStringList &params = QStringList())
        : IrcEvent(EventManager::IrcEventNumeric, network, prefix, params),
        _number(number),
        _target(target)
    {}

    inline uint number() const { return _number; }

    inline QString target() const { return _target; }
    inline void setTarget(const QString &target) { _target = target; }

protected:
    explicit IrcEventNumeric(EventManager::EventType type, QVariantMap &map, Network *network);
    void toVariantMap(QVariantMap &map) const;

    virtual inline QString className() const { return "IrcEventNumeric"; }
    virtual inline void debugInfo(QDebug &dbg) const
    {
        dbg << ", num = " << number();
        NetworkEvent::debugInfo(dbg);
        dbg << ", target = " << qPrintable(target())
            << ", prefix = " << qPrintable(prefix())
            << ", params = " << params();
    }


private:
    uint _number;
    QString _target;

    friend class IrcEvent;
};


class IrcEventRawMessage : public IrcEvent
{
public:
    explicit inline IrcEventRawMessage(EventManager::EventType type, Network *network,
        const QByteArray &rawMessage, const QString &prefix, const QString &target,
        const QDateTime &timestamp = QDateTime())
        : IrcEvent(type, network, prefix, QStringList() << target),
        _rawMessage(rawMessage)
    {
        setTimestamp(timestamp);
    }


    inline QString target() const { return params().at(0); }
    inline void setTarget(const QString &target) { setParams(QStringList() << target); }

    inline QByteArray rawMessage() const { return _rawMessage; }
    inline void setRawMessage(const QByteArray &rawMessage) { _rawMessage = rawMessage; }

protected:
    explicit IrcEventRawMessage(EventManager::EventType type, QVariantMap &map, Network *network);
    void toVariantMap(QVariantMap &map) const;

    virtual inline QString className() const { return "IrcEventRawMessage"; }
    virtual inline void debugInfo(QDebug &dbg) const
    {
        NetworkEvent::debugInfo(dbg);
        dbg << ", target = " << qPrintable(target())
            << ", prefix = " << qPrintable(prefix())
            << ", msg = " << rawMessage();
    }


private:
    QByteArray _rawMessage;

    friend class IrcEvent;
};


#endif

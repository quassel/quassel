/***************************************************************************
 *   Copyright (C) 2013 by the Quassel Project                             *
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

#ifndef KEYEVENT_H
#define KEYEVENT_H

#include "ircevent.h"

class KeyEvent : public IrcEvent
{
public:
    enum ExchangeType {
        Init,
        Finish
    };

    explicit KeyEvent(EventManager::EventType type, Network *network, const QString &prefix, const QString &target,
        ExchangeType exchangeType, const QByteArray &key,
        const QDateTime &timestamp = QDateTime())
        : IrcEvent(type, network, prefix),
        _exchangeType(exchangeType),
        _target(target),
        _key(key)
    {
        setTimestamp(timestamp);
    }


    inline ExchangeType exchangeType() const { return _exchangeType; }
    inline void setExchangeType(ExchangeType type) { _exchangeType = type; }

    inline QString target() const { return _target; }
    inline void setTarget(const QString &target) { _target = target; }

    inline QByteArray key() const { return _key; }
    inline void setKey(const QByteArray &key) { _key = key; }

    static Event *create(EventManager::EventType type, QVariantMap &map, Network *network);

protected:
    explicit KeyEvent(EventManager::EventType type, QVariantMap &map, Network *network);
    void toVariantMap(QVariantMap &map) const;

    virtual inline QString className() const { return "KeyEvent"; }
    virtual inline void debugInfo(QDebug &dbg) const
    {
        NetworkEvent::debugInfo(dbg);
        dbg << ", prefix = " << qPrintable(prefix())
            << ", target = " << qPrintable(target())
            << ", exchangetype = " << (exchangeType() == Init ? "init" : "finish")
            << ", key = " << key();
    }


private:
    ExchangeType _exchangeType;
    QString _target;
    QByteArray _key;
};


#endif

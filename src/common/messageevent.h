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

#ifndef MESSAGEEVENT_H
#define MESSAGEEVENT_H

#include "message.h"
#include "networkevent.h"

// this corresponds to CoreSession::RawMessage for now and should contain the information we need to convert events
// into messages for the legacy code to work with

class MessageEvent : public NetworkEvent
{
public:
    explicit MessageEvent(Message::Type msgType,
        Network *network,
        const QString &msg,
        const QString &sender = QString(),
        const QString &target = QString(),
        Message::Flags msgFlags = Message::None,
        const QDateTime &timestamp = QDateTime()
        );

    inline Message::Type msgType() const { return _msgType; }
    inline void setMsgType(Message::Type type) { _msgType = type; }

    inline BufferInfo::Type bufferType() const { return _bufferType; }
    inline void setBufferType(BufferInfo::Type type) { _bufferType = type; }

    inline QString target() const { return _target; }
    inline QString text() const { return _text; }
    inline QString sender() const { return _sender; }

    inline Message::Flags msgFlags() const { return _msgFlags; }
    inline void setMsgFlag(Message::Flag flag) { _msgFlags |= flag; }
    inline void setMsgFlags(Message::Flags flags) { _msgFlags = flags; }

    static Event *create(EventManager::EventType type, QVariantMap &map, Network *network);

protected:
    explicit MessageEvent(EventManager::EventType type, QVariantMap &map, Network *network);
    void toVariantMap(QVariantMap &map) const;

    virtual inline QString className() const { return "MessageEvent"; }
    virtual inline void debugInfo(QDebug &dbg) const
    {
        NetworkEvent::debugInfo(dbg);
        dbg.nospace() << ", sender = " << qPrintable(sender())
                      << ", target = " << qPrintable(target())
                      << ", text = " << text()
                      << ", msgtype = " << qPrintable(QString::number(msgType(), 16))
                      << ", buffertype = " << qPrintable(QString::number(bufferType(), 16))
                      << ", msgflags = " << qPrintable(QString::number(msgFlags(), 16));
    }


private:
    BufferInfo::Type bufferTypeByTarget(const QString &target) const;

    Message::Type _msgType;
    BufferInfo::Type _bufferType;
    QString _text, _sender, _target;
    Message::Flags _msgFlags;
};


#endif

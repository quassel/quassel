/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "message.h"

#include "util.h"
#include "peer.h"
#include "signalproxy.h"

#include <QDataStream>

Message::Message(const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, const QString &senderPrefixes, Flags flags)
    : _timestamp(QDateTime::currentDateTime().toUTC()),
    _bufferInfo(bufferInfo),
    _contents(contents),
    _sender(sender),
    _senderPrefixes(senderPrefixes),
    _type(type),
    _flags(flags)
{
}


Message::Message(const QDateTime &ts, const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, const QString &senderPrefixes, Flags flags)
    : _timestamp(ts),
    _bufferInfo(bufferInfo),
    _contents(contents),
    _sender(sender),
    _senderPrefixes(senderPrefixes),
    _type(type),
    _flags(flags)
{
}


QDataStream &operator<<(QDataStream &out, const Message &msg)
{
    // We do not serialize the sender prefixes until we have a new protocol or client-features implemented
    out << msg.msgId()
        << (quint32) msg.timestamp().toTime_t()
        << (quint32) msg.type()
        << (quint8) msg.flags()
        << msg.bufferInfo()
        << msg.sender().toUtf8();

    if (SignalProxy::current()->targetPeer()->hasFeature(Quassel::Feature::SenderPrefixes))
        out << msg.senderPrefixes().toUtf8();

    out << msg.contents().toUtf8();
    return out;
}


QDataStream &operator>>(QDataStream &in, Message &msg)
{
    in >> msg._msgId;

    quint32 timeStamp;
    in >> timeStamp;
    msg._timestamp = QDateTime::fromTime_t(timeStamp);

    quint32 type;
    in >> type;
    msg._type = Message::Type(type);

    quint8 flags;
    in >> flags;
    msg._flags = Message::Flags(flags);

    in >> msg._bufferInfo;

    QByteArray sender;
    in >> sender;
    msg._sender = QString::fromUtf8(sender);

    QByteArray senderPrefixes;
    if (SignalProxy::current()->sourcePeer()->hasFeature(Quassel::Feature::SenderPrefixes))
        in >> senderPrefixes;
    msg._senderPrefixes = QString::fromUtf8(senderPrefixes);

    QByteArray contents;
    in >> contents;
    msg._contents = QString::fromUtf8(contents);

    return in;
}


QDebug operator<<(QDebug dbg, const Message &msg)
{
    dbg.nospace() << qPrintable(QString("Message(MsgId:")) << msg.msgId()
    << qPrintable(QString(",")) << msg.timestamp()
    << qPrintable(QString(", Type:")) << msg.type()
    << qPrintable(QString(", Flags:")) << msg.flags() << qPrintable(QString(")"))
    << msg.sender() << ":" << msg.contents();
    return dbg;
}

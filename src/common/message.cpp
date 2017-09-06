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

#include <iostream>
#include "message.h"

#include "signalproxy.h"
#include "peer.h"

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
    out << msg.msgId();
    out << (quint32)msg.timestamp().toTime_t();
    out << (quint32)msg.type();
    out << (quint8)msg.flags();
    out << msg.bufferInfo();
    out << msg.sender().toUtf8();

    if (SignalProxy::current()->_targetPeer->_features.testFlag(Quassel::Feature::ClientFeatures))
        out << msg.senderPrefixes().toUtf8();

    out << msg.contents().toUtf8();
    return out;
}


QDataStream &operator>>(QDataStream &in, Message &msg)
{
    quint8 f;
    quint32 t;
    quint32 ts;
    QByteArray s, p, m;
    BufferInfo buf;
    in >> msg._msgId;
    in >> ts;
    in >> t;
    in >> f;
    in >> buf;
    in >> s;

    // We do not serialize the sender prefixes until we have a new protocol or client-features implemented
    if (SignalProxy::current()->_sourcePeer->_features.testFlag(Quassel::Feature::ClientFeatures))
        in >> p;

    in >> m;
    msg._type = (Message::Type)t;
    msg._flags = (Message::Flags)f;
    msg._bufferInfo = buf;
    msg._timestamp = QDateTime::fromTime_t(ts);
    msg._sender = QString::fromUtf8(s);
    msg._senderPrefixes = QString::fromUtf8(p);
    msg._contents = QString::fromUtf8(m);
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

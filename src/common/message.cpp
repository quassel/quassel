/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include <utility>

#include <QDataStream>

#include "message.h"
#include "peer.h"
#include "signalproxy.h"
#include "util.h"

Message::Message(BufferInfo bufferInfo,
                 Type type,
                 QString contents,
                 QString sender,
                 QString senderPrefixes,
                 QString realName,
                 QString avatarUrl,
                 Flags flags)
    : _timestamp(QDateTime::currentDateTime().toUTC())
    , _bufferInfo(std::move(bufferInfo))
    , _contents(std::move(contents))
    , _sender(std::move(sender))
    , _senderPrefixes(std::move(senderPrefixes))
    , _realName(std::move(realName))
    , _avatarUrl(std::move(avatarUrl))
    , _type(type)
    , _flags(flags)
{}

Message::Message(QDateTime ts,
                 BufferInfo bufferInfo,
                 Type type,
                 QString contents,
                 QString sender,
                 QString senderPrefixes,
                 QString realName,
                 QString avatarUrl,
                 Flags flags)
    : _timestamp(std::move(ts))
    , _bufferInfo(std::move(bufferInfo))
    , _contents(std::move(contents))
    , _sender(std::move(sender))
    , _senderPrefixes(std::move(senderPrefixes))
    , _realName(std::move(realName))
    , _avatarUrl(std::move(avatarUrl))
    , _type(type)
    , _flags(flags)
{}

QDataStream& operator<<(QDataStream& out, const Message& msg)
{
    Q_ASSERT(SignalProxy::current());
    Q_ASSERT(SignalProxy::current()->targetPeer());

    out << msg.msgId();

    if (SignalProxy::current()->targetPeer()->hasFeature(Quassel::Feature::LongTime)) {
        // toMSecs returns a qint64, signed rather than unsigned
        out << (qint64) msg.timestamp().toMSecsSinceEpoch();
    }
    else {
        out << (quint32) msg.timestamp().toTime_t();
    }

    out << (quint32) msg.type()
        << (quint8) msg.flags()
        << msg.bufferInfo()
        << msg.sender().toUtf8();

    if (SignalProxy::current()->targetPeer()->hasFeature(Quassel::Feature::SenderPrefixes))
        out << msg.senderPrefixes().toUtf8();

    if (SignalProxy::current()->targetPeer()->hasFeature(Quassel::Feature::RichMessages)) {
        out << msg.realName().toUtf8();
        out << msg.avatarUrl().toUtf8();
    }

    out << msg.contents().toUtf8();
    return out;
}

QDataStream& operator>>(QDataStream& in, Message& msg)
{
    Q_ASSERT(SignalProxy::current());
    Q_ASSERT(SignalProxy::current()->sourcePeer());

    in >> msg._msgId;

    if (SignalProxy::current()->sourcePeer()->hasFeature(Quassel::Feature::LongTime)) {
        // timestamp is a qint64, signed rather than unsigned
        qint64 timeStamp;
        in >> timeStamp;
        msg._timestamp = QDateTime::fromMSecsSinceEpoch(timeStamp);
    }
    else {
        quint32 timeStamp;
        in >> timeStamp;
        msg._timestamp = QDateTime::fromTime_t(timeStamp);
    }

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

    QByteArray realName;
    QByteArray avatarUrl;
    if (SignalProxy::current()->sourcePeer()->hasFeature(Quassel::Feature::RichMessages)) {
        in >> realName;
        in >> avatarUrl;
    }
    msg._realName = QString::fromUtf8(realName);
    msg._avatarUrl = QString::fromUtf8(avatarUrl);

    QByteArray contents;
    in >> contents;
    msg._contents = QString::fromUtf8(contents);

    return in;
}

QDebug operator<<(QDebug dbg, const Message& msg)
{
    dbg.nospace() << qPrintable(QString("Message(MsgId:")) << msg.msgId()
                  << qPrintable(QString(",")) << msg.timestamp()
                  << qPrintable(QString(", Type:")) << msg.type()
                  << qPrintable(QString(", RealName:")) << msg.realName()
                  << qPrintable(QString(", AvatarURL:")) << msg.avatarUrl()
                  << qPrintable(QString(", Flags:")) << msg.flags()
                  << qPrintable(QString(")"))
                  << msg.senderPrefixes() << msg.sender() << ":"
                  << msg.contents();
    return dbg;
}

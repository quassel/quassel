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

#include "message.h"

#include "util.h"

#include <QDataStream>

Message::Message(const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, Flags flags)
    : _timestamp(QDateTime::currentDateTime().toUTC()),
    _bufferInfo(bufferInfo),
    _contents(contents),
    _sender(sender),
    _type(type),
    _flags(flags)
{
}


Message::Message(const QDateTime &ts, const BufferInfo &bufferInfo, Type type, const QString &contents, const QString &sender, Flags flags)
    : _timestamp(ts),
    _bufferInfo(bufferInfo),
    _contents(contents),
    _sender(sender),
    _type(type),
    _flags(flags)
{
}


QDataStream &operator<<(QDataStream &out, const Message &msg)
{
    out << msg.msgId() << (quint32)msg.timestamp().toTime_t() << (quint32)msg.type() << (quint8)msg.flags()
    << msg.bufferInfo() << msg.sender().toUtf8() << msg.contents().toUtf8();
    return out;
}


QDataStream &operator>>(QDataStream &in, Message &msg)
{
    quint8 f;
    quint32 t;
    quint32 ts;
    QByteArray s, m;
    BufferInfo buf;
    in >> msg._msgId >> ts >> t >> f >> buf >> s >> m;
    msg._type = (Message::Type)t;
    msg._flags = (Message::Flags)f;
    msg._bufferInfo = buf;
    msg._timestamp = QDateTime::fromTime_t(ts);
    msg._sender = QString::fromUtf8(s);
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

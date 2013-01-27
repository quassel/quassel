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

#include <QString>
#include <QDataStream>
#include <QDebug>
#include <QByteArray>

#include "bufferinfo.h"

#include "util.h"

BufferInfo::BufferInfo()
    : _bufferId(0),
    _netid(0),
    _type(InvalidBuffer),
    _groupId(0),
    _bufferName(QString())
{
}


BufferInfo::BufferInfo(BufferId id,  NetworkId networkid, Type type, uint gid, QString buf)
    : _bufferId(id),
    _netid(networkid),
    _type(type),
    _groupId(gid),
    _bufferName(buf)
{
}


BufferInfo BufferInfo::fakeStatusBuffer(NetworkId networkId)
{
    return BufferInfo(0, networkId, StatusBuffer);
}


QString BufferInfo::bufferName() const
{
    if (isChannelName(_bufferName))
        return _bufferName;
    else
        return nickFromMask(_bufferName);  // FIXME get rid of global functions and use the Network stuff instead!
}


QDebug operator<<(QDebug dbg, const BufferInfo &b)
{
    dbg.nospace() << "(bufId: " << b.bufferId() << ", netId: " << b.networkId() << ", groupId: " << b.groupId() << ", buf: " << b.bufferName() << ")";
    return dbg.space();
}


QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo)
{
    out << bufferInfo._bufferId << bufferInfo._netid << (qint16)bufferInfo._type << bufferInfo._groupId << bufferInfo._bufferName.toUtf8();
    return out;
}


QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo)
{
    QByteArray buffername;
    qint16 bufferType;
    in >> bufferInfo._bufferId >> bufferInfo._netid >> bufferType >> bufferInfo._groupId >> buffername;
    bufferInfo._type = (BufferInfo::Type)bufferType;
    bufferInfo._bufferName = QString::fromUtf8(buffername);
    return in;
}


uint qHash(const BufferInfo &bufferid)
{
    return qHash(bufferid._bufferId);
}

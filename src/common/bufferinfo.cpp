/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QString>
#include <QDataStream>
#include <QDebug>
#include <QByteArray>

#include "bufferinfo.h"

#include "util.h"

BufferInfo::BufferInfo()
  : _id(0),
    _netid(0),
    _gid(0),
    _networkName(QString()),
    _bufferName(QString()) {
}

BufferInfo::BufferInfo(uint id, uint networkid, uint gid, QString net, QString buf)
  : _id(id),
    _netid(networkid),
    _gid(gid),
    _networkName(net),
    _bufferName(buf) {
}

QString BufferInfo::buffer() const {
  if(isChannelName(_bufferName))
    return _bufferName;
  else
    return nickFromMask(_bufferName);
}

QDebug operator<<(QDebug dbg, const BufferInfo &b) {
  dbg.nospace() << "(bufId: " << b.uid() << ", netId: " << b.networkId() << ", groupId: " << b.groupId()
                << ", net: " << b.network() << ", buf: " << b.buffer() << ")";

  return dbg.space();
}

QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo) {
  out << bufferInfo._id << bufferInfo._netid << bufferInfo._gid << bufferInfo._networkName.toUtf8() << bufferInfo._bufferName.toUtf8();
  return out;
}

QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo) {
  QByteArray n, b;
  in >> bufferInfo._id >> bufferInfo._netid >> bufferInfo._gid >> n >> b;
  bufferInfo._networkName = QString::fromUtf8(n);
  bufferInfo._bufferName = QString::fromUtf8(b);
  return in;
}

uint qHash(const BufferInfo &bufferid) {
  return qHash(bufferid._id);
}


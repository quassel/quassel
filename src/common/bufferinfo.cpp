/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
  : _bufferId(0),
    _netid(0),
    _groupId(0),
    _bufferName(QString())
{
}

BufferInfo::BufferInfo(BufferId id,  NetworkId networkid, uint gid, QString buf)
  : _bufferId(id),
    _netid(networkid),
    _groupId(gid),
    _bufferName(buf)
{
}

QString BufferInfo::bufferName() const {
  if(isChannelName(_bufferName))
    return _bufferName;
  else
    return nickFromMask(_bufferName);  // FIXME get rid of global functions and use the Network stuff instead!
}

QDebug operator<<(QDebug dbg, const BufferInfo &b) {
  dbg.nospace() << "(bufId: " << b.bufferId() << ", netId: " << b.networkId() << ", groupId: " << b.groupId() << ", buf: " << b.bufferName() << ")";
  return dbg.space();
}

QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo) {
  out << bufferInfo._bufferId << bufferInfo._netid << bufferInfo._groupId << bufferInfo._bufferName.toUtf8();
  return out;
}

QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo) {
  QByteArray buffername;
  in >> bufferInfo._bufferId >> bufferInfo._netid >> bufferInfo._groupId >> buffername;
  bufferInfo._bufferName = QString::fromUtf8(buffername);
  return in;
}

uint qHash(const BufferInfo &bufferid) {
  return qHash(bufferid._bufferId.toInt());
}


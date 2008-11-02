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
#ifndef BUFFERINFO_H
#define BUFFERINFO_H

#include <QtCore>
#include "types.h"

class QString;
class QDataStream;

class BufferInfo {
public:
  enum Type {
    InvalidBuffer = 0x00,
    StatusBuffer = 0x01,
    ChannelBuffer = 0x02,
    QueryBuffer = 0x04,
    GroupBuffer = 0x08
  };
  
  enum Activity {
    NoActivity = 0x00,
    OtherActivity = 0x01,
    NewMessage = 0x02,
    Highlight = 0x40
  };
  Q_DECLARE_FLAGS(ActivityLevel, Activity)

  BufferInfo();
  BufferInfo(BufferId id, NetworkId networkid, Type type, uint gid = 0, QString buf = QString());

  static BufferInfo fakeStatusBuffer(NetworkId networkId);

  inline bool isValid() const { return _bufferId != 0; }
  inline BufferId bufferId() const { return _bufferId; }
  inline NetworkId networkId() const { return _netid; }
  inline Type type() const { return _type; }
  inline uint groupId() const { return _groupId; }
  QString bufferName() const;
  
  void setGroupId(uint gid) { _groupId = gid; }
  
  inline bool operator==(const BufferInfo &other) const { return _bufferId == other._bufferId; }

private:
  BufferId _bufferId;
  NetworkId _netid;
  Type _type;
  uint _groupId;
  QString _bufferName;
  
  friend uint qHash(const BufferInfo &);
  friend QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo);
  friend QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo);
};

QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo);
QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo);
QDebug operator<<(QDebug dbg, const BufferInfo &b);

Q_DECLARE_METATYPE(BufferInfo)
Q_DECLARE_OPERATORS_FOR_FLAGS(BufferInfo::ActivityLevel)

uint qHash(const BufferInfo &);

#endif

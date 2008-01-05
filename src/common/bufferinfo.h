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
#ifndef _BUFFERINFO_H_
#define _BUFFERINFO_H_

#include <QtCore>

class QString;
class QDataStream;

class BufferInfo {
public:
  BufferInfo();
  BufferInfo(uint id, uint networkid, uint gid = 0, QString net = QString(), QString buf = QString());
  
  inline uint uid() const { return _id; }
  inline uint networkId() const { return _netid; }
  inline uint groupId() const { return _gid; }
  inline QString network() const { return _networkName; }
  QString buffer() const;
  
  void setGroupId(uint gid) { _gid = gid; }
  
  inline bool operator==(const BufferInfo &other) const { return _id == other._id; }

private:
  uint _id;
  uint _netid;
  uint _gid;
  QString _networkName; // WILL BE REMOVED
  QString _bufferName;
  
  friend uint qHash(const BufferInfo &);
  friend QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo);
  friend QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo);
};

QDataStream &operator<<(QDataStream &out, const BufferInfo &bufferInfo);
QDataStream &operator>>(QDataStream &in, BufferInfo &bufferInfo);
QDebug operator<<(QDebug dbg, const BufferInfo &b);

Q_DECLARE_METATYPE(BufferInfo);

uint qHash(const BufferInfo &);

#endif

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

#ifndef _TYPES_H_
#define _TYPES_H_

#include <QString>

/*
class UnsignedId {
    quint32 id;

  public:
    inline UnsignedId(int _id = 0) { id = _id; }
    inline quint32 toInt() const { return id; }
    inline bool operator==(const UnsignedId &other) const { return id == other.id; }
    inline bool operator!=(const UnsignedId &other) const { return id != other.id; }
};

struct BufferId : public UnsignedId {
  inline BufferId(int _id = 0) : UnsignedId(_id) {};

};
*/

// FIXME make all ID types quint32 as soon as they all have been replaced
typedef uint UserId;     //!< Identifies a core user.
typedef uint MsgId;      //!< Identifies a message.
typedef uint BufferId;   //!< Identifies a buffer.
// These must be signed!
typedef qint32 NetworkId;  //!< Identifies an IRC Network.
typedef qint32 IdentityId; //!< Identifies an identity.

//! Base class for exceptions.
struct Exception {
  Exception(QString msg = "Unknown Exception") : _msg(msg) {};
  virtual ~Exception() {}; // make gcc happy
  virtual inline QString msg() { return _msg; }

  protected:
    QString _msg;

};

#endif

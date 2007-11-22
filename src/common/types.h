/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

typedef uint UserId;    //!< Identifies a core user.
typedef uint MsgId;     //!< Identifies a message.
typedef uint BufferId;  //!< Identifies a buffer.
typedef uint NetworkId; //!< Identifies an IRC Network.

//! Base class for exceptions.
struct Exception {
  Exception(QString msg = "Unknown Exception") : _msg(msg) {};
  virtual ~Exception() {}; // make gcc happy
  virtual inline QString msg() { return _msg; }

  protected:
    QString _msg;

};

#endif

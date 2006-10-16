/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#include <QtGlobal>
#include "messages.h"


/** This macro marks strings as translateable for Qt's linguist tools */
#define _(str) QT_TR_NOOP(str)


/** Defines the message codes according to RFCs 1495/281x.
 *  Named commands have a negative enum value.
 */

/** \NOTE: Function handlers _must_ be global functions or static methods! */

/** Set handler addresses to 0 to use the default (server) handler. */

BuiltinCmd builtins[] = {
  { _("admin"), _("Get information about the administrator of a server."),
    _("[server]"), _("server: Server"),
    0, 0 },


  { 0, 0, 0, 0, 0, 0 }
};



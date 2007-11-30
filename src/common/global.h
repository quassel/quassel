/***************************************************************************
 *   Copyright (C) 2005 by the Quassel IRC Team                            *
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

// This file needs probably to go away at some point. Not much left anymore.

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

// Enable some shortcuts and stuff
//#define DEVELMODE


/** The protocol version we use fo the communication between core and GUI */
#define GUI_PROTOCOL 3

#define DEFAULT_PORT 4242

/* Some global stuff */

namespace Global {
  enum RunMode { Monolithic, ClientOnly, CoreOnly };
  extern RunMode runMode;
}

#endif

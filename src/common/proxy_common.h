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

#ifndef _PROXY_COMMON_H_
#define _PROXY_COMMON_H_

enum ClientSignal { GS_CLIENT_INIT, GS_USER_INPUT, GS_REQUEST_CONNECT, GS_UPDATE_GLOBAL_DATA, GS_IMPORT_BACKLOG,
  GS_REQUEST_BACKLOG

};

enum CoreSignal { CS_CORE_STATE, CS_SERVER_CONNECTED, CS_SERVER_DISCONNECTED, CS_SERVER_STATE,
  CS_DISPLAY_MSG, CS_DISPLAY_STATUS_MSG, CS_UPDATE_GLOBAL_DATA,
  CS_MODE_SET, CS_TOPIC_SET, CS_SET_NICKS, CS_NICK_ADDED, CS_NICK_REMOVED, CS_NICK_RENAMED, CS_NICK_UPDATED,
  CS_OWN_NICK_SET, CS_QUERY_REQUESTED, CS_BACKLOG_DATA, CS_UPDATE_BUFFERID

};


#endif

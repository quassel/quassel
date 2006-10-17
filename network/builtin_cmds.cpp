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
#include "message.h"
#include "cmdcodes.h"

/** This macro marks strings as translateable for Qt's linguist tools */
#define _(str) QT_TR_NOOP(str)


/** Defines the message codes according to RFCs 1495/281x.
 *  Named commands have a negative enum value.
 */

/** \NOTE: Function handlers _must_ be global functions or static methods! */

/** Set handler addresses to 0 to use the default (server) handler. */

BuiltinCmd builtins[] = {
  { CMD_ADMIN, "admin", _("Get information about the administrator of a server."),
    _("[server]"), _("server: Server"), 0, 0 },
  { CMD_AME, "ame", "", "", "", 0, 0 },
  { CMD_AMSG, "amsg", _("Send message to all channels of all connected servers."),
    _("message"), _("message: Message to send"), 0, 0 },
  { CMD_AWAY, "away", _("Toggle away status."),
    _("[-all] [message]"), _("   -all: Toggle status on all connected servers\n"
                             "message: Away message (away status is removed if no message is given)"), 0, 0 },
  { CMD_BAN, "ban", _("Ban a nickname or hostmask."),
    _("[channel] [nick [nick ...]]"), _("channel: Channel for ban (current of empty)\n"
                                        "   nick: Nickname or hostmask. If no nicknames are given, /ban displays the current banlist."), 0, 0 },
  { CMD_CTCP, "ctcp", _("Send a CTCP message (Client-To-Client Protocol)"),
    _("target type [args]"), _("target: Nick or channel to send CTCP to\n"
                               "  type: CTCP Type (e.g. VERSION, PING, CHAT...)\n"
                               "  args: Arguments for CTCP"), 0, 0 },
  { CMD_CYCLE, "cycle", "", "", "", 0, 0 },
  { CMD_DEHALFOP, "dehalfop", "", "", "", 0, 0 },
  { CMD_DEOP, "deop", "", "", "", 0, 0 },
  { CMD_DEVOICE, "devoice", "", "", "", 0, 0 },
  { CMD_DIE, "die", "", "", "", 0, 0 },
  { CMD_ERROR, "error", "", "", "", 0, 0 },
  { CMD_HALFOP, "halfop", "", "", "", 0, 0 },
  { CMD_INFO, "info", "", "", "", 0, 0 },
  { CMD_INVITE, "invite", "", "", "", 0, 0 },
  { CMD_ISON, "ison", "", "", "", 0, 0 },
  { CMD_JOIN, "join", "", "", "", 0, 0 },
  { CMD_KICK, "kick", "", "", "", 0, 0 },
  { CMD_KICKBAN, "kickban", "", "", "", 0, 0 },
  { CMD_KILL, "kill", "", "", "", 0, 0 },
  { CMD_LINKS, "links", "", "", "", 0, 0 },
  { CMD_LIST, "list", "", "", "", 0, 0 },
  { CMD_LUSERS, "lusers", "", "", "", 0, 0 },
  { CMD_ME, "me", "", "", "", 0, 0 },
  { CMD_MODE, "mode", "", "", "", 0, 0 },
  { CMD_MOTD, "motd", "", "", "", 0, 0 },
  { CMD_MSG, "msg", "", "", "", 0, 0 },
  { CMD_NAMES, "names", "", "", "", 0, 0 },
  { CMD_NICK, "nick", "", "", "", 0, 0 },
  { CMD_NOTICE, "notice", _("Send notice message to user."),
    _("nick message"), _("   nick: user to send notice to\n"
                         "message: text to send"), 0, 0 },
  { CMD_OP, "op", "", "", "", 0, 0 },
  { CMD_OPER, "oper", "", "", "", 0, 0 },
  { CMD_PART, "part", "", "", "", 0, 0 },
  { CMD_PING, "ping", _("Ping a server."),
    _("server1 [server2]"), _("server1: Server to ping\nserver2: Forward ping to this server"), 0, 0 },
  { CMD_PONG, "pong", "", "", "", 0, 0 },
  { CMD_PRIVMSG, "privmsg", "", "", "", 0, 0 },
  { CMD_QUERY, "query", "", "", "", 0, 0 },
  { CMD_QUIT, "quit", "", "", "", 0, 0 },
  { CMD_QUOTE, "quote", "", "", "", 0, 0 },
  { CMD_REHASH, "rehash", "", "", "", 0, 0 },
  { CMD_RESTART, "restart", "", "", "", 0, 0 },
  { CMD_SERVICE, "service", "", "", "", 0, 0 },
  { CMD_SERVLIST, "servlist", "", "", "", 0, 0 },
  { CMD_SQUERY, "squery", "", "", "", 0, 0 },
  { CMD_SQUIT, "squit", "", "", "", 0, 0 },
  { CMD_STATS, "stats", "", "", "", 0, 0 },
  { CMD_SUMMON, "summon", "", "", "", 0, 0 },
  { CMD_TIME, "time", "", "", "", 0, 0 },
  { CMD_TOPIC, "topic", "", "", "", 0, 0 },
  { CMD_TRACE, "trace", "", "", "", 0, 0 },
  { CMD_UNBAN, "unban", "", "", "", 0, 0 },
  { CMD_USERHOST, "userhost", "", "", "", 0, 0 },
  { CMD_USERS, "users", "", "", "", 0, 0 },
  { CMD_VERSION, "version", "", "", "", 0, 0 },
  { CMD_VOICE, "voice", "", "", "", 0, 0 },
  { CMD_WALLOPS, "wallops", "", "", "", 0, 0 },
  { CMD_WHO, "who", "", "", "", 0, 0 },
  { CMD_WHOIS, "whois", "", "", "", 0, 0 },
  { CMD_WHOWAS, "whowas", "", "", "", 0, 0 },

  { 0, 0, 0, 0, 0, 0, 0 }
};



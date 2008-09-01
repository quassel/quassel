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
#include "ircserverhandler.h"

#include "util.h"

#include "coresession.h"
#include "coreirclisthelper.h"
#include "networkconnection.h"
#include "network.h"
#include "identity.h"
#include "ctcphandler.h"

#include "ircuser.h"
#include "ircchannel.h"
#include "logger.h"

#include <QDebug>

IrcServerHandler::IrcServerHandler(NetworkConnection *parent)
  : BasicHandler(parent),
    _whois(false)
{
}

IrcServerHandler::~IrcServerHandler() {

}

/*! Handle a raw message string sent by the server. We try to find a suitable handler, otherwise we call a default handler. */
void IrcServerHandler::handleServerMsg(QByteArray msg) {
  try {
    if(msg.isEmpty()) {
      quWarning() << "Received empty string from server!";
      return;
    }

    // Now we split the raw message into its various parts...
    QString prefix = "";
    QByteArray trailing;
    QString cmd;

    // First, check for a trailing parameter introduced by " :", since this might screw up splitting the msg
    // NOTE: This assumes that this is true in raw encoding, but well, hopefully there are no servers running in japanese on protocol level...
    int idx = msg.indexOf(" :");
    if(idx >= 0) {
      if(msg.length() > idx + 2) trailing = msg.mid(idx + 2);
      msg = msg.left(idx);
    }
    // OK, now it is safe to split...
    QList<QByteArray> params = msg.split(' ');
    if(!trailing.isEmpty()) params << trailing;
    if(params.count() < 1) {
      quWarning() << "Received invalid string from server!";
      return;
    }

    QString foo = serverDecode(params.takeFirst());

    // a colon as the first chars indicates the existence of a prefix
    if(foo[0] == ':') {
      foo.remove(0, 1);
      prefix = foo;
      if(params.count() < 1) {
        quWarning() << "Received invalid string from server!";
        return;
      }
      foo = serverDecode(params.takeFirst());
    }

    // next string without a whitespace is the command
    cmd = foo.trimmed().toUpper();

    // numeric replies have the target as first param (RFC 2812 - 2.4). this is usually our own nick. Remove this!
    uint num = cmd.toUInt();
    if(num > 0) {
      if(params.count() == 0) {
        quWarning() << "Message received from server violates RFC and is ignored!";
        return;
      }
      params.removeFirst();
    }

    // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
    handle(cmd, Q_ARG(QString, prefix), Q_ARG(QList<QByteArray>, params));
    //handle(cmd, Q_ARG(QString, prefix));
  } catch(Exception e) {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", e.msg());
  }
}


void IrcServerHandler::defaultHandler(QString cmd, const QString &prefix, const QList<QByteArray> &rawparams) {
  // we assume that all this happens in server encoding
  QStringList params = serverDecode(rawparams);
  uint num = cmd.toUInt();
  if(num) {
    // A lot of server messages don't really need their own handler because they don't do much.
    // Catch and handle these here.
    switch(num) {
      // Welcome, status, info messages. Just display these.
      case 2: case 3: case 4: case 5: case 251: case 252: case 253: case 254: case 255: case 372: case 375:
        emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", params.join(" "), prefix);
        break;
      // Server error messages without param, just display them
      case 409: case 411: case 412: case 422: case 424: case 445: case 446: case 451: case 462:
      case 463: case 464: case 465: case 466: case 472: case 481: case 483: case 485: case 491: case 501: case 502:
      case 431: // ERR_NONICKNAMEGIVEN 
        emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", params.join(" "), prefix);
        break;
      // Server error messages, display them in red. First param will be appended.
      case 401: {
	QString target = params.takeFirst();
	displayMsg(Message::Error, target, params.join(" ") + " " + target, prefix, Message::Redirected);
	break;
      }
      case 402: case 403: case 404: case 406: case 408: case 415: case 421: case 442: {
	QString channelName = params.takeFirst();
        emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", params.join(" ") + " " + channelName, prefix);
        break;
      }
      // Server error messages which will be displayed with a colon between the first param and the rest
      case 413: case 414: case 423: case 441: case 444: case 461:  // FIXME see below for the 47x codes
      case 467: case 471: case 473: case 474: case 475: case 476: case 477: case 478: case 482:
      case 436: // ERR_NICKCOLLISION
      { QString p = params.takeFirst();
        emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", p + ": " + params.join(" "));
        break;
      }
      // Ignore these commands.
      case 321: case 366: case 376:
        break;

      // Everything else will be marked in red, so we can add them somewhere.
      default:
	if(_whois) {
	  // many nets define their own WHOIS fields. we fetch those not in need of special attention here:
	  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", "[Whois] " + params.join(" "), prefix);
	} else {
	  if(networkConnection()->coreSession()->ircListHelper()->requestInProgress(network()->networkId()))
	    networkConnection()->coreSession()->ircListHelper()->reportError(params.join(" "));
	  else
	    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", cmd + " " + params.join(" "), prefix);
	}
    }
    //qDebug() << prefix <<":"<<cmd<<params;
  } else {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Unknown: ") + cmd + " " + params.join(" "), prefix);
    //qDebug() << prefix <<":"<<cmd<<params;
  }
}

//******************************/
// IRC SERVER HANDLER
//******************************/
void IrcServerHandler::handleJoin(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleJoin()", params, 1))
    return;

  QString channel = serverDecode(params[0]);
  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  emit displayMsg(Message::Join, BufferInfo::ChannelBuffer, channel, channel, prefix);
  //qDebug() << "IrcServerHandler::handleJoin()" << prefix << params;
  ircuser->joinChannel(channel);
  if(network()->isMe(ircuser)) {
    networkConnection()->setChannelJoined(channel);
    putCmd("MODE", params[0]); // we want to know the modes of the channel we just joined, so we ask politely
  }
}

void IrcServerHandler::handleKick(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleKick()", params, 2))
    return;

  network()->updateNickFromMask(prefix);
  IrcUser *victim = network()->ircUser(params[1]);
  if(!victim)
    return;
  
  QString channel = serverDecode(params[0]);
  victim->partChannel(channel);

  QString msg;
  if(params.count() > 2) // someone got a reason!
    msg = QString("%1 %2").arg(victim->nick()).arg(channelDecode(channel, params[2]));
  else
    msg = victim->nick();

  emit displayMsg(Message::Kick, BufferInfo::ChannelBuffer, channel, msg, prefix);
  //if(network()->isMe(victim)) networkConnection()->setKickedFromChannel(channel);
}

void IrcServerHandler::handleMode(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleMode()", params, 2))
    return;

  if(network()->isChannelName(serverDecode(params[0]))) {
    // Channel Modes
    emit displayMsg(Message::Mode, BufferInfo::ChannelBuffer, serverDecode(params[0]), serverDecode(params).join(" "), prefix);

    IrcChannel *channel = network()->ircChannel(params[0]);
    if(!channel) {
      // we received mode information for a channel we're not in. that means probably we've just been kicked out or something like that
      // anyways: we don't have a place to store the data --> discard the info.
      return;
    }

    QString modes = params[1];
    bool add = true;
    int paramOffset = 2;
    for(int c = 0; c < modes.length(); c++) {
      if(modes[c] == '+') {
	add = true;
	continue;
      }
      if(modes[c] == '-') {
	add = false;
	continue;
      }

      if(network()->prefixModes().contains(modes[c])) {
	// user channel modes (op, voice, etc...)
	if(paramOffset < params.count()) {
	  IrcUser *ircUser = network()->ircUser(params[paramOffset]);
	  if(add)
	    channel->addUserMode(ircUser, QString(modes[c]));
	  else
	    channel->removeUserMode(ircUser, QString(modes[c]));
	} else {
	  quWarning() << "Received MODE with too few parameters:" << serverDecode(params);
	}
	paramOffset++;
      } else {
	// regular channel modes
	QString value;
	Network::ChannelModeType modeType = network()->channelModeType(modes[c]);
	if(modeType == Network::A_CHANMODE || modeType == Network::B_CHANMODE || (modeType == Network::C_CHANMODE && add)) {
	    if(paramOffset < params.count()) {
	      value = params[paramOffset];
	    } else {
	      quWarning() << "Received MODE with too few parameters:" << serverDecode(params);
	    }
	    paramOffset++;
	}
	
	if(add)
	  channel->addChannelMode(modes[c], value);
	else
	  channel->removeChannelMode(modes[c], value);
      }
    }
    
  } else {
    // pure User Modes
    IrcUser *ircUser = network()->newIrcUser(params[0]);
    QString modeString(serverDecode(params[1]));
    QString addModes;
    QString removeModes;
    bool add = false;
    for(int c = 0; c < modeString.count(); c++) {
      if(modeString[c] == '+') {
	add = true;
	continue;
      }
      if(modeString[c] == '-') {
	add = false;
	continue;
      }
      if(add)
	addModes += modeString[c];
      else
	removeModes += modeString[c];
    }
    if(!addModes.isEmpty())
      ircUser->addUserModes(addModes);
    if(!removeModes.isEmpty())
      ircUser->removeUserModes(removeModes);
    
    // FIXME: redirect
    emit displayMsg(Message::Mode, BufferInfo::StatusBuffer, "", serverDecode(params).join(" "), prefix);
  }
}

void IrcServerHandler::handleNick(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleNick()", params, 1))
    return;

  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  if(!ircuser) {
    quWarning() << "IrcServerHandler::handleNick(): Unknown IrcUser!";
    return;
  }
  QString newnick = serverDecode(params[0]);
  QString oldnick = ircuser->nick();

  QString sender = network()->isMyNick(oldnick)
    ? newnick
    : prefix;

  emit nickChanged(newnick, oldnick);
  foreach(QString channel, ircuser->channels())
    emit displayMsg(Message::Nick, BufferInfo::ChannelBuffer, channel, newnick, sender);
  
  ircuser->setNick(newnick);
}

void IrcServerHandler::handleNotice(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleNotice()", params, 2))
    return;

  QString target = serverDecode(params[0]);
  if(prefix.isEmpty() || target == "AUTH")
    target = "";
  else if(!network()->isChannelName(target))
    target = nickFromMask(prefix);

  networkConnection()->ctcpHandler()->parse(Message::Notice, prefix, target, params[1]);
}

void IrcServerHandler::handlePart(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handlePart()", params, 1))
    return;

  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  QString channel = serverDecode(params[0]);
  if(!ircuser) {
    quWarning() << "IrcServerHandler::handlePart(): Unknown IrcUser!";
    return;
  }

  ircuser->partChannel(channel);

  QString msg;
  if(params.count() > 1)
    msg = userDecode(ircuser->nick(), params[1]);

  emit displayMsg(Message::Part, BufferInfo::ChannelBuffer, channel, msg, prefix);
  if(network()->isMe(ircuser)) networkConnection()->setChannelParted(channel);
}

void IrcServerHandler::handlePing(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  putCmd("PONG", params);
}

void IrcServerHandler::handlePong(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  // the server is supposed to send back what we passed as param. and we send a timestamp
  // but using quote and whatnought one can send arbitrary pings, so we have to do some sanity checks
  if(params.count() < 2)
    return;

  QString timestamp = serverDecode(params[1]);
  QTime sendTime = QTime::fromString(timestamp, "hh:mm:ss.zzz");
  if(!sendTime.isValid()) {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", "PONG " + serverDecode(params).join(" "), prefix);
    return;
  }

  network()->setLatency(sendTime.msecsTo(QTime::currentTime()) / 2);
}

void IrcServerHandler::handlePrivmsg(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handlePrivmsg()", params, 1))
    return;

  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  if(!ircuser) {
    quWarning() << "IrcServerHandler::handlePrivmsg(): Unknown IrcUser!";
    return;
  }

  if(params.isEmpty()) {
    quWarning() << "IrcServerHandler::handlePrivmsg(): received PRIVMSG without target or message from:" << prefix;
    return;
  }
     
  QString target = serverDecode(params[0]);

  QByteArray msg = params.count() < 2
    ? QByteArray("")
    : params[1];

  // are we the target?
  if(network()->isMyNick(target))
    target = nickFromMask(ircuser->nick());

  // it's possible to pack multiple privmsgs into one param using ctcp
  // - > we let the ctcpHandler do the work
  networkConnection()->ctcpHandler()->parse(Message::Plain, prefix, target, msg);
}

void IrcServerHandler::handleQuit(const QString &prefix, const QList<QByteArray> &params) {
  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  if(!ircuser) return;

  QString msg;
  if(params.count() > 0)
    msg = userDecode(ircuser->nick(), params[0]);

  foreach(QString channel, ircuser->channels())
    emit displayMsg(Message::Quit, BufferInfo::ChannelBuffer, channel, msg, prefix);

  network()->removeIrcUser(nickFromMask(prefix));
}

void IrcServerHandler::handleTopic(const QString &prefix, const QList<QByteArray> &params) {
  if(!checkParamCount("IrcServerHandler::handleTopic()", params, 1))
    return;

  IrcUser *ircuser = network()->updateNickFromMask(prefix);
  if(!ircuser)
    return;

  IrcChannel *channel = network()->ircChannel(serverDecode(params[0]));
  if(!channel)
    return;
  
  QString topic;
  if(params.count() > 1)
    topic = channelDecode(channel->name(), params[1]);

  channel->setTopic(topic);

  emit displayMsg(Message::Server, BufferInfo::ChannelBuffer, channel->name(), tr("%1 has changed topic for %2 to: \"%3\"").arg(ircuser->nick()).arg(channel->name()).arg(topic));
}

/* RPL_WELCOME */
void IrcServerHandler::handle001(const QString &prefix, const QList<QByteArray> &params) {
  network()->setCurrentServer(prefix);

  if(params.isEmpty()) {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("%1 didn't supply a valid welcome message... expect some serious issues..."));
  }
  // there should be only one param: "Welcome to the Internet Relay Network <nick>!<user>@<host>"
  QString param = serverDecode(params[0]);
  QString myhostmask = param.section(' ', -1, -1);

  network()->setMyNick(nickFromMask(myhostmask));

  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", param, prefix);
}

/* RPL_ISUPPORT */
// TODO Complete 005 handling, also use sensible defaults for non-sent stuff
void IrcServerHandler::handle005(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  const int numParams = params.size();
  if(numParams == 0) {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Received RPL_ISUPPORT (005) without parameters!"), prefix);
    return;
  }

  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", serverDecode(params).join(" "), prefix);

  QString rpl_isupport_suffix = serverDecode(params.last());
  if(!rpl_isupport_suffix.toLower().contains("are supported by this server")) {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Received non RFC compliant RPL_ISUPPORT: this can lead to unexpected behavior!"), prefix);
  }

  QString rawSupport;
  QString key, value;
  for(int i = 0; i < numParams - 1; i++) {
    QString rawSupport = serverDecode(params[i]);
    QString key = rawSupport.section("=", 0, 0);
    QString value = rawSupport.section("=", 1);
    network()->addSupport(key, value);
  }
}

/* RPL_UMODEIS - "<user_modes> [<user_mode_params>]" */
void IrcServerHandler::handle221(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  //TODO: save information in network object
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("%1").arg(serverDecode(params).join(" ")));
}

/* RPL_STATSCONN - "Highest connection cout: 8000 (7999 clients)" */
void IrcServerHandler::handle250(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  //TODO: save information in network object
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("%1").arg(serverDecode(params).join(" ")));
}

/* RPL_LOCALUSERS - "Current local user: 5024  Max: 7999 */
void IrcServerHandler::handle265(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  //TODO: save information in network object
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("%1").arg(serverDecode(params).join(" ")));
}

/* RPL_GLOBALUSERS - "Current global users: 46093  Max: 47650" */
void IrcServerHandler::handle266(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  //TODO: save information in network object
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("%1").arg(serverDecode(params).join(" ")));
}

/* 
WHOIS-Message: 
   Replies 311 - 313, 317 - 319 are all replies generated in response to a WHOIS message.
  and 301 (RPL_AWAY)
              "<nick> :<away message>"
WHO-Message:
   Replies 352 and 315 paired are used to answer a WHO message.

WHOWAS-Message:
   Replies 314 and 369 are responses to a WHOWAS message.

*/


/*   RPL_AWAY - "<nick> :<away message>" */
void IrcServerHandler::handle301(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle301()", params, 2))
    return;


  QString nickName = serverDecode(params[0]);
  QString awayMessage = userDecode(nickName, params[1]);

  IrcUser *ircuser = network()->ircUser(nickName);
  if(ircuser) {
    ircuser->setAwayMessage(awayMessage);
    ircuser->setAway(true);
  }

  // FIXME: proper redirection needed
  if(_whois) {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is away: \"%2\"").arg(nickName).arg(awayMessage));
  } else {
    if(ircuser) {
      int now = QDateTime::currentDateTime().toTime_t();
      int silenceTime = 60;
      if(ircuser->lastAwayMessage() + silenceTime < now) {
        emit displayMsg(Message::Server, BufferInfo::QueryBuffer, params[0], tr("%1 is away: \"%2\"").arg(nickName).arg(awayMessage));
      }
      ircuser->setLastAwayMessage(now);
    } else {
      // probably should not happen
      emit displayMsg(Message::Server, BufferInfo::QueryBuffer, params[0], tr("%1 is away: \"%2\"").arg(nickName).arg(awayMessage));
    }
  }
}

// 305  RPL_UNAWAY
//      ":You are no longer marked as being away"
void IrcServerHandler::handle305(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  IrcUser *me = network()->me();
  if(me)
    me->setAway(false);

  if(!params.isEmpty())
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", serverDecode(params[0]));
}

// 306  RPL_NOWAWAY
//      ":You have been marked as being away"
void IrcServerHandler::handle306(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  IrcUser *me = network()->me();
  if(me)
    me->setAway(true);

  if(!params.isEmpty())
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", serverDecode(params[0]));
}

/* RPL_WHOISSERVICE - "<user> is registered nick" */
void IrcServerHandler::handle307(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle307()", params, 1))
    return;

  QString whoisServiceReply = serverDecode(params).join(" ");
  IrcUser *ircuser = network()->ircUser(serverDecode(params[0]));
  if(ircuser) {
    ircuser->setWhoisServiceReply(whoisServiceReply);
  }
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1").arg(whoisServiceReply));
}

/* RPL_SUSERHOST - "<user> is available for help." */
void IrcServerHandler::handle310(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle310()", params, 1))
    return;

  QString suserHost = serverDecode(params).join(" ");
  IrcUser *ircuser = network()->ircUser(serverDecode(params[0]));
  if(ircuser) {
    ircuser->setSuserHost(suserHost);
  }
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1").arg(suserHost));
}

/*  RPL_WHOISUSER - "<nick> <user> <host> * :<real name>" */
void IrcServerHandler::handle311(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle311()", params, 3))
    return;

  _whois = true;
  IrcUser *ircuser = network()->ircUser(serverDecode(params[0]));
  if(ircuser) {
    ircuser->setUser(serverDecode(params[1]));
    ircuser->setHost(serverDecode(params[2]));
    ircuser->setRealName(serverDecode(params.last()));
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is %2 (%3)") .arg(ircuser->nick()).arg(ircuser->hostmask()).arg(ircuser->realName()));
  } else {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is %2 (%3)") .arg(serverDecode(params[1])).arg(serverDecode(params[2])).arg(serverDecode(params.last())));
  }
}
 
/*  RPL_WHOISSERVER -  "<nick> <server> :<server info>" */
void IrcServerHandler::handle312(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle312()", params, 2))
    return;

  IrcUser *ircuser = network()->ircUser(serverDecode(params[0]));
  if(ircuser) {
    ircuser->setServer(serverDecode(params[1]));
  }

  QString returnString = tr("%1 is online via %2 (%3)").arg(serverDecode(params[0])).arg(serverDecode(params[1])).arg(serverDecode(params.last()));
  if(_whois) {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1").arg(returnString));
  } else {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whowas] %1").arg(returnString));
  }
}

/*  RPL_WHOISOPERATOR - "<nick> :is an IRC operator" */
void IrcServerHandler::handle313(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle313()", params, 1))
    return;

  IrcUser *ircuser = network()->ircUser(serverDecode(params[0]));
  if(ircuser) {
    ircuser->setIrcOperator(params.last());
  }
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1").arg(serverDecode(params).join(" ")));
}

/*  RPL_WHOWASUSER - "<nick> <user> <host> * :<real name>" */
void IrcServerHandler::handle314(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle314()", params, 3))
    return;

  QString nick = serverDecode(params[0]);
  QString hostmask = QString("%1@%2").arg(serverDecode(params[1])).arg(serverDecode(params[2]));
  QString realName = serverDecode(params.last());
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whowas] %1 was %2 (%3)").arg(nick).arg(hostmask).arg(realName));
}

/*  RPL_ENDOFWHO: "<name> :End of WHO list" */
void IrcServerHandler::handle315(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle315()", params, 1))
    return;

  QStringList p = serverDecode(params);
  if(networkConnection()->setAutoWhoDone(p[0])) {
    return; // stay silent
  }
  p.takeLast(); // should be "End of WHO list"
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Who] End of /WHO list for %1").arg(p.join(" ")));
}

/*  RPL_WHOISIDLE - "<nick> <integer> :seconds idle" 
   (real life: "<nick> <integer> <integer> :seconds idle, signon time) */
void IrcServerHandler::handle317(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle317()", params, 2))
    return;

  QString nick = serverDecode(params[0]);
  IrcUser *ircuser = network()->ircUser(nick);
  if(ircuser) {
    QDateTime now = QDateTime::currentDateTime();
    int idleSecs = serverDecode(params[1]).toInt();
    idleSecs *= -1;
    ircuser->setIdleTime(now.addSecs(idleSecs));
    if(params.size() > 3) {	// if we have more then 3 params we have the obove mentioned "real life" situation
      int loginTime = serverDecode(params[2]).toInt();
      ircuser->setLoginTime(QDateTime::fromTime_t(loginTime));
      emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is logged in since %2").arg(ircuser->nick()).arg(ircuser->loginTime().toString()));
    }
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is idling for %2 (%3)").arg(ircuser->nick()).arg(secondsToString(ircuser->idleTime().secsTo(now))).arg(ircuser->idleTime().toString()));
    
  } else {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] idle message: %1").arg(userDecode(nick, params).join(" ")));
  }
}

/*  RPL_ENDOFWHOIS - "<nick> :End of WHOIS list" */
void IrcServerHandler::handle318(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  _whois = false;
  QStringList parameter = serverDecode(params);
  parameter.removeFirst();
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1").arg(parameter.join(" ")));
}

/*  RPL_WHOISCHANNELS - "<nick> :*( ( "@" / "+" ) <channel> " " )" */
void IrcServerHandler::handle319(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle319()", params, 2))
    return;

  QString nick = serverDecode(params.first());
  QStringList op;
  QStringList voice;
  QStringList user;
  foreach (QString channel, serverDecode(params.last()).split(" ")) {
    if(channel.startsWith("@"))
       op.append(channel.remove(0,1));
    else if(channel.startsWith("+"))
      voice.append(channel.remove(0,1));
    else
      user.append(channel);
  }
  if(!user.isEmpty()) 
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is a user on channels: %2").arg(nick).arg(user.join(" ")));
  if(!voice.isEmpty()) 
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 has voice on channels: %2").arg(nick).arg(voice.join(" ")));
  if(!op.isEmpty()) 
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1 is an operator on channels: %2").arg(nick).arg(op.join(" ")));
}

/*  RPL_WHOISVIRT - "<nick> is identified to services" */
void IrcServerHandler::handle320(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whois] %1").arg(serverDecode(params).join(" ")));
}

/* RPL_LIST -  "<channel> <# visible> :<topic>" */
void IrcServerHandler::handle322(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  QString channelName;
  quint32 userCount = 0;
  QString topic;
  
  int paramCount = params.count();
  switch(paramCount) {
  case 3:
    topic = serverDecode(params[2]);
  case 2:
    userCount = serverDecode(params[1]).toUInt();
  case 1:
    channelName = serverDecode(params[0]);
  default:
    break;
  }
  if(!networkConnection()->coreSession()->ircListHelper()->addChannel(network()->networkId(), channelName, userCount, topic))
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Channel %1 has %2 users. Topic is: %3").arg(channelName).arg(userCount).arg(topic));
}

/* RPL_LISTEND ":End of LIST" */
void IrcServerHandler::handle323(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  Q_UNUSED(params)

  if(!networkConnection()->coreSession()->ircListHelper()->endOfChannelList(network()->networkId()))
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("End of channel list"));
}
       
/* RPL_CHANNELMODEIS - "<channel> <mode> <mode params>" */
void IrcServerHandler::handle324(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  handleMode(prefix, params);
}

/* RPL_??? - "<channel> <creation time (unix)>" */
void IrcServerHandler::handle329(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  Q_UNUSED(params)
#ifdef __GNUC__
#  warning "Implement handle329 (Channel creation time)"
#endif
  // FIXME implement this... 
}

/* RPL_NOTOPIC */
void IrcServerHandler::handle331(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle331()", params, 1))
    return;

  QString channel = serverDecode(params[0]);
  network()->ircChannel(channel)->setTopic(QString());
  emit displayMsg(Message::Server, BufferInfo::ChannelBuffer, channel, tr("No topic is set for %1.").arg(channel));
}

/* RPL_TOPIC */
void IrcServerHandler::handle332(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle332()", params, 2))
    return;

  QString channel = serverDecode(params[0]);
  QString topic = channelDecode(channel, params[1]);
  network()->ircChannel(channel)->setTopic(topic);
  emit displayMsg(Message::Server, BufferInfo::ChannelBuffer, channel, tr("Topic for %1 is \"%2\"").arg(channel, topic));
}

/* Topic set by... */
void IrcServerHandler::handle333(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle333()", params, 3))
    return;

  QString channel = serverDecode(params[0]);
  emit displayMsg(Message::Server, BufferInfo::ChannelBuffer, channel,
                  tr("Topic set by %1 on %2") .arg(serverDecode(params[1]), QDateTime::fromTime_t(channelDecode(channel, params[2]).toUInt()).toString()));
}

/*  RPL_WHOREPLY: "<channel> <user> <host> <server> <nick> 
              ( "H" / "G" > ["*"] [ ( "@" / "+" ) ] :<hopcount> <real name>" */
void IrcServerHandler::handle352(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  if(!checkParamCount("IrcServerHandler::handle352()", params, 6))
    return;

  QString channel = serverDecode(params[0]);
  IrcUser *ircuser = network()->ircUser(serverDecode(params[4]));
  if(ircuser) {
    ircuser->setUser(serverDecode(params[1]));
    ircuser->setHost(serverDecode(params[2]));

    bool away = serverDecode(params[5]).startsWith("G") ? true : false;
    ircuser->setAway(away);
    ircuser->setServer(serverDecode(params[3])); 
    ircuser->setRealName(serverDecode(params.last()).section(" ", 1));
  }

  if(!networkConnection()->isAutoWhoInProgress(channel)) {
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Who] %1").arg(serverDecode(params).join(" ")));
  }
}

/* RPL_NAMREPLY */
void IrcServerHandler::handle353(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle353()", params, 2))
    return;
    
  // param[0] is either "=", "*" or "@" indicating a public, private or secret channel
  // we don't use this information at the time beeing
  QString channelname = serverDecode(params[1]);

  IrcChannel *channel = network()->ircChannel(channelname);
  if(!channel) {
    quWarning() << "IrcServerHandler::handle353(): received unknown target channel:" << channelname;
    return;
  }

  QStringList nicks;
  QStringList modes;
  
  foreach(QString nick, serverDecode(params[2]).split(' ')) {
    QString mode = QString();

    if(network()->prefixes().contains(nick[0])) {
      mode = network()->prefixToMode(nick[0]);
      nick = nick.mid(1);
    }

    nicks << nick;
    modes << mode;
  }
  
  channel->joinIrcUsers(nicks, modes);
}

/*  RPL_ENDOFWHOWAS - "<nick> :End of WHOWAS" */
void IrcServerHandler::handle369(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix)
  emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("[Whowas] %1").arg(serverDecode(params).join(" ")));
}

/* ERR_ERRONEUSNICKNAME */
void IrcServerHandler::handle432(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);

  if(params.size() < 2) {
    // handle unreal-ircd bug, where unreal ircd doesnt supply a TARGET in ERR_ERRONEUSNICKNAME during registration phase:
    // nick @@@
    // :irc.scortum.moep.net 432  @@@ :Erroneous Nickname: Illegal characters
    // correct server reply:
    // :irc.scortum.moep.net 432 * @@@ :Erroneous Nickname: Illegal characters
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("There is a nickname in your identity's nicklist which contains illegal characters"));
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Due to a bug in Unreal IRCd (and maybe other irc-servers too) we're unable to determine the erroneous nick"));
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Please use: /nick <othernick> to continue or clean up your nicklist"));
  } else {
    QString errnick = params[0];
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Nick %1 contains illegal characters").arg(errnick));
    tryNextNick(errnick);
  }
}

/* ERR_NICKNAMEINUSE */
void IrcServerHandler::handle433(const QString &prefix, const QList<QByteArray> &params) {
  Q_UNUSED(prefix);
  if(!checkParamCount("IrcServerHandler::handle433()", params, 1))
    return;
    
  QString errnick = serverDecode(params[0]);
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Nick already in use: %1").arg(errnick));

  // if there is a problem while connecting to the server -> we handle it
  // but only if our connection has not been finished yet...
  if(!network()->currentServer().isEmpty())
    return;

  tryNextNick(errnick);
}

/* */

// FIXME networkConnection()->setChannelKey("") for all ERR replies indicating that a JOIN went wrong
//       mostly, these are codes in the 47x range

/* */

void IrcServerHandler::tryNextNick(const QString &errnick) {
  QStringList desiredNicks = networkConnection()->coreSession()->identity(network()->identity())->nicks();
  int nextNick = desiredNicks.indexOf(errnick) + 1;
  if(desiredNicks.size() > nextNick) {
    putCmd("NICK", serverEncode(desiredNicks[nextNick]));
  } else {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("No free and valid nicks in nicklist found. use: /nick <othernick> to continue"));
  }
}

bool IrcServerHandler::checkParamCount(const QString &methodName, const QList<QByteArray> &params, int minParams) {
  if(params.count() < minParams) {
    quWarning() << qPrintable(methodName) << "requires" << minParams << "parameters but received only" << params.count() << serverDecode(params);
    return false;
  } else {
    return true;
  }
}


/***********************************************************************************/



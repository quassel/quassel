/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
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
#include "ircserverhandler.h"

#include "util.h"

#include "server.h"
#include "networkinfo.h"
#include "ctcphandler.h"

#include "ircuser.h"
#include "ircchannel.h"

#include <QDebug>

IrcServerHandler::IrcServerHandler(Server *parent)
  : BasicHandler(parent) {
}

IrcServerHandler::~IrcServerHandler() {
}

/*! Handle a raw message string sent by the server. We try to find a suitable handler, otherwise we call a default handler. */
void IrcServerHandler::handleServerMsg(QByteArray rawmsg) {
  try {
    if(rawmsg.isEmpty()) {
      qWarning() << "Received empty string from server!";
      return;
    }
    // TODO Implement encoding conversion
    /* At this point, we have a raw message as a byte array. This needs to be converted to a QString somewhere.
     * Problem is, that at this point we don't know which encoding to use for the various parts of the message.
     * This is something the command handler needs to take care of (e.g. PRIVMSG needs to first parse for CTCP,
     * and then convert the raw strings into the correct encoding.
     * We _can_ safely assume Server encoding for prefix and cmd, but not for the params. Therefore, we need to
     * change from a QStringList to a QList<QByteArray> in all the handlers, and have the handlers call decodeString
     * where needed...
    */
    QString msg = QString::fromLatin1(rawmsg);

    // OK, first we split the raw message into its various parts...
    QString prefix = "";
    QString cmd;
    QStringList params;

    // a colon as the first chars indicates the existance of a prefix
    if(msg[0] == ':') {
      msg.remove(0,1);
      prefix = msg.section(' ', 0, 0);
      msg = msg.section(' ', 1);
    }

    // next string without a whitespace is the command
    cmd = msg.section(' ', 0, 0).toUpper();
    msg = msg.mid(cmd.length());

    // get the parameters
    QString trailing = "";
    if(msg.contains(" :")) {
      trailing = msg.section(" :", 1);
      msg = msg.section(" :", 0, 0);
    }
    if(!msg.isEmpty()) {
      params << msg.split(' ', QString::SkipEmptyParts);
    }
    if(!trailing.isEmpty()) {
      params << trailing;
    }

    // numeric replies have the target as first param (RFC 2812 - 2.4). this is usually our own nick. Remove this!
    uint num = cmd.toUInt();
    if(num > 0) {
      Q_ASSERT(params.count() > 0); // Violation to RFC
      params.removeFirst();
    }

    // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
    handle(cmd, Q_ARG(QString, prefix), Q_ARG(QStringList, params));
    //handle(cmd, Q_ARG(QString, prefix));
  } catch(Exception e) {
    emit displayMsg(Message::Error, "", e.msg());
  }
}


void IrcServerHandler::defaultHandler(QString cmd, QString prefix, QStringList params) {
  uint num = cmd.toUInt();
  if(num) {
    // A lot of server messages don't really need their own handler because they don't do much.
    // Catch and handle these here.
    switch(num) {
      // Welcome, status, info messages. Just display these.
      case 2: case 3: case 4: case 5: case 251: case 252: case 253: case 254: case 255: case 372: case 375:
        emit displayMsg(Message::Server, "", params.join(" "), prefix);
        break;
      // Server error messages without param, just display them
      case 409: case 411: case 412: case 422: case 424: case 445: case 446: case 451: case 462:
      case 463: case 464: case 465: case 466: case 472: case 481: case 483: case 485: case 491: case 501: case 502:
      case 431: // ERR_NONICKNAMEGIVEN 
        emit displayMsg(Message::Error, "", params.join(" "), prefix);
        break;
      // Server error messages, display them in red. First param will be appended.
      case 401: case 402: case 403: case 404: case 406: case 408: case 415: case 421: case 442:
      { QString p = params.takeFirst();
        emit displayMsg(Message::Error, "", params.join(" ") + " " + p, prefix);
        break;
      }
      // Server error messages which will be displayed with a colon between the first param and the rest
      case 413: case 414: case 423: case 441: case 444: case 461:
      case 467: case 471: case 473: case 474: case 475: case 476: case 477: case 478: case 482:
      case 436: // ERR_NICKCOLLISION
      { QString p = params.takeFirst();
        emit displayMsg(Message::Error, "", p + ": " + params.join(" "));
        break;
      }
      // Ignore these commands.
      case 366: case 376:
        break;

      // Everything else will be marked in red, so we can add them somewhere.
      default:
        emit displayMsg(Message::Error, "", cmd + " " + params.join(" "), prefix);
    }
    //qDebug() << prefix <<":"<<cmd<<params;
  } else {
    emit displayMsg(Message::Error, "", QString("Unknown: ") + cmd + " " + params.join(" "), prefix);
    //qDebug() << prefix <<":"<<cmd<<params;
  }
}

//******************************/
// IRC SERVER HANDLER
//******************************/
void IrcServerHandler::handleJoin(QString prefix, QStringList params) {
  Q_ASSERT(params.count() == 1);
  QString channel = params[0];
  IrcUser *ircuser = networkInfo()->updateNickFromMask(prefix);
  emit displayMsg(Message::Join, channel, channel, prefix);

  ircuser->joinChannel(channel);
}

void IrcServerHandler::handleKick(QString prefix, QStringList params) {
  networkInfo()->updateNickFromMask(prefix);
  IrcUser *victim = networkInfo()->ircUser(params[1]);
  QString channel = params[0];
  Q_ASSERT(victim);

  victim->partChannel(channel);

  QString msg;
  if(params.count() > 2) // someone got a reason!
    msg = QString("%1 %2").arg(victim->nick()).arg(params[2]);
  else
    msg = victim->nick();
  
  emit displayMsg(Message::Kick, params[0], msg, prefix);
}

void IrcServerHandler::handleMode(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  Q_UNUSED(params)
    
//   if(isChannelName(params[0])) {
//     // TODO only channel-user modes supported by now
//     QString prefixes = serverSupports["PrefixModes"].toString();
//     QString modes = params[1];
//     int p = 2;
//     int m = 0;
//     bool add = true;
//     while(m < modes.length()) {
//       if(modes[m] == '+') { add = true; m++; continue; }
//       if(modes[m] == '-') { add = false; m++; continue; }
//       if(prefixes.contains(modes[m])) {  // it's a user channel mode
//         Q_ASSERT(params.count() > m);
//         QString nick = params[p++];
//         if(nicks.contains(nick)) {  // sometimes, a server might try to set a MODE on a nick that is no longer there
//           QVariantMap n = nicks[nick]; QVariantMap clist = n["Channels"].toMap(); QVariantMap chan = clist[params[0]].toMap();
//           QString mstr = chan["Mode"].toString();
//           add ? mstr += modes[m] : mstr.remove(modes[m]);
//           chan["Mode"] = mstr; clist[params[0]] = chan; n["Channels"] = clist; nicks[nick] = n;
//           emit nickUpdated(network, nick, n);
//         }
//         m++;
//       } else {
//         // TODO add more modes
//         m++;
//       }
//     }
//     emit displayMsg(Message::Mode, params[0], params.join(" "), prefix);
//   } else {
//     //Q_ASSERT(nicks.contains(params[0]));
//     //QVariantMap n = nicks[params[0]].toMap();
//     //QString mode = n["Mode"].toString();
//     emit displayMsg(Message::Mode, "", params.join(" "));
//   }
}

void IrcServerHandler::handleNick(QString prefix, QStringList params) {
  IrcUser *ircuser = networkInfo()->updateNickFromMask(prefix);
  Q_ASSERT(ircuser);
  QString newnick = params[0];
  QString oldnick = ircuser->nick();

  foreach(QString channel, ircuser->channels()) {
    if(networkInfo()->isMyNick(oldnick)) {
      emit displayMsg(Message::Nick, channel, newnick, newnick);
    } else {
      emit displayMsg(Message::Nick, channel, newnick, prefix);
    }
  }
  ircuser->setNick(newnick);
}

void IrcServerHandler::handleNotice(QString prefix, QStringList params) {
  if(networkInfo()->currentServer().isEmpty() || networkInfo()->currentServer() == prefix)
    emit displayMsg(Message::Server, "", params[1], prefix);
  else
    emit displayMsg(Message::Notice, "", params[1], prefix);
}

void IrcServerHandler::handlePart(QString prefix, QStringList params) {
  IrcUser *ircuser = networkInfo()->updateNickFromMask(prefix);
  QString channel = params[0];
  Q_ASSERT(ircuser);
  
  ircuser->partChannel(channel);
  
  QString msg;
  if(params.count() > 1)
    msg = params[1];
  
  emit displayMsg(Message::Part, params[0], msg, prefix);
}

void IrcServerHandler::handlePing(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  emit putCmd("PONG", params);
}

void IrcServerHandler::handlePrivmsg(QString prefix, QStringList params) {
  networkInfo()->updateNickFromMask(prefix);
  if(params.count()<2)
    params << QString("");
  
  // it's possible to pack multiple privmsgs into one param using ctcp
  QStringList messages = server->ctcpHandler()->parse(CtcpHandler::CtcpQuery, prefix, params[0], params[1]);
  
  // are we the target or is it a channel?
  if(networkInfo()->isMyNick(params[0])) {
    foreach(QString message, messages) {
      if(!message.isEmpty()) {
	emit displayMsg(Message::Plain, "", message, prefix, Message::PrivMsg);
      }
    }
    
  } else {
    Q_ASSERT(isChannelName(params[0]));  // should be channel!
    foreach(QString message, messages) {
      if(!message.isEmpty()) {
	emit displayMsg(Message::Plain, params[0], message, prefix);
      }
    }
  }

}

void IrcServerHandler::handleQuit(QString prefix, QStringList params) {
  IrcUser *ircuser = networkInfo()->updateNickFromMask(prefix);
  Q_ASSERT(ircuser);

  QString msg;
  if(params.count())
    msg = params[0];
  
  foreach(QString channel, ircuser->channels())
    emit displayMsg(Message::Quit, channel, msg, prefix);

  ircuser->deleteLater();
}

void IrcServerHandler::handleTopic(QString prefix, QStringList params) {
  IrcUser *ircuser = networkInfo()->updateNickFromMask(prefix);
  QString channel = params[0];
  QString topic = params[1];
  Q_ASSERT(ircuser);

  networkInfo()->ircChannel(channel)->setTopic(topic);

  emit displayMsg(Message::Server, params[0], tr("%1 has changed topic for %2 to: \"%3\"").arg(ircuser->nick()).arg(channel).arg(topic));
}

/* RPL_WELCOME */
void IrcServerHandler::handle001(QString prefix, QStringList params) {
  // there should be only one param: "Welcome to the Internet Relay Network <nick>!<user>@<host>"
  QString myhostmask = params[0].section(' ', -1, -1);
  networkInfo()->setCurrentServer(prefix);
  networkInfo()->setMyNick(nickFromMask(myhostmask));

  emit displayMsg(Message::Server, "", params[0], prefix);

  // TODO: reimplement perform List!
  //// send performlist
  //QStringList performList = networkSettings["Perform"].toString().split( "\n" );
  //int count = performList.count();
  //for(int a = 0; a < count; a++) {
  //  if(!performList[a].isEmpty() ) {
  //    userInput(network, "", performList[a]);
  //  }
  //}
}

/* RPL_ISUPPORT */
// TODO Complete 005 handling, also use sensible defaults for non-sent stuff
void IrcServerHandler::handle005(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  QString rpl_isupport_suffix = params.takeLast();
  if(rpl_isupport_suffix.toLower() != QString("are supported by this server")) {
    qWarning() << "Received invalid RPL_ISUPPORT! Suffix is:" << rpl_isupport_suffix << "Excpected: are supported by this server";
    return;
  }
  
  foreach(QString param, params) {
    QString key = param.section("=", 0, 0);
    QString value = param.section("=", 1);
    networkInfo()->addSupport(key, value);
  }
}


/* RPL_NOTOPIC */
void IrcServerHandler::handle331(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  networkInfo()->ircChannel(params[0])->setTopic(QString());
  emit displayMsg(Message::Server, params[0], tr("No topic is set for %1.").arg(params[0]));
}

/* RPL_TOPIC */
void IrcServerHandler::handle332(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  networkInfo()->ircChannel(params[0])->setTopic(params[1]);
  emit displayMsg(Message::Server, params[0], tr("Topic for %1 is \"%2\"").arg(params[0]).arg(params[1]));
}

/* Topic set by... */
void IrcServerHandler::handle333(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  emit displayMsg(Message::Server, params[0], tr("Topic set by %1 on %2").arg(params[1]).arg(QDateTime::fromTime_t(params[2].toUInt()).toString()));
}

/* RPL_NAMREPLY */
void IrcServerHandler::handle353(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  params.removeFirst(); // either "=", "*" or "@" indicating a public, private or secret channel
  QString channelname = params.takeFirst();

  foreach(QString nick, params.takeFirst().split(' ')) {
    QString mode = QString();

    if(networkInfo()->prefixes().contains(nick[0])) {
      mode = networkInfo()->prefixToMode(nick[0]);
      nick = nick.mid(1);
    }
    
    IrcUser *ircuser = networkInfo()->newIrcUser(nick);
    ircuser->joinChannel(channelname);

    if(!mode.isNull())
      networkInfo()->ircChannel(channelname)->addUserMode(ircuser, mode);
  }
}

/* ERR_ERRONEUSNICKNAME */
void IrcServerHandler::handle432(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  Q_UNUSED(params)
  emit displayMsg(Message::Error, "", tr("Your desired nickname contains illegal characters!"));
  emit displayMsg(Message::Error, "", tr("Please use /nick <othernick> to continue your IRC-Session!"));
  // FIXME!

//   if(params.size() < 2) {
//     // handle unreal-ircd bug, where unreal ircd doesnt supply a TARGET in ERR_ERRONEUSNICKNAME during registration phase:
//     // nick @@@
//     // :irc.scortum.moep.net 432  @@@ :Erroneous Nickname: Illegal characters
//     // correct server reply:
//     // :irc.scortum.moep.net 432 * @@@ :Erroneous Nickname: Illegal characters
//     emit displayMsg(Message::Error, "", tr("There is a nickname in your identity's nicklist which contains illegal characters"));
//     emit displayMsg(Message::Error, "", tr("Due to a bug in Unreal IRCd (and maybe other irc-servers too) we're unable to determine the erroneous nick"));
//     emit displayMsg(Message::Error, "", tr("Please use: /nick <othernick> to continue or clean up your nicklist"));
//   } else {
//     QString errnick = params[0];
//     emit displayMsg(Message::Error, "", tr("Nick %1 contains illegal characters").arg(errnick));
//     // if there is a problem while connecting to the server -> we handle it
//     // TODO rely on another source...
//     if(currentServer.isEmpty()) {
//       QStringList desiredNicks = identity["NickList"].toStringList();
//       int nextNick = desiredNicks.indexOf(errnick) + 1;
//       if (desiredNicks.size() > nextNick) {
//         putCmd("NICK", QStringList(desiredNicks[nextNick]));
//       } else {
//         emit displayMsg(Message::Error, "", tr("No free and valid nicks in nicklist found. use: /nick <othernick> to continue"));
//       }
//     }
//   }
}

/* ERR_NICKNAMEINUSE */
void IrcServerHandler::handle433(QString prefix, QStringList params) {
  Q_UNUSED(prefix)
  QString errnick = params[0];
  emit displayMsg(Message::Error, "", tr("Nick %1 is already taken").arg(errnick));
  emit displayMsg(Message::Error, "", tr("Please use /nick <othernick> to continue your IRC-Session!"));
  // FIXME!
  
//   // if there is a problem while connecting to the server -> we handle it
//   // TODO rely on another source...
//   if(currentServer.isEmpty()) {
//     QStringList desiredNicks = identity["NickList"].toStringList();
//     int nextNick = desiredNicks.indexOf(errnick) + 1;
//     if (desiredNicks.size() > nextNick) {
//       putCmd("NICK", QStringList(desiredNicks[nextNick]));
//     } else {
//       emit displayMsg(Message::Error, "", tr("No free and valid nicks in nicklist found. use: /nick <othernick> to continue"));
//     }
//   }
}

/***********************************************************************************/



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

#include "util.h"
#include "global.h"
#include "server.h"
#include "message.h"

#include <QMetaObject>
#include <QDateTime>

Server::Server(QString net) : network(net) {

}

Server::~Server() {

}

void Server::run() {
  connect(&socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));

  exec();
}

void Server::sendState() {
  VarMap s;
  VarMap n, t;
  foreach(QString key, nicks.keys())  { n[key] = nicks[key]; }
  foreach(QString key, topics.keys()) { t[key] = topics[key];}
  s["Nicks"] = n;
  s["Topics"] = t;
  s["OwnNick"] = ownNick;
  s["ServerSupports"] = serverSupports;
  emit serverState(network, s);
}

void Server::connectToIrc(QString net) {
  if(net != network) return; // not me!
  networkSettings = global->getData("Networks").toMap()[net].toMap();
  identity = global->getData("Identities").toMap()[networkSettings["Identity"].toString()].toMap();
  QList<QVariant> servers = networkSettings["Servers"].toList();
  QString host = servers[0].toMap()["Address"].toString();
  quint16 port = servers[0].toMap()["Port"].toUInt();
  displayStatusMsg(QString("Connecting to %1:%2...").arg(host).arg(port));
  socket.connectToHost(host, port);
}

void Server::disconnectFromIrc(QString net) {
  if(net != network) return; // not me!
  socket.disconnectFromHost();
}

void Server::socketHasData() {
  while(socket.canReadLine()) {
    QString s = socket.readLine().trimmed();
    qDebug() << "Read" << s;
    emit recvRawServerMsg(s);
    //Message *msg = Message::createFromServerString(this, s);
    handleServerMsg(s);
  }
}

void Server::socketError( QAbstractSocket::SocketError err ) {
  //qDebug() << "Socket Error!";
  //emit error(err);
}

void Server::socketConnected( ) {
  emit connected(network);
  putRawLine(QString("NICK :%1").arg(identity["NickList"].toStringList()[0]));
  putRawLine(QString("USER %1 8 * :%2").arg(identity["Ident"].toString()).arg(identity["RealName"].toString()));
}

void Server::socketDisconnected( ) {
  //qDebug() << "Socket disconnected!";
  emit disconnected(network);
  topics.clear();
  nicks.clear();
}

void Server::socketStateChanged(QAbstractSocket::SocketState state) {
  //qDebug() << "Socket state changed: " << state;
}

QString Server::updateNickFromMask(QString mask) {
  QString user = userFromMask(mask);
  QString host = hostFromMask(mask);
  QString nick = nickFromMask(mask);
  if(nicks.contains(nick) && !user.isEmpty() && !host.isEmpty()) {
    VarMap n = nicks[nick];
    if(n["User"].toString() != user || n["Host"].toString() != host) {
      if(!n["User"].toString().isEmpty() || !n["Host"].toString().isEmpty())
        qWarning(QString("Strange: Hostmask for nick %1 has changed!").arg(nick).toAscii());
      n["User"] = user; n["Host"] = host;
      nicks[nick] = n;
      emit nickUpdated(network, nick, n);
    }
  }
  return nick;
}

void Server::userInput(QString net, QString buf, QString msg) {
  if(net != network) return; // not me!
  //msg = msg.trimmed(); // remove whitespace from start and end
  if(msg.isEmpty()) return;
  if(!msg.startsWith('/')) {
    msg = QString("/SAY ") + msg;
  }
  handleUserInput(buf, msg);
}

void Server::putRawLine(QString s) {
  qDebug() << "SentRaw: " << s;
  s += "\r\n";
  socket.write(s.toAscii());
}

void Server::putCmd(QString cmd, QStringList params, QString prefix) {
  QString m;
  if(!prefix.isEmpty()) m += ":" + prefix + " ";
  m += cmd.toUpper();
  for(int i = 0; i < params.size() - 1; i++) {
    m += " " + params[i];
  }
  if(!params.isEmpty()) m += " :" + params.last();
  qDebug() << "Sent: " << m;
  m += "\r\n";
  socket.write(m.toAscii());
}

/** Handle a raw message string sent by the server. We try to find a suitable handler, otherwise we call a default handler. */
void Server::handleServerMsg(QString msg) {
  try {
    if(msg.isEmpty()) {
      qWarning() << "Received empty string from server!";
      return;
    }
    // OK, first we split the raw message into its various parts...
    QString prefix;
    QString cmd;
    QStringList params;
    if(msg[0] == ':') {
      msg.remove(0,1);
      prefix = msg.section(' ', 0, 0);
      msg = msg.section(' ', 1);
    }
    cmd = msg.section(' ', 0, 0).toUpper();
    msg = msg.section(' ', 1);
    QString left, trailing;
    // RPL_ISUPPORT (005) can contain colons, so don't treat it like the rest of the commands
    if(cmd.toUInt() == 5) {
      left = msg.remove(QString(":are supported by this server"));
    } else {
      left = msg.section(':', 0, 0);
      trailing = msg.section(':', 1);
    }
    if(!left.isEmpty()) {
      params << left.split(' ', QString::SkipEmptyParts);
    }
    if(!trailing.isEmpty()) {
      params << trailing;
    }
    // numeric replies usually have our own nick as first param. Remove this!
    // BTW, this behavior is not in the RFC.
    uint num = cmd.toUInt();
    if(num > 1 && params.count() > 0) {  // 001 sets our nick, so we shouldn't remove anything
      if(params[0] == ownNick) params.removeFirst();
      else qWarning((QString("First param NOT nick: %1:%2 %3").arg(prefix).arg(cmd).arg(params.join(" "))).toAscii());
    }
    // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
    QString hname = cmd.toLower();
    hname[0] = hname[0].toUpper();
    hname = "handleServer" + hname;
    if(!QMetaObject::invokeMethod(this, hname.toAscii(), Q_ARG(QString, prefix), Q_ARG(QStringList, params))) {
      // Ok. Default handler it is.
      defaultServerHandler(cmd, prefix, params);
    }
  } catch(Exception e) {
    emit displayMsg(Message::error("", e.msg()));
  }
}

void Server::defaultServerHandler(QString cmd, QString prefix, QStringList params) {
  uint num = cmd.toUInt();
  if(num) {
    // A lot of server messages don't really need their own handler because they don't do much.
    // Catch and handle these here.
    switch(num) {
      // Welcome, status, info messages. Just display these.
      case 2: case 3: case 4: case 5: case 251: case 252: case 253: case 254: case 255: case 372: case 375:
        emit displayMsg(Message::server("", params.join(" "), prefix));
        break;
      // Server error messages without param, just display them
      case 409: case 411: case 412: case 422: case 424: case 431: case 445: case 446: case 451: case 462:
      case 463: case 464: case 465: case 466: case 472: case 481: case 483: case 485: case 491: case 501: case 502:
        emit displayMsg(Message::error("", params.join(" "), prefix));
        break;
      // Server error messages, display them in red. First param will be appended.
      case 401: case 402: case 403: case 404: case 406: case 408: case 415: case 421: case 432: case 442:
      { QString p = params.takeFirst();
        emit displayMsg(Message::error("", params.join(" ") + " " + p, prefix));
        break;
      }
      // Server error messages which will be displayed with a colon between the first param and the rest
      case 413: case 414: case 423: case 433: case 436: case 441: case 444: case 461:
      case 467: case 471: case 473: case 474: case 475: case 476: case 477: case 478: case 482:
      { QString p = params.takeFirst();
        emit displayMsg(Message::error("", p + ": " + params.join(" ")));
        break;
      }
      // Ignore these commands.
      case 366: case 376:
        break;

      // Everything else will be marked in red, so we can add them somewhere.
      default:
        emit displayMsg(Message::error("", cmd + " " + params.join(" "), prefix));
    }
    //qDebug() << prefix <<":"<<cmd<<params;
  } else {
    emit displayMsg(Message::error("", QString("Unknown: ") + cmd + " " + params.join(" "), prefix));
    //qDebug() << prefix <<":"<<cmd<<params;
  }
}

void Server::handleUserInput(QString bufname, QString usrMsg) {
  try {
    /* Looks like we don't need core-side buffers...
    Buffer *buffer = 0;
    if(!bufname.isEmpty()) {
      Q_ASSERT(buffers.contains(bufname));
      buffer = buffers[bufname];
    }
    */
    QString cmd = usrMsg.section(' ', 0, 0).remove(0, 1).toUpper();
    QString msg = usrMsg.section(' ', 1);
    QString hname = cmd.toLower();
    hname[0] = hname[0].toUpper();
    hname = "handleUser" + hname;
    if(!QMetaObject::invokeMethod(this, hname.toAscii(), Q_ARG(QString, bufname), Q_ARG(QString, msg))) {
        // Ok. Default handler it is.
      defaultUserHandler(bufname, cmd, msg);
    }
  } catch(Exception e) {
    emit displayMsg(Message::error("", e.msg()));
  }
}

void Server::defaultUserHandler(QString bufname, QString cmd, QString msg) {
  emit displayMsg(Message::error("", QString("Error: %1 %2").arg(cmd).arg(msg)));

}

/**********************************************************************************/

/*
void Server::handleUser(QString bufname, QString msg) {


}
*/

void Server::handleUserAway(QString bufname, QString msg) {
  putCmd("AWAY", QStringList(msg));
}

void Server::handleUserDeop(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufname << m << nicks;
  putCmd("MODE", params);
}

void Server::handleUserDevoice(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufname << m << nicks;
  putCmd("MODE", params);
}

void Server::handleUserInvite(QString bufname, QString msg) {
  QStringList params;
  params << msg << bufname;
  putCmd("INVITE", params);
}

void Server::handleUserJoin(QString bufname, QString msg) {
  putCmd("JOIN", QStringList(msg));
}

void Server::handleUserKick(QString bufname, QString msg) {
  QStringList params;
  params << bufname << msg.split(' ', QString::SkipEmptyParts);
  putCmd("KICK", params);
}

void Server::handleUserList(QString bufname, QString msg) {
  putCmd("LIST", msg.split(' ', QString::SkipEmptyParts));
}

void Server::handleUserMode(QString bufname, QString msg) {
  putCmd("MODE", msg.split(' ', QString::SkipEmptyParts));
}

void Server::handleUserMsg(QString bufname, QString msg) {
  QString nick = msg.section(" ", 0, 0);
  msg = msg.section(" ", 1);
  if(nick.isEmpty() || msg.isEmpty()) return;
  QStringList params;
  params << nick << msg;
  putCmd("PRIVMSG", params);
}

void Server::handleUserNick(QString bufname, QString msg) {
  QString nick = msg.section(' ', 0, 0);
  putCmd("NICK", QStringList(nick));
}

void Server::handleUserOp(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufname << m << nicks;
  putCmd("MODE", params);
}

void Server::handleUserPart(QString bufname, QString msg) {
  QStringList params;
  params << bufname << msg;
  putCmd("PART", params);
}

void Server::handleUserQuit(QString bufname, QString msg) {
  putCmd("QUIT", QStringList(msg));
}

void Server::handleUserQuote(QString bufname, QString msg) {
  putRawLine(msg);
}

void Server::handleUserSay(QString bufname, QString msg) {
  if(bufname.isEmpty()) return;  // server buffer
  QStringList params;
  params << bufname << msg;
  putCmd("PRIVMSG", params);
  emit displayMsg(Message::plain(params[0], msg, ownNick, Message::Self));
}

void Server::handleUserTopic(QString bufname, QString msg) {
  if(bufname.isEmpty()) return;
  QStringList params;
  params << bufname << msg;
  putCmd("TOPIC", params);
}

void Server::handleUserVoice(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufname << m << nicks;
  putCmd("MODE", params);
}

/**********************************************************************************/

void Server::handleServerJoin(QString prefix, QStringList params) {
  Q_ASSERT(params.count() == 1);
  QString nick = updateNickFromMask(prefix);
  if(nick == ownNick) {
  //  Q_ASSERT(!buffers.contains(params[0]));  // cannot join a buffer twice!
  //  Buffer *buf = new Buffer(params[0]);
  //  buffers[params[0]] = buf;
    topics[params[0]] = "";
    emit topicSet(network, params[0], "");
  } //else {
    VarMap n;
    if(nicks.contains(nick)) {
      n = nicks[nick];
      VarMap chans = n["Channels"].toMap();
      // Q_ASSERT(!chans.keys().contains(params[0])); TODO uncomment
      chans[params[0]] = VarMap();
      n["Channels"] = chans;
      nicks[nick] = n;
      emit nickUpdated(network, nick, n);
    } else {
      VarMap chans;
      chans[params[0]] = VarMap();
      n["Channels"] = chans;
      n["User"] = userFromMask(prefix);
      n["Host"] = hostFromMask(prefix);
      nicks[nick] = n;
      emit nickAdded(network, nick, n);
    }
    emit displayMsg(Message::join(params[0], params[0], prefix));
  //}
}

void Server::handleServerKick(QString prefix, QStringList params) {
  QString kicker = updateNickFromMask(prefix);
  QString nick = params[1];
  Q_ASSERT(nicks.contains(nick));
  VarMap n = nicks[nick];
  VarMap chans = n["Channels"].toMap();
  Q_ASSERT(chans.contains(params[0]));
  chans.remove(params[0]);
  QString msg = nick;
  if(params.count() > 2) msg = QString("%1 %2").arg(msg).arg(params[2]);
  emit displayMsg(Message::kick(params[0], msg, prefix));
  if(chans.count() > 0) {
    n["Channels"] = chans;
    nicks[nick] = n;
    emit nickUpdated(network, nick, n);
  } else {
    nicks.remove(nick);
    emit nickRemoved(network, nick);
  }
  if(nick == ownNick) {
    topics.remove(params[0]);
  }
}

void Server::handleServerMode(QString prefix, QStringList params) {
  if(isChannelName(params[0])) {
    // TODO only channel-user modes supported by now
    QString prefixes = serverSupports["PrefixModes"].toString();
    QString modes = params[1];
    int p = 2;
    int m = 0;
    bool add = true;
    while(m < modes.length()) {
      if(modes[m] == '+') { add = true; m++; continue; }
      if(modes[m] == '-') { add = false; m++; continue; }
      if(prefixes.contains(modes[m])) {  // it's a user channel mode
        Q_ASSERT(params.count() > m);
        QString nick = params[p++];
        if(nicks.contains(nick)) {  // sometimes, a server might try to set a MODE on a nick that is no longer there
          VarMap n = nicks[nick]; VarMap clist = n["Channels"].toMap(); VarMap chan = clist[params[0]].toMap();
          QString mstr = chan["Mode"].toString();
          add ? mstr += modes[m] : mstr.remove(modes[m]);
          chan["Mode"] = mstr; clist[params[0]] = chan; n["Channels"] = clist; nicks[nick] = n;
          emit nickUpdated(network, nick, n);
        }
        m++;
      } else {
        // TODO add more modes
        m++;
      }
    }
    emit displayMsg(Message::mode(params[0], params.join(" "), prefix));
  } else {
    //Q_ASSERT(nicks.contains(params[0]));
    //VarMap n = nicks[params[0]].toMap();
    //QString mode = n["Mode"].toString();
    emit displayMsg(Message::mode("", params.join(" ")));
  }
}

void Server::handleServerNick(QString prefix, QStringList params) {
  QString oldnick = updateNickFromMask(prefix);
  QString newnick = params[0];
  VarMap v = nicks.take(oldnick);
  nicks[newnick] = v;
  VarMap chans = v["Channels"].toMap();
  foreach(QString c, chans.keys()) {
    if(oldnick != ownNick) { emit displayMsg(Message::nick(c, newnick, prefix)); }
    else { emit displayMsg(Message::nick(c, newnick, newnick)); }
  }
  emit nickRenamed(network, oldnick, newnick);
  if(oldnick == ownNick) {
    ownNick = newnick;
    emit ownNickSet(network, newnick);
  }
}

void Server::handleServerNotice(QString prefix, QStringList params) {
  //Message msg(Message::Notice, params[1], prefix);
  if(prefix == currentServer) emit displayMsg(Message::server("", params[1], prefix));
  else emit displayMsg(Message::notice("", params[1], prefix));
}

void Server::handleServerPart(QString prefix, QStringList params) {
  QString nick = updateNickFromMask(prefix);
  Q_ASSERT(nicks.contains(nick));
  VarMap n = nicks[nick];
  VarMap chans = n["Channels"].toMap();
  Q_ASSERT(chans.contains(params[0]));
  chans.remove(params[0]);
  QString msg;
  if(params.count() > 1) msg = params[1];
  emit displayMsg(Message::part(params[0], msg, prefix));
  if(chans.count() > 0) {
    n["Channels"] = chans;
    nicks[nick] = n;
    emit nickUpdated(network, nick, n);
  } else {
    nicks.remove(nick);
    emit nickRemoved(network, nick);
  }
  if(nick == ownNick) {
    Q_ASSERT(topics.contains(params[0]));
    topics.remove(params[0]);
  }
}

void Server::handleServerPing(QString prefix, QStringList params) {
  putCmd("PONG", params);
}

void Server::handleServerPrivmsg(QString prefix, QStringList params) {
  updateNickFromMask(prefix);
  if(params.count()<2) emit displayMsg(Message::plain(params[0], "", prefix));
  else emit displayMsg(Message::plain(params[0], params[1], prefix));
}

void Server::handleServerQuit(QString prefix, QStringList params) {
  QString nick = updateNickFromMask(prefix);
  Q_ASSERT(nicks.contains(nick));
  VarMap chans = nicks[nick]["Channels"].toMap();
  foreach(QString c, chans.keys()) {
    emit displayMsg(Message::quit(c, params[0], prefix));
  }
  nicks.remove(nick);
  emit nickRemoved(network, nick);
}

void Server::handleServerTopic(QString prefix, QStringList params) {
  QString nick = updateNickFromMask(prefix);
  Q_ASSERT(nicks.contains(nick));
  topics[params[0]] = params[1];
  emit topicSet(network, params[0], params[1]);
  emit displayMsg(Message::server(params[0], tr("%1 has changed topic for %2 to: \"%3\"").arg(nick).arg(params[0]).arg(params[1])));
}

/* RPL_WELCOME */
void Server::handleServer001(QString prefix, QStringList params) {
  currentServer = prefix;
  ownNick = params[0];
  VarMap n;
  n["Channels"] = VarMap();
  nicks[ownNick] = n;
  emit ownNickSet(network, ownNick);
  emit nickAdded(network, ownNick, VarMap());
  emit displayMsg(Message::server("", params[1], prefix));
  // send performlist
  QStringList performList = networkSettings["Perform"].toString().split( "\n" );
  int count = performList.count();
  for ( int a = 0; a < count; a++ ) {
  	if ( !performList[a].isEmpty() ) {
  		userInput( network, "", performList[a] ); 
  	}
  }
}

/* RPL_ISUPPORT */
void Server::handleServer005(QString prefix, QStringList params) {
  foreach(QString p, params) {
    QString key = p.section("=", 0, 0);
    QString val = p.section("=", 1);
    serverSupports[key] = val;
    // handle some special cases
    if(key == "PREFIX") {
      VarMap foo; QString modes, prefixes;
      Q_ASSERT(val.contains(')') && val.startsWith('('));
      int m = 1, p;
      for(p = 2; p < val.length(); p++) if(val[p] == ')') break;
      p++;
      for(; val[m] != ')'; m++, p++) {
        Q_ASSERT(p < val.length());
        foo[QString(val[m])] = QString(val[p]);
        modes += val[m]; prefixes += val[p];
      }
      serverSupports["PrefixModes"] = modes; serverSupports["Prefixes"] = prefixes;
      serverSupports["ModePrefixMap"] = foo;
    }
  }
}


/* RPL_NOTOPIC */
void Server::handleServer331(QString prefix, QStringList params) {
  topics[params[0]] = "";
  emit topicSet(network, params[0], "");
  emit displayMsg(Message::server(params[0], tr("No topic is set for %1.").arg(params[0])));
}

/* RPL_TOPIC */
void Server::handleServer332(QString prefix, QStringList params) {
  topics[params[0]] = params[1];
  emit topicSet(network, params[0], params[1]);
  emit displayMsg(Message::server(params[0], tr("Topic for %1 is \"%2\"").arg(params[0]).arg(params[1])));
}

/* Topic set by... */
void Server::handleServer333(QString prefix, QStringList params) {
  emit displayMsg(Message::server(params[0],
                  tr("Topic set by %1 on %2").arg(params[1]).arg(QDateTime::fromTime_t(params[2].toUInt()).toString())));
}

/* RPL_NAMREPLY */
void Server::handleServer353(QString prefix, QStringList params) {
  params.removeFirst(); // = or *
  QString buf = params.takeFirst();
  QString prefixes = serverSupports["Prefixes"].toString();
  foreach(QString nick, params[0].split(' ')) {
    QString mode = "", pfx = "";
    if(prefixes.contains(nick[0])) {
      pfx = nick[0];
      for(int i = 0;; i++)
        if(prefixes[i] == nick[0]) { mode = serverSupports["PrefixModes"].toString()[i]; break; }
      nick.remove(0,1);
    }
    VarMap c; c["Mode"] = mode; c["Prefix"] = pfx;
    if(nicks.contains(nick)) {
      VarMap n = nicks[nick];
      VarMap chans = n["Channels"].toMap();
      chans[buf] = c;
      n["Channels"] = chans;
      nicks[nick] = n;
      emit nickUpdated(network, nick, n);
    } else {
      VarMap n; VarMap c; VarMap chans;
      c["Mode"] = mode;
      chans[buf] = c;
      n["Channels"] = chans;
      nicks[nick] = n;
      emit nickAdded(network, nick, n);
    }
  }
}
/***********************************************************************************/

/* Exception classes for message handling */
Server::ParseError::ParseError(QString cmd, QString prefix, QStringList params) {
  _msg = QString("Command Parse Error: ") + cmd + params.join(" ");

}

Server::UnknownCmdError::UnknownCmdError(QString cmd, QString prefix, QStringList params) {
  _msg = QString("Unknown Command: ") + cmd;

}

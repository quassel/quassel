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
#include "cmdcodes.h"
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
    qDebug() << "Read: " << s;
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
  putRawLine(QString("NICK :%1").arg(identity["NickList"].toStringList()[0]));
  putRawLine(QString("USER %1 8 * :%2").arg(identity["Ident"].toString()).arg(identity["RealName"].toString()));
}

void Server::socketDisconnected( ) {
  //qDebug() << "Socket disconnected!";
  emit disconnected();
}

void Server::socketStateChanged(QAbstractSocket::SocketState state) {
  //qDebug() << "Socket state changed: " << state;
}

QString Server::updateNickFromMask(QString mask) {
  QString user = userFromMask(mask);
  QString host = hostFromMask(mask);
  QString nick = nickFromMask(mask);
  if(nicks.contains(nick) && !user.isEmpty() && !host.isEmpty()) {
    VarMap n = nicks[nick].toMap();
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
  msg = msg.trimmed(); // remove whitespace from start and end
  if(msg.isEmpty()) return;
  if(!msg.startsWith('/')) {
    msg = QString("/SAY ") + msg;
  }
  handleUserMsg(buf, msg);
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
  qDebug() << "SentCmd: " << m;
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
    QString left = msg.section(':', 0, 0);
    QString trailing = msg.section(':', 1);
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
      if(params[0] == currentNick) params.removeFirst();
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
    emit displayMsg("", Message(Message::Error, e.msg()));
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
        emit displayMsg("", Message(Message::Server, params.join(" "), prefix));
        break;
      // Ignore these commands.
      case 366: case 376:
        break;

      // Everything else will be marked in red, so we can add them somewhere.
      default:
        emit displayMsg("", Message(Message::Error, cmd + " " + params.join(" "), prefix));
    }
    //qDebug() << prefix <<":"<<cmd<<params;
  } else {
    emit displayMsg("", Message(Message::Error, QString("Unknown: ") + cmd + " " + params.join(" "), prefix));
    //qDebug() << prefix <<":"<<cmd<<params;
  }
}

void Server::handleUserMsg(QString bufname, QString usrMsg) {
  try {
    Buffer *buffer = 0;
    if(!bufname.isEmpty()) {
      Q_ASSERT(buffers.contains(bufname));
      buffer = buffers[bufname];
    }
    QString cmd = usrMsg.section(' ', 0, 0).remove(0, 1).toUpper();
    QString msg = usrMsg.section(' ', 1).trimmed();
    QString hname = cmd.toLower();
    hname[0] = hname[0].toUpper();
    hname = "handleUser" + hname;
    if(!QMetaObject::invokeMethod(this, hname.toAscii(), Q_ARG(QString, msg), Q_ARG(Buffer*, buffer))) {
        // Ok. Default handler it is.
      defaultUserHandler(cmd, msg, buffer);
    }
  } catch(Exception e) {
    emit displayMsg("", Message(Message::Error, e.msg()));
  }
}

void Server::defaultUserHandler(QString cmd, QString msg, Buffer *buf) {
  emit displayMsg("", Message(Message::Error, QString("Error: %1 %2").arg(cmd).arg(msg)));

}

/**********************************************************************************/

/*
void Server::handleUser(QString msg, Buffer *buf) {


}
*/

void Server::handleUserJoin(QString msg, Buffer *buf) {
  putCmd("JOIN", QStringList(msg));

}

void Server::handleUserQuote(QString msg, Buffer *buf) {
  putRawLine(msg);
}

void Server::handleUserSay(QString msg, Buffer *buf) {
  if(!buf) return;  // server buffer
  QStringList params;
  params << buf->name() << msg;
  putCmd("PRIVMSG", params);
  emit displayMsg(params[0], Message(Message::Msg, msg, currentNick, Message::Self));
}

/**********************************************************************************/

void Server::handleServerJoin(QString prefix, QStringList params) {
  Q_ASSERT(params.count() == 1);
  QString nick = updateNickFromMask(prefix);
  if(nick == currentNick) {
    Q_ASSERT(!buffers.contains(params[0]));  // cannot join a buffer twice!
    Buffer *buf = new Buffer(params[0]);
    buffers[params[0]] = buf;
  } else {
    VarMap n;
    if(nicks.contains(nick)) {
      n = nicks[nick].toMap();
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
    emit displayMsg(params[0], Message(Message::Join, params[0], prefix));
  }
}

void Server::handleServerKick(QString prefix, QStringList params) {
  QString kicker = updateNickFromMask(prefix);
  QString nick = params[1];
  Q_ASSERT(nicks.contains(nick));
  VarMap n = nicks[nick].toMap();
  VarMap chans = n["Channels"].toMap();
  Q_ASSERT(chans.contains(params[0]));
  chans.remove(params[0]);
  QString msg = nick;
  if(params.count() > 2) msg = QString("%1 %2").arg(msg).arg(params[2]);
  emit displayMsg(params[0], Message(Message::Kick, msg, prefix));
  if(chans.count() > 0) {
    n["Channels"] = chans;
    nicks[nick] = n;
    emit nickUpdated(network, nick, n);
  } else {
    nicks.remove(nick);
    emit nickRemoved(network, nick);
  }
}

void Server::handleServerNick(QString prefix, QStringList params) {
  QString oldnick = updateNickFromMask(prefix);
  QString newnick = params[0];
  QVariant v = nicks.take(oldnick);
  nicks[newnick] = v;
  VarMap chans = v.toMap()["Channels"].toMap();
  foreach(QString c, chans.keys()) {
    if(oldnick != currentNick) { emit displayMsg(c, Message(Message::Nick, newnick, prefix)); }
    else { emit displayMsg(c, Message(Message::Nick, newnick, newnick)); }
  }
  emit nickRenamed(network, oldnick, newnick);
  if(oldnick == currentNick) {
    currentNick == newnick;
    emit ownNickSet(network, newnick);
  }
}

void Server::handleServerNotice(QString prefix, QStringList params) {
  //Message msg(Message::Notice, params[1], prefix);
  if(prefix == currentServer) emit displayMsg("", Message(Message::Server, params[1], prefix));
  else emit displayMsg("", Message(Message::Notice, params[1], prefix));
}

void Server::handleServerPart(QString prefix, QStringList params) {
  QString nick = updateNickFromMask(prefix);
  Q_ASSERT(nicks.contains(nick));
  VarMap n = nicks[nick].toMap();
  VarMap chans = n["Channels"].toMap();
  Q_ASSERT(chans.contains(params[0]));
  chans.remove(params[0]);
  QString msg;
  if(params.count() > 1) msg = params[1];
  emit displayMsg(params[0], Message(Message::Part, msg, prefix));
  if(chans.count() > 0) {
    n["Channels"] = chans;
    nicks[nick] = n;
    emit nickUpdated(network, nick, n);
  } else {
    nicks.remove(nick);
    emit nickRemoved(network, nick);
  }
}

void Server::handleServerPing(QString prefix, QStringList params) {
  putCmd("PONG", params);
}

void Server::handleServerPrivmsg(QString prefix, QStringList params) {
  updateNickFromMask(prefix);
  emit displayMsg(params[0], Message(Message::Msg, params[1], prefix));

}

void Server::handleServerQuit(QString prefix, QStringList params) {
  QString nick = updateNickFromMask(prefix);
  Q_ASSERT(nicks.contains(nick));
  VarMap chans = nicks[nick].toMap()["Channels"].toMap();
  foreach(QString c, chans.keys()) {
    emit displayMsg(c, Message(Message::Quit, params[0], prefix));
  }
  nicks.remove(nick);
  emit nickRemoved(network, nick);
}

/* RPL_WELCOME */
void Server::handleServer001(QString prefix, QStringList params) {
  currentServer = prefix;
  currentNick = params[0];
  emit ownNickSet(network, currentNick);
  emit displayMsg("", Message(Message::Server, params[1], prefix));
}

/* RPL_NOTOPIC */
void Server::handleServer331(QString prefix, QStringList params) {
  emit topicSet(network, params[0], "");
}

/* RPL_TOPIC */
void Server::handleServer332(QString prefix, QStringList params) {
  emit topicSet(network, params[0], params[1]);
  emit displayMsg(params[0], Message(Message::Server, tr("Topic for %1 is \"%2\"").arg(params[0]).arg(params[1])));
}

/* Topic set by... */
void Server::handleServer333(QString prefix, QStringList params) {
  emit displayMsg(params[0], Message(Message::Server,
                  tr("Topic set by %1 on %2").arg(params[1]).arg(QDateTime::fromTime_t(params[2].toUInt()).toString())));
}

/* RPL_NAMREPLY */
void Server::handleServer353(QString prefix, QStringList params) {
  params.removeFirst(); // = or *
  QString buf = params.takeFirst();
  foreach(QString nick, params[0].split(' ')) {
    // TODO: parse more prefix characters! use 005?
    QString mode = "";
    if(nick.startsWith('@')) { mode = "o"; nick.remove(0,1); }
    else if(nick.startsWith('+')) { mode = "v"; nick.remove(0,1); }
    VarMap c; c["Mode"] = mode;
    if(nicks.contains(nick)) {
      VarMap n = nicks[nick].toMap();
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
      n["Nick"] = nick;
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

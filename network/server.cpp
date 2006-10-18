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

#include "quassel.h"
#include "server.h"
#include "cmdcodes.h"

#include <QMetaObject>

Server::Server() {

}

Server::~Server() {

}

void Server::init() {
  //Message::init(&dispatchServerMsg, &dispatchUserMsg);
}

void Server::run() {
  connect(&socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));

  exec();
}

void Server::connectToIrc( const QString & host, quint16 port ) {
  qDebug() << "Connecting...";
  socket.connectToHost(host, port);
}

void Server::disconnectFromIrc( ) {
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
  qDebug() << "Socket Error!";
  //emit error(err);
}

void Server::socketConnected( ) {
  qDebug() << "Socket connected!";
  putRawLine("NICK :Sput|QuasselDev");
  putRawLine("USER Sputnick 8 * :Using Quassel IRC (WiP Version)");
}

void Server::socketDisconnected( ) {
  qDebug() << "Socket disconnected!";
  //emit disconnected();
}

void Server::socketStateChanged(QAbstractSocket::SocketState state) {
  qDebug() << "Socket state changed: " << state;
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
    // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
    QString hname = cmd.toLower();
    hname[0] = hname[0].toUpper();
    hname = "handle" + hname + "FromServer";
    if(!QMetaObject::invokeMethod(this, hname.toAscii(), Q_ARG(QString, prefix), Q_ARG(QStringList, params))) {
      // Ok. Default handler it is.
      defaultHandlerForServer(cmd, prefix, params);
    }
  } catch(Exception e) {
    emit recvLine(e.msg());
  }
}

void Server::defaultHandlerForServer(QString cmd, QString prefix, QStringList params) {
  uint num = cmd.toUInt();
  if(num) {
    recvLine(cmd + " " + params.join(" "));
  } else {
    recvLine(QString("Unknown: ") + cmd + " " + params.join(" "));
  }
}

void Server::handleUserMsg(QString usrMsg) {

}

/*
void Server::handleServerMsg(Message *msg) {
  int cmdCode = msg->getCmdCode();
  QString prefix = msg->getPrefix();
  QStringList params = msg->getParams();
  if(cmdCode < 0) {
    switch(-cmdCode) {
      case CMD_PING:
        // PING <server1> [<server2>]
        if(params.size() < 1 || params.size() > 2) throw ParseError(msg);
        putCmd("PONG", params);
        break;

      default:
        throw Exception(QString("No handler installed for command: ") + msg->getCmd() + " " + msg->getParams().join(" "));
    }
  } else if(msg->getCmdCode() > 0) {
    switch(msg->getCmdCode()) {

      default:
        //
        throw Exception(msg->getCmd() + " " + msg->getParams().join(" "));
    }

  } else {
    throw UnknownCmdError(msg);
  }
}
*/

void Server::handleNoticeFromServer(QString prefix, QStringList params) {
  recvLine(params.join(" "));


}

void Server::handlePingFromServer(QString prefix, QStringList params) {
  putCmd("PONG", params);
}

/* Exception classes for message handling */
Server::ParseError::ParseError(QString cmd, QString prefix, QStringList params) {
  _msg = QString("Command Parse Error: ") + cmd + params.join(" ");

}

Server::UnknownCmdError::UnknownCmdError(QString cmd, QString prefix, QStringList params) {
  _msg = QString("Unknown Command: ") + cmd;

}

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

#include "global.h"
#include "server.h"
#include "cmdcodes.h"

#include <QMetaObject>

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
  QList<QVariant> servers = global->getData("Networks").toMap()[net].toMap()["Servers"].toList();
  qDebug() << "Connecting to"<< servers[0].toMap();
  QString host = servers[0].toMap()["Address"].toString();
  quint16 port = servers[0].toMap()["Port"].toUInt();
  sendStatusMsg(QString("Connecting to %1:%2...").arg(host).arg(port));
  socket.connectToHost(host, port);
}

void Server::disconnectFromIrc(QString net) {
  if(net != network) return; // not me!
  socket.disconnectFromHost();
}

void Server::socketHasData() {
  while(socket.canReadLine()) {
    QString s = socket.readLine().trimmed();
    //qDebug() << "Read: " << s;
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
  putRawLine("NICK :QuasselDev");
  putRawLine("USER Sputnick 8 * :Using Quassel IRC (WiP Version)");
}

void Server::socketDisconnected( ) {
  qDebug() << "Socket disconnected!";
  //emit disconnected();
}

void Server::socketStateChanged(QAbstractSocket::SocketState state) {
  qDebug() << "Socket state changed: " << state;
}

void Server::userInput(QString net, QString buf, QString msg) {
  putRawLine(msg);
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
    emit sendStatusMsg(e.msg());
  }
}

void Server::defaultHandlerForServer(QString cmd, QString prefix, QStringList params) {
  uint num = cmd.toUInt();
  if(num) {
    emit sendMessage("", cmd + " " + params.join(" "));
  } else {
    emit sendMessage("", QString("Unknown: ") + cmd + " " + params.join(" "));
  }
}

void Server::handleUserMsg(QString usrMsg) {

}

void Server::handleNoticeFromServer(QString prefix, QStringList params) {
  sendMessage("", params.join(" "));


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

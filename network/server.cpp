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

Server::Server() {
  socket = new QTcpSocket();

}

Server::~Server() {
  delete socket;
}

void Server::init() {
  Message::init(&dispatchServerMsg, &dispatchUserMsg);
}

void Server::run() {
  connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
  connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));

  stream.setDevice(socket);
  //connectToIrc("irc.quakenet.org", 6667);
  exec();
}

/*
QAbstractSocket::SocketState TcpConnection::state( ) const {
  return socket.state();
}
*/

void Server::connectToIrc( const QString & host, quint16 port ) {
  qDebug() << "Connecting...";
  socket->connectToHost(host, port);
}

void Server::disconnectFromIrc( ) {
  socket->disconnectFromHost();
}

void Server::putRawLine( const QString &s ) {
  qDebug() << "Sent: " << s;
  stream << s << "\r\n" << flush;
  //Message::createFromServerString(this, s);
}

void Server::socketHasData( ) {
  while(socket->canReadLine()) {
    QString s = stream.readLine();
    qDebug() << "Read: " << s;
    emit recvRawServerMsg(s);
    Message *msg = Message::createFromServerString(this, s);
    if(msg) {
      try { handleServerMsg(msg); } catch(Exception e) {
        emit recvLine(e.msg() + "\n");
      }
    }
    delete msg;
  }
}

void Server::socketError( QAbstractSocket::SocketError err ) {
  qDebug() << "Socket Error!";
  //emit error(err);
}

void Server::socketConnected( ) {
  qDebug() << "Socket connected!";
  //emit connected();
}

void Server::socketDisconnected( ) {
  qDebug() << "Socket disconnected!";
  //emit disconnected();
}

void Server::socketStateChanged(QAbstractSocket::SocketState state) {
  qDebug() << "Socket state changed: " << state;
}

/** Handle a message sent by the IRC server that does not have a custom handler. */
void Server::handleServerMsg(Message *msg) {
  int cmdCode = msg->getCmdCode();
  QString prefix = msg->getPrefix();
  QStringList params = msg->getParams();
  if(cmdCode < 0) {
    switch(-cmdCode) {
      case CMD_PING:
        // PING <server1> [<server2>]
        if(params.size() == 1) {
          putRawLine(QString("PONG :") + params[0]);
        } else if(params.size() == 2) {
          putRawLine(QString("PONG ") + params[0] + " :" + params[1]);
        } else throw ParseError(msg);
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

QString Server::handleUserMsg(Message *msg) {

  return "";
}

/* Exception classes for message handling */
Server::ParseError::ParseError(Message *msg) {
  _msg = QString("Command Parse Error: ") + msg->getCmd() + msg->getParams().join(" ");

}

Server::UnknownCmdError::UnknownCmdError(Message *msg) {
  _msg = QString("Unknown Command: ") + msg->getCmd();

}

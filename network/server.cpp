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

#include "server.h"

Server::Server() {
  socket = new QTcpSocket();

}

Server::~Server() {
  delete socket;
}

void Server::init() {
  Message::init(&handleServerMsg, &handleUserMsg);
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
  qDebug() << "Raw line: " << s;
  stream << s << "\r\n" << flush;
}

void Server::socketHasData( ) {
  while(socket->canReadLine()) {
    QString s = stream.readLine();
    qDebug() << "Read: " << s;
    emit recvLine(s + "\n");
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

void Server::handleServerMsg(Message *msg) {


}

void Server::handleUserMsg(Message *msg) {


}


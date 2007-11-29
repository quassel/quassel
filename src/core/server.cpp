/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
#include "server.h"

#include <QMetaObject>
#include <QMetaMethod>
#include <QDateTime>

#include "util.h"
#include "core.h"
#include "coresession.h"

#include "networkinfo.h"

#include "ircserverhandler.h"
#include "userinputhandler.h"
#include "ctcphandler.h"

Server::Server(UserId uid, uint networkId, QString net)
  : _userId(uid),
    _networkId(networkId),
    _ircServerHandler(new IrcServerHandler(this)),
    _userInputHandler(new UserInputHandler(this)),
    _ctcpHandler(new CtcpHandler(this)),
    _networkInfo(new NetworkInfo(networkId, this))
{
  networkInfo()->setNetworkName(net);
  networkInfo()->setProxy(coreSession()->signalProxy());
}

Server::~Server() {
  delete _ircServerHandler;
  delete _userInputHandler;
  delete _ctcpHandler;
}

void Server::run() {
  connect(&socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(quit()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));
  connect(this, SIGNAL(finished()), this, SLOT(threadFinished()));

  exec();
}

void Server::connectToIrc(QString net) {
  if(net != networkName())
    return; // not me!
  
  CoreSession *sess = coreSession();
  networkSettings = sess->retrieveSessionData("Networks").toMap()[net].toMap();
  identity = sess->retrieveSessionData("Identities").toMap()[networkSettings["Identity"].toString()].toMap();

  //FIXME this will result in a pretty fuckup if there are no servers in the list
  QList<QVariant> servers = networkSettings["Servers"].toList();
  QString host = servers[0].toMap()["Address"].toString();
  quint16 port = servers[0].toMap()["Port"].toUInt();
  displayStatusMsg(QString("Connecting to %1:%2...").arg(host).arg(port));
  socket.connectToHost(host, port);
}

void Server::disconnectFromIrc(QString net) {
  if(net != networkName())
    return; // not me!
  socket.disconnectFromHost();
}

void Server::socketHasData() {
  while(socket.canReadLine()) {
    QByteArray s = socket.readLine().trimmed();
    ircServerHandler()->handleServerMsg(s);
  }
}

void Server::socketError( QAbstractSocket::SocketError err ) {
  //qDebug() << "Socket Error!";
}

void Server::socketConnected() {
  emit connected(networkId());
  putRawLine(QString("NICK :%1").arg(identity["NickList"].toStringList()[0]));  // FIXME: try more nicks if error occurs
  putRawLine(QString("USER %1 8 * :%2").arg(identity["Ident"].toString()).arg(identity["RealName"].toString()));
}

void Server::threadFinished() {
  // the Socket::disconnected() is connect to this::quit()
  // so after the event loop is finished we're beeing called
  // and propagate the disconnect
  emit disconnected(networkId());
}

void Server::socketStateChanged(QAbstractSocket::SocketState state) {
  //qDebug() << "Socket state changed: " << state;
}

void Server::userInput(uint netid, QString buf, QString msg) {
  if(netid != networkId())
    return; // not me!
  userInputHandler()->handleUserInput(buf, msg);
}

void Server::putRawLine(QString s) {
  s += "\r\n";
  socket.write(s.toAscii());
}

void Server::putCmd(QString cmd, QStringList params, QString prefix) {
  QString msg;
  if(!prefix.isEmpty())
    msg += ":" + prefix + " ";
  msg += cmd.toUpper();
  
  for(int i = 0; i < params.size() - 1; i++) {
    msg += " " + params[i];
  }
  if(!params.isEmpty())
    msg += " :" + params.last();

  putRawLine(msg);
}


uint Server::networkId() const {
  return _networkId;
}

QString Server::networkName() {
  return networkInfo()->networkName();
}

CoreSession *Server::coreSession() const {
  return Core::session(userId());
}

/* Exception classes for message handling */
Server::ParseError::ParseError(QString cmd, QString prefix, QStringList params) {
  _msg = QString("Command Parse Error: ") + cmd + params.join(" ");
}

Server::UnknownCmdError::UnknownCmdError(QString cmd, QString prefix, QStringList params) {
  _msg = QString("Unknown Command: ") + cmd;
}

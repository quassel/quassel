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
#include "networkconnection.h"

#include <QMetaObject>
#include <QMetaMethod>
#include <QDateTime>

#include "util.h"
#include "core.h"
#include "coresession.h"

#include "ircuser.h"
#include "network.h"

#include "ircserverhandler.h"
#include "userinputhandler.h"
#include "ctcphandler.h"

NetworkConnection::NetworkConnection(UserId uid, NetworkId networkId, QString net, const QVariant &state)
  : _userId(uid),
    _networkId(networkId),
    _ircServerHandler(new IrcServerHandler(this)),
    _userInputHandler(new UserInputHandler(this)),
    _ctcpHandler(new CtcpHandler(this)),
    _network(new Network(networkId, this)),
    _previousState(state)
{
  connect(network(), SIGNAL(currentServerSet(const QString &)), this, SLOT(sendPerform()));
  network()->setCodecForEncoding("ISO-8859-15"); // FIXME
  network()->setCodecForDecoding("ISO-8859-15"); // FIXME
  network()->setNetworkName(net);
  network()->setProxy(coreSession()->signalProxy());
}

NetworkConnection::~NetworkConnection() {
  delete _ircServerHandler;
  delete _userInputHandler;
  delete _ctcpHandler;
}

void NetworkConnection::run() {
  connect(&socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(quit()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));
  connect(this, SIGNAL(finished()), this, SLOT(threadFinished()));

  exec();
}

QString NetworkConnection::serverDecode(const QByteArray &string) const {
  return network()->decodeString(string);
}

QString NetworkConnection::bufferDecode(const QString &bufferName, const QByteArray &string) const {
  Q_UNUSED(bufferName);
  // TODO: Implement buffer-specific encodings
  return network()->decodeString(string);
}

QString NetworkConnection::userDecode(const QString &userNick, const QByteArray &string) const {
  IrcUser *user = network()->ircUser(userNick);
  if(user) return user->decodeString(string);
  return network()->decodeString(string);
}

QByteArray NetworkConnection::serverEncode(const QString &string) const {
  return network()->encodeString(string);
}

QByteArray NetworkConnection::bufferEncode(const QString &bufferName, const QString &string) const {
  Q_UNUSED(bufferName);
  // TODO: Implement buffer-specific encodings
  return network()->encodeString(string);
}

QByteArray NetworkConnection::userEncode(const QString &userNick, const QString &string) const {
  IrcUser *user = network()->ircUser(userNick);
  if(user) return user->encodeString(string);
  return network()->encodeString(string);
}


void NetworkConnection::connectToIrc(QString net) {
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

void NetworkConnection::sendPerform() {
  // TODO: reimplement perform List!
  //// send performlist
  //QStringList performList = networkSettings["Perform"].toString().split( "\n" );
  //int count = performList.count();
  //for(int a = 0; a < count; a++) {
  //  if(!performList[a].isEmpty() ) {
  //    userInput(network, "", performList[a]);
  //  }
  //}

  // rejoin channels we've been in
  QStringList chans = _previousState.toStringList();
  if(chans.count() > 0) {
    qDebug() << "autojoining" << chans;
    QString list = chans.join(",");
    putCmd("join", QStringList(list));
  }
  // delete _previousState, we won't need it again
  _previousState = QVariant();
}

QVariant NetworkConnection::state() {
  IrcUser *me = network()->ircUser(network()->myNick());
  if(!me) return QVariant();  // this shouldn't really happen, I guess
  return me->channels();
}

void NetworkConnection::disconnectFromIrc(QString net) {
  if(net != networkName())
    return; // not me!
  socket.disconnectFromHost();
}

void NetworkConnection::socketHasData() {
  while(socket.canReadLine()) {
    QByteArray s = socket.readLine().trimmed();
    ircServerHandler()->handleServerMsg(s);
  }
}

void NetworkConnection::socketError( QAbstractSocket::SocketError err ) {
  //qDebug() << "Socket Error!";
}

void NetworkConnection::socketConnected() {
  emit connected(networkId());
  putRawLine(QString("NICK :%1").arg(identity["NickList"].toStringList()[0]));  // FIXME: try more nicks if error occurs
  putRawLine(QString("USER %1 8 * :%2").arg(identity["Ident"].toString()).arg(identity["RealName"].toString()));
}

void NetworkConnection::threadFinished() {
  // the Socket::disconnected() is connect to this::quit()
  // so after the event loop is finished we're beeing called
  // and propagate the disconnect
  emit disconnected(networkId());
}

void NetworkConnection::socketStateChanged(QAbstractSocket::SocketState state) {
  //qDebug() << "Socket state changed: " << state;
}

void NetworkConnection::userInput(uint netid, QString buf, QString msg) {
  if(netid != networkId())
    return; // not me!
  userInputHandler()->handleUserInput(buf, msg);
}

void NetworkConnection::putRawLine(QString s) {
  s += "\r\n";
  socket.write(s.toAscii());
}

void NetworkConnection::putCmd(QString cmd, QStringList params, QString prefix) {
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


uint NetworkConnection::networkId() const {
  return _networkId;
}

QString NetworkConnection::networkName() const {
  return network()->networkName();
}

CoreSession *NetworkConnection::coreSession() const {
  return Core::session(userId());
}

/* Exception classes for message handling */
NetworkConnection::ParseError::ParseError(QString cmd, QString prefix, QStringList params) {
  Q_UNUSED(prefix);
  _msg = QString("Command Parse Error: ") + cmd + params.join(" ");
}

NetworkConnection::UnknownCmdError::UnknownCmdError(QString cmd, QString prefix, QStringList params) {
  Q_UNUSED(prefix);
  _msg = QString("Unknown Command: ") + cmd + params.join(" ");
}

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

#include "ircchannel.h"
#include "ircuser.h"
#include "network.h"
#include "identity.h"

#include "ircserverhandler.h"
#include "userinputhandler.h"
#include "ctcphandler.h"

NetworkConnection::NetworkConnection(Network *network, CoreSession *session, const QVariant &state) : QObject(network),
    _connectionState(Network::Disconnected),
    _network(network),
    _coreSession(session),
    _ircServerHandler(new IrcServerHandler(this)),
    _userInputHandler(new UserInputHandler(this)),
    _ctcpHandler(new CtcpHandler(this)),
    _previousState(state)
{
  connect(network, SIGNAL(currentServerSet(const QString &)), this, SLOT(networkInitialized(const QString &)));

  connect(&socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));

}

NetworkConnection::~NetworkConnection() {
  disconnectFromIrc();
  delete _ircServerHandler;
  delete _userInputHandler;
  delete _ctcpHandler;
}

bool NetworkConnection::isConnected() const {
  // return socket.state() == QAbstractSocket::ConnectedState;
  return connectionState() == Network::Initialized;
}

Network::ConnectionState NetworkConnection::connectionState() const {
  return _connectionState;
}

void NetworkConnection::setConnectionState(Network::ConnectionState state) {
  _connectionState = state;
  network()->setConnectionState(state);
  emit connectionStateChanged(state);
}

NetworkId NetworkConnection::networkId() const {
  return network()->networkId();
}

QString NetworkConnection::networkName() const {
  return network()->networkName();
}

Identity *NetworkConnection::identity() const {
  return coreSession()->identity(network()->identity());
}

Network *NetworkConnection::network() const {
  return _network;
}

CoreSession *NetworkConnection::coreSession() const {
  return _coreSession;
}

IrcServerHandler *NetworkConnection::ircServerHandler() const {
  return _ircServerHandler;
}

UserInputHandler *NetworkConnection::userInputHandler() const {
  return _userInputHandler;
}

CtcpHandler *NetworkConnection::ctcpHandler() const {
  return _ctcpHandler;
}

QString NetworkConnection::serverDecode(const QByteArray &string) const {
  return network()->decodeServerString(string);
}

QString NetworkConnection::channelDecode(const QString &bufferName, const QByteArray &string) const {
  if(!bufferName.isEmpty()) {
    IrcChannel *channel = network()->ircChannel(bufferName);
    if(channel) return channel->decodeString(string);
  }
  return network()->decodeString(string);
}

QString NetworkConnection::userDecode(const QString &userNick, const QByteArray &string) const {
  IrcUser *user = network()->ircUser(userNick);
  if(user) return user->decodeString(string);
  return network()->decodeString(string);
}

QByteArray NetworkConnection::serverEncode(const QString &string) const {
  return network()->encodeServerString(string);
}

QByteArray NetworkConnection::channelEncode(const QString &bufferName, const QString &string) const {
  if(!bufferName.isEmpty()) {
    IrcChannel *channel = network()->ircChannel(bufferName);
    if(channel) return channel->encodeString(string);
  }
  return network()->encodeString(string);
}

QByteArray NetworkConnection::userEncode(const QString &userNick, const QString &string) const {
  IrcUser *user = network()->ircUser(userNick);
  if(user) return user->encodeString(string);
  return network()->encodeString(string);
}


void NetworkConnection::connectToIrc() {
  QVariantList serverList = network()->serverList();
  Identity *identity = coreSession()->identity(network()->identity());
  if(!serverList.count()) {
    qWarning() << "Server list empty, ignoring connect request!";
    return;
  }
  if(!identity) {
    qWarning() << "Invalid identity configures, ignoring connect request!";
    return;
  }
  // TODO implement cycling / random servers
  QString host = serverList[0].toMap()["Host"].toString();
  quint16 port = serverList[0].toMap()["Port"].toUInt();
  displayStatusMsg(QString("Connecting to %1:%2...").arg(host).arg(port));
  socket.connectToHost(host, port);
}

void NetworkConnection::networkInitialized(const QString &currentServer) {
  if(currentServer.isEmpty()) return;

  sendPerform();

    // rejoin channels we've been in
  QStringList chans = _previousState.toStringList();
  if(chans.count() > 0) {
    qDebug() << "autojoining" << chans;
    QVariantList list;
    foreach(QString chan, chans) list << serverEncode(chan);
    putCmd("JOIN", list);  // FIXME check for 512 byte limit!
  }
  // delete _previousState, we won't need it again
  _previousState = QVariant();
  // now we are initialized
  setConnectionState(Network::Initialized);
  network()->setConnected(true);
  emit connected(networkId());
}

void NetworkConnection::sendPerform() {
  BufferInfo statusBuf = Core::bufferInfo(coreSession()->user(), network()->networkId(), BufferInfo::StatusBuffer);
  // do auto identify
  if(network()->useAutoIdentify() && !network()->autoIdentifyService().isEmpty() && !network()->autoIdentifyPassword().isEmpty()) {
    userInputHandler()->handleMsg(statusBuf, QString("%1 IDENTIFY %2").arg(network()->autoIdentifyService(), network()->autoIdentifyPassword()));
  }
  // send perform list
  foreach(QString line, network()->perform()) {
    if(!line.isEmpty()) userInput(statusBuf, line);
  }
}

QVariant NetworkConnection::state() const {
  IrcUser *me = network()->ircUser(network()->myNick());
  if(!me) return QVariant();  // this shouldn't really happen, I guess
  return me->channels();
}

void NetworkConnection::disconnectFromIrc() {
  if(socket.state() < QAbstractSocket::ConnectedState) {
    setConnectionState(Network::Disconnected);
    socketDisconnected();
  } else socket.disconnectFromHost();
}

void NetworkConnection::socketHasData() {
  while(socket.canReadLine()) {
    QByteArray s = socket.readLine().trimmed();
    ircServerHandler()->handleServerMsg(s);
  }
}

void NetworkConnection::socketError(QAbstractSocket::SocketError) {
  qDebug() << qPrintable(tr("Could not connect to %1 (%2)").arg(network()->networkName(), socket.errorString()));
  emit connectionError(socket.errorString());
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Connection failure: %1").arg(socket.errorString()));
  network()->emitConnectionError(socket.errorString());
}

void NetworkConnection::socketConnected() {
  //emit connected(networkId());  initialize first!
  Identity *identity = coreSession()->identity(network()->identity());
  if(!identity) {
    qWarning() << "Identity invalid!";
    disconnectFromIrc();
    return;
  }
  putRawLine(serverEncode(QString("NICK :%1").arg(identity->nicks()[0])));  // FIXME: try more nicks if error occurs
  putRawLine(serverEncode(QString("USER %1 8 * :%2").arg(identity->ident(), identity->realName())));
}

void NetworkConnection::socketStateChanged(QAbstractSocket::SocketState socketState) {
  Network::ConnectionState state;
  switch(socketState) {
    case QAbstractSocket::UnconnectedState:
      state = Network::Disconnected;
      break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
      state = Network::Connecting;
      break;
    case QAbstractSocket::ConnectedState:
      state = Network::Initializing;
      break;
    case QAbstractSocket::ClosingState:
      state = Network::Disconnecting;
      break;
    default:
      state = Network::Disconnected;
  }
  setConnectionState(state);
}

void NetworkConnection::socketDisconnected() {
  network()->setConnected(false);
  emit disconnected(networkId());
}

// FIXME switch to BufferId
void NetworkConnection::userInput(BufferInfo buf, QString msg) {
  userInputHandler()->handleUserInput(buf, msg);
}

void NetworkConnection::putRawLine(QByteArray s) {
  s += "\r\n";
  socket.write(s);
}

void NetworkConnection::putCmd(const QString &cmd, const QVariantList &params, const QByteArray &prefix) {
  QByteArray msg;
  if(!prefix.isEmpty())
    msg += ":" + prefix + " ";
  msg += cmd.toUpper().toAscii();

  for(int i = 0; i < params.size() - 1; i++) {
    msg += " " + params[i].toByteArray();
  }
  if(!params.isEmpty())
    msg += " :" + params.last().toByteArray();

  putRawLine(msg);
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

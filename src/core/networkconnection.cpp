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

NetworkConnection::NetworkConnection(Network *network, CoreSession *session) : QObject(network),
    _connectionState(Network::Disconnected),
    _network(network),
    _coreSession(session),
    _ircServerHandler(new IrcServerHandler(this)),
    _userInputHandler(new UserInputHandler(this)),
    _ctcpHandler(new CtcpHandler(this)),
    _autoReconnectCount(0)
{
  _autoReconnectTimer.setSingleShot(true);

  // TODO make configurable
  _whoTimer.setInterval(60 * 1000);
  _whoTimer.setSingleShot(false);

  QHash<QString, QString> channels = coreSession()->persistentChannels(networkId());
  foreach(QString chan, channels.keys()) {
    _channelKeys[chan.toLower()] = channels[chan];
  }

  connect(&_autoReconnectTimer, SIGNAL(timeout()), this, SLOT(doAutoReconnect()));
  connect(&_whoTimer, SIGNAL(timeout()), this, SLOT(sendWho()));

  connect(network, SIGNAL(currentServerSet(const QString &)), this, SLOT(networkInitialized(const QString &)));
  connect(network, SIGNAL(useAutoReconnectSet(bool)), this, SLOT(autoReconnectSettingsChanged()));
  connect(network, SIGNAL(autoReconnectIntervalSet(quint32)), this, SLOT(autoReconnectSettingsChanged()));
  connect(network, SIGNAL(autoReconnectRetriesSet(quint16)), this, SLOT(autoReconnectSettingsChanged()));

  connect(&socket, SIGNAL(connected()), this, SLOT(socketConnected()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));

  connect(_ircServerHandler, SIGNAL(nickChanged(const QString &, const QString &)),
	  this, SLOT(nickChanged(const QString &, const QString &)));
}

NetworkConnection::~NetworkConnection() {
  if(connectionState() != Network::Disconnected && connectionState() != Network::Reconnecting)
    disconnectFromIrc(false); // clean up, but this does not count as requested disconnect!
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

void NetworkConnection::autoReconnectSettingsChanged() {
  if(!network()->useAutoReconnect()) {
    _autoReconnectTimer.stop();
    _autoReconnectCount = 0;
  } else {
    _autoReconnectTimer.setInterval(network()->autoReconnectInterval() * 1000);
    if(_autoReconnectCount != 0) {
      if(network()->unlimitedReconnectRetries()) _autoReconnectCount = -1;
      else _autoReconnectCount = network()->autoReconnectRetries();
    }
  }
}

void NetworkConnection::connectToIrc(bool reconnecting) {
  if(!reconnecting && network()->useAutoReconnect() && _autoReconnectCount == 0) {
    _autoReconnectTimer.setInterval(network()->autoReconnectInterval() * 1000);
    if(network()->unlimitedReconnectRetries()) _autoReconnectCount = -1;
    else _autoReconnectCount = network()->autoReconnectRetries();
  }
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
  displayStatusMsg(tr("Connecting to %1:%2...").arg(host).arg(port));
  displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Connecting to %1:%2...").arg(host).arg(port));
  socket.connectToHost(host, port);
}

void NetworkConnection::networkInitialized(const QString &currentServer) {
  if(currentServer.isEmpty()) return;

  if(network()->useAutoReconnect() && !network()->unlimitedReconnectRetries()) {
    _autoReconnectCount = network()->autoReconnectRetries(); // reset counter
  }

  sendPerform();

  // now we are initialized
  setConnectionState(Network::Initialized);
  network()->setConnected(true);
  emit connected(networkId());
  sendWho();
  _whoTimer.start();
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

  // rejoin channels we've been in
  QStringList channels, keys;
  foreach(QString chan, persistentChannels()) {
    QString key = channelKey(chan);
    if(!key.isEmpty()) {
      channels.prepend(chan); keys.prepend(key);
    } else {
      channels.append(chan);
    }
  }
  QString joinString = QString("%1 %2").arg(channels.join(",")).arg(keys.join(",")).trimmed();
  if(!joinString.isEmpty()) userInputHandler()->handleJoin(statusBuf, joinString);
}

void NetworkConnection::disconnectFromIrc(bool requested) {
  if(requested) {
    _autoReconnectTimer.stop();
    _autoReconnectCount = 0;
  }
  displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Disconnecting."));
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
  if(socket.state() < QAbstractSocket::ConnectedState) {
    setConnectionState(Network::Disconnected);
    socketDisconnected();
  }
  //qDebug() << "exiting...";
  //exit(1);
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
  _whoTimer.stop();
  network()->setConnected(false);
  emit disconnected(networkId());
  if(_autoReconnectCount == 0) emit quitRequested(networkId());
  else {
    setConnectionState(Network::Reconnecting);
    if(_autoReconnectCount == network()->autoReconnectRetries()) doAutoReconnect(); // first try is immediate
    else _autoReconnectTimer.start();
  }
}

void NetworkConnection::doAutoReconnect() {
  if(connectionState() != Network::Disconnected && connectionState() != Network::Reconnecting) {
    qWarning() << "NetworkConnection::doAutoReconnect(): Cannot reconnect while not being disconnected!";
    return;
  }
  if(_autoReconnectCount > 0) _autoReconnectCount--;
  connectToIrc(true);
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

void NetworkConnection::sendWho() {
  foreach(QString chan, network()->channels()) {
    putRawLine("WHO " + serverEncode(chan));
  }
}

void NetworkConnection::setChannelJoined(const QString &channel) {
  emit channelJoined(networkId(), channel, _channelKeys[channel.toLower()]);
}

void NetworkConnection::setChannelParted(const QString &channel) {
  removeChannelKey(channel);
  emit channelParted(networkId(), channel);
}

void NetworkConnection::addChannelKey(const QString &channel, const QString &key) {
  if(key.isEmpty()) {
    removeChannelKey(channel);
  } else {
    _channelKeys[channel.toLower()] = key;
  }
}

void NetworkConnection::removeChannelKey(const QString &channel) {
  _channelKeys.remove(channel.toLower());
}

void NetworkConnection::nickChanged(const QString &newNick, const QString &oldNick) {
  emit nickChanged(networkId(), newNick, oldNick);
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


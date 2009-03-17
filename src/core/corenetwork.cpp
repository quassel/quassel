/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "corenetwork.h"

#include "core.h"
#include "coresession.h"
#include "coreidentity.h"

#include "ircserverhandler.h"
#include "userinputhandler.h"
#include "ctcphandler.h"

CoreNetwork::CoreNetwork(const NetworkId &networkid, CoreSession *session)
  : Network(networkid, session),
    _coreSession(session),
    _ircServerHandler(new IrcServerHandler(this)),
    _userInputHandler(new UserInputHandler(this)),
    _ctcpHandler(new CtcpHandler(this)),
    _autoReconnectCount(0),
    _quitRequested(false),

    _previousConnectionAttemptFailed(false),
    _lastUsedServerIndex(0),

    _lastPingTime(0),
    _maxPingCount(3),
    _pingCount(0),

    // TODO make autowho configurable (possibly per-network)
    _autoWhoEnabled(true),
    _autoWhoInterval(90),
    _autoWhoNickLimit(0), // unlimited
    _autoWhoDelay(5)
{
  _autoReconnectTimer.setSingleShot(true);
  _socketCloseTimer.setSingleShot(true);
  connect(&_socketCloseTimer, SIGNAL(timeout()), this, SLOT(socketCloseTimeout()));

  _pingTimer.setInterval(30000);
  connect(&_pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));

  _autoWhoTimer.setInterval(_autoWhoDelay * 1000);
  _autoWhoCycleTimer.setInterval(_autoWhoInterval * 1000);

  QHash<QString, QString> channels = coreSession()->persistentChannels(networkId());
  foreach(QString chan, channels.keys()) {
    _channelKeys[chan.toLower()] = channels[chan];
  }

  connect(&_autoReconnectTimer, SIGNAL(timeout()), this, SLOT(doAutoReconnect()));
  connect(&_autoWhoTimer, SIGNAL(timeout()), this, SLOT(sendAutoWho()));
  connect(&_autoWhoCycleTimer, SIGNAL(timeout()), this, SLOT(startAutoWhoCycle()));
  connect(&_tokenBucketTimer, SIGNAL(timeout()), this, SLOT(fillBucketAndProcessQueue()));
  connect(this, SIGNAL(connectRequested()), this, SLOT(connectToIrc()));


  connect(&socket, SIGNAL(connected()), this, SLOT(socketInitialized()));
  connect(&socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
  connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  connect(&socket, SIGNAL(readyRead()), this, SLOT(socketHasData()));
#ifdef HAVE_SSL
  connect(&socket, SIGNAL(encrypted()), this, SLOT(socketInitialized()));
  connect(&socket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));
#endif
}

CoreNetwork::~CoreNetwork() {
  if(connectionState() != Disconnected && connectionState() != Network::Reconnecting)
    disconnectFromIrc(false);      // clean up, but this does not count as requested disconnect!
  disconnect(&socket, 0, this, 0); // this keeps the socket from triggering events during clean up
  delete _ircServerHandler;
  delete _userInputHandler;
  delete _ctcpHandler;
}

QString CoreNetwork::channelDecode(const QString &bufferName, const QByteArray &string) const {
  if(!bufferName.isEmpty()) {
    IrcChannel *channel = ircChannel(bufferName);
    if(channel)
      return channel->decodeString(string);
  }
  return decodeString(string);
}

QString CoreNetwork::userDecode(const QString &userNick, const QByteArray &string) const {
  IrcUser *user = ircUser(userNick);
  if(user)
    return user->decodeString(string);
  return decodeString(string);
}

QByteArray CoreNetwork::channelEncode(const QString &bufferName, const QString &string) const {
  if(!bufferName.isEmpty()) {
    IrcChannel *channel = ircChannel(bufferName);
    if(channel)
      return channel->encodeString(string);
  }
  return encodeString(string);
}

QByteArray CoreNetwork::userEncode(const QString &userNick, const QString &string) const {
  IrcUser *user = ircUser(userNick);
  if(user)
    return user->encodeString(string);
  return encodeString(string);
}

void CoreNetwork::connectToIrc(bool reconnecting) {
  if(!reconnecting && useAutoReconnect() && _autoReconnectCount == 0) {
    _autoReconnectTimer.setInterval(autoReconnectInterval() * 1000);
    if(unlimitedReconnectRetries())
      _autoReconnectCount = -1;
    else
      _autoReconnectCount = autoReconnectRetries();
  }
  if(serverList().isEmpty()) {
    qWarning() << "Server list empty, ignoring connect request!";
    return;
  }
  CoreIdentity *identity = identityPtr();
  if(!identity) {
    qWarning() << "Invalid identity configures, ignoring connect request!";
    return;
  }

  // cleaning up old quit reason
  _quitReason.clear();

  // use a random server?
  if(useRandomServer()) {
    _lastUsedServerIndex = qrand() % serverList().size();
  } else if(_previousConnectionAttemptFailed) {
    // cycle to next server if previous connection attempt failed
    displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Connection failed. Cycling to next Server"));
    if(++_lastUsedServerIndex >= serverList().size()) {
      _lastUsedServerIndex = 0;
    }
  }
  _previousConnectionAttemptFailed = false;

  Server server = usedServer();
  displayStatusMsg(tr("Connecting to %1:%2...").arg(server.host).arg(server.port));
  displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Connecting to %1:%2...").arg(server.host).arg(server.port));

  if(server.useProxy) {
    QNetworkProxy proxy((QNetworkProxy::ProxyType)server.proxyType, server.proxyHost, server.proxyPort, server.proxyUser, server.proxyPass);
    socket.setProxy(proxy);
  } else {
    socket.setProxy(QNetworkProxy::NoProxy);
  }

#ifdef HAVE_SSL
  socket.setProtocol((QSsl::SslProtocol)server.sslVersion);
  if(server.useSsl) {
    CoreIdentity *identity = identityPtr();
    if(identity) {
      socket.setLocalCertificate(identity->sslCert());
      socket.setPrivateKey(identity->sslKey());
    }
    socket.connectToHostEncrypted(server.host, server.port);
  } else {
    socket.connectToHost(server.host, server.port);
  }
#else
  socket.connectToHost(server.host, server.port);
#endif
}

void CoreNetwork::disconnectFromIrc(bool requested, const QString &reason, bool withReconnect) {
  _quitRequested = requested; // see socketDisconnected();
  if(!withReconnect) {
    _autoReconnectTimer.stop();
    _autoReconnectCount = 0; // prohibiting auto reconnect
  }
  disablePingTimeout();
  _msgQueue.clear();

  IrcUser *me_ = me();
  if(me_) {
    QString awayMsg;
    if(me_->isAway())
      awayMsg = me_->awayMessage();
    Core::setAwayMessage(userId(), networkId(), awayMsg);
    Core::setUserModes(userId(), networkId(), me_->userModes());
  }

  if(reason.isEmpty() && identityPtr())
    _quitReason = identityPtr()->quitReason();
  else
    _quitReason = reason;

  displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Disconnecting. (%1)").arg((!requested && !withReconnect) ? tr("Core Shutdown") : _quitReason));
  switch(socket.state()) {
  case QAbstractSocket::ConnectedState:
    userInputHandler()->issueQuit(_quitReason);
    if(requested || withReconnect) {
      // the irc server has 10 seconds to close the socket
      _socketCloseTimer.start(10000);
      break;
    }
  default:
    socket.close();
    socketDisconnected();
  }
}

void CoreNetwork::userInput(BufferInfo buf, QString msg) {
  userInputHandler()->handleUserInput(buf, msg);
}

void CoreNetwork::putRawLine(QByteArray s) {
  if(_tokenBucket > 0)
    writeToSocket(s);
  else
    _msgQueue.append(s);
}

void CoreNetwork::putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix) {
  QByteArray msg;

  if(!prefix.isEmpty())
    msg += ":" + prefix + " ";
  msg += cmd.toUpper().toAscii();

  for(int i = 0; i < params.size() - 1; i++) {
    msg += " " + params[i];
  }
  if(!params.isEmpty())
    msg += " :" + params.last();

  putRawLine(msg);
}

void CoreNetwork::setChannelJoined(const QString &channel) {
  _autoWhoQueue.prepend(channel.toLower()); // prepend so this new chan is the first to be checked

  Core::setChannelPersistent(userId(), networkId(), channel, true);
  Core::setPersistentChannelKey(userId(), networkId(), channel, _channelKeys[channel.toLower()]);
}

void CoreNetwork::setChannelParted(const QString &channel) {
  removeChannelKey(channel);
  _autoWhoQueue.removeAll(channel.toLower());
  _autoWhoPending.remove(channel.toLower());

  Core::setChannelPersistent(userId(), networkId(), channel, false);
}

void CoreNetwork::addChannelKey(const QString &channel, const QString &key) {
  if(key.isEmpty()) {
    removeChannelKey(channel);
  } else {
    _channelKeys[channel.toLower()] = key;
  }
}

void CoreNetwork::removeChannelKey(const QString &channel) {
  _channelKeys.remove(channel.toLower());
}

bool CoreNetwork::setAutoWhoDone(const QString &channel) {
  QString chan = channel.toLower();
  if(_autoWhoPending.value(chan, 0) <= 0)
    return false;
  if(--_autoWhoPending[chan] <= 0)
    _autoWhoPending.remove(chan);
  return true;
}

void CoreNetwork::setMyNick(const QString &mynick) {
  Network::setMyNick(mynick);
  if(connectionState() == Network::Initializing)
    networkInitialized();
}

void CoreNetwork::socketHasData() {
  while(socket.canReadLine()) {
    QByteArray s = socket.readLine().trimmed();
    ircServerHandler()->handleServerMsg(s);
  }
}

void CoreNetwork::socketError(QAbstractSocket::SocketError error) {
  if(_quitRequested && error == QAbstractSocket::RemoteHostClosedError)
    return;

  _previousConnectionAttemptFailed = true;
  qWarning() << qPrintable(tr("Could not connect to %1 (%2)").arg(networkName(), socket.errorString()));
  emit connectionError(socket.errorString());
  displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Connection failure: %1").arg(socket.errorString()));
  emitConnectionError(socket.errorString());
  if(socket.state() < QAbstractSocket::ConnectedState) {
    socketDisconnected();
  }
}

void CoreNetwork::socketInitialized() {
  Server server = usedServer();
#ifdef HAVE_SSL
  if(server.useSsl && !socket.isEncrypted())
    return;
#endif

  CoreIdentity *identity = identityPtr();
  if(!identity) {
    qCritical() << "Identity invalid!";
    disconnectFromIrc();
    return;
  }

  // TokenBucket to avoid sending too much at once
  _messageDelay = 2200;    // this seems to be a safe value (2.2 seconds delay)
  _burstSize = 5;
  _tokenBucket = _burstSize; // init with a full bucket
  _tokenBucketTimer.start(_messageDelay);

  if(!server.password.isEmpty()) {
    putRawLine(serverEncode(QString("PASS %1").arg(server.password)));
  }
  QString nick;
  if(identity->nicks().isEmpty()) {
    nick = "quassel";
    qWarning() << "CoreNetwork::socketInitialized(): no nicks supplied for identity Id" << identity->id();
  } else {
    nick = identity->nicks()[0];
  }
  putRawLine(serverEncode(QString("NICK :%1").arg(nick)));
  putRawLine(serverEncode(QString("USER %1 8 * :%2").arg(identity->ident(), identity->realName())));
}

void CoreNetwork::socketDisconnected() {
  disablePingTimeout();
  _msgQueue.clear();

  _autoWhoCycleTimer.stop();
  _autoWhoTimer.stop();
  _autoWhoQueue.clear();
  _autoWhoPending.clear();

  _socketCloseTimer.stop();

  _tokenBucketTimer.stop();

  IrcUser *me_ = me();
  if(me_) {
    foreach(QString channel, me_->channels())
      displayMsg(Message::Quit, BufferInfo::ChannelBuffer, channel, _quitReason, me_->hostmask());
  }

  setConnected(false);
  emit disconnected(networkId());
  if(_quitRequested) {
    setConnectionState(Network::Disconnected);
    Core::setNetworkConnected(userId(), networkId(), false);
  } else if(_autoReconnectCount != 0) {
    setConnectionState(Network::Reconnecting);
    if(_autoReconnectCount == autoReconnectRetries())
      doAutoReconnect(); // first try is immediate
    else
      _autoReconnectTimer.start();
  }
}

void CoreNetwork::socketStateChanged(QAbstractSocket::SocketState socketState) {
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

void CoreNetwork::networkInitialized() {
  setConnectionState(Network::Initialized);
  setConnected(true);
  _quitRequested = false;

  if(useAutoReconnect()) {
    // reset counter
    _autoReconnectCount = autoReconnectRetries();
  }

  // restore away state
  QString awayMsg = Core::awayMessage(userId(), networkId());
  if(!awayMsg.isEmpty())
    userInputHandler()->handleAway(BufferInfo(), Core::awayMessage(userId(), networkId()));

  // restore old user modes if server default mode is set.
  IrcUser *me_ = me();
  if(me_) {
    if(!me_->userModes().isEmpty()) {
      restoreUserModes();
    } else {
      connect(me_, SIGNAL(userModesSet(QString)), this, SLOT(restoreUserModes()));
      connect(me_, SIGNAL(userModesAdded(QString)), this, SLOT(restoreUserModes()));
    }
  }

  sendPerform();

  enablePingTimeout();

  if(_autoWhoEnabled) {
    _autoWhoCycleTimer.start();
    _autoWhoTimer.start();
    startAutoWhoCycle();  // FIXME wait for autojoin to be completed
  }

  Core::bufferInfo(userId(), networkId(), BufferInfo::StatusBuffer); // create status buffer
  Core::setNetworkConnected(userId(), networkId(), true);
}

void CoreNetwork::sendPerform() {
  BufferInfo statusBuf = BufferInfo::fakeStatusBuffer(networkId());

  // do auto identify
  if(useAutoIdentify() && !autoIdentifyService().isEmpty() && !autoIdentifyPassword().isEmpty()) {
    userInputHandler()->handleMsg(statusBuf, QString("%1 IDENTIFY %2").arg(autoIdentifyService(), autoIdentifyPassword()));
  }

  // send perform list
  foreach(QString line, perform()) {
    if(!line.isEmpty()) userInput(statusBuf, line);
  }

  // rejoin channels we've been in
  if(rejoinChannels()) {
    QStringList channels, keys;
    foreach(QString chan, coreSession()->persistentChannels(networkId()).keys()) {
      QString key = channelKey(chan);
      if(!key.isEmpty()) {
        channels.prepend(chan);
        keys.prepend(key);
      } else {
        channels.append(chan);
      }
    }
    QString joinString = QString("%1 %2").arg(channels.join(",")).arg(keys.join(",")).trimmed();
    if(!joinString.isEmpty())
      userInputHandler()->handleJoin(statusBuf, joinString);
  }
}

void CoreNetwork::restoreUserModes() {
  IrcUser *me_ = me();
  Q_ASSERT(me_);

  disconnect(me_, SIGNAL(userModesSet(QString)), this, SLOT(restoreUserModes()));
  disconnect(me_, SIGNAL(userModesAdded(QString)), this, SLOT(restoreUserModes()));

  QString removeModes;
  QString addModes = Core::userModes(userId(), networkId());
  QString currentModes = me_->userModes();

  removeModes = currentModes;
  removeModes.remove(QRegExp(QString("[%1]").arg(addModes)));
  addModes.remove(QRegExp(QString("[%1]").arg(currentModes)));

  removeModes = QString("%1 -%2").arg(me_->nick(), removeModes);
  addModes = QString("%1 +%2").arg(me_->nick(), addModes);
  userInputHandler()->handleMode(BufferInfo(), removeModes);
  userInputHandler()->handleMode(BufferInfo(), addModes);
}

void CoreNetwork::setUseAutoReconnect(bool use) {
  Network::setUseAutoReconnect(use);
  if(!use)
    _autoReconnectTimer.stop();
}

void CoreNetwork::setAutoReconnectInterval(quint32 interval) {
  Network::setAutoReconnectInterval(interval);
  _autoReconnectTimer.setInterval(interval * 1000);
}

void CoreNetwork::setAutoReconnectRetries(quint16 retries) {
  Network::setAutoReconnectRetries(retries);
  if(_autoReconnectCount != 0) {
    if(unlimitedReconnectRetries())
      _autoReconnectCount = -1;
    else
      _autoReconnectCount = autoReconnectRetries();
  }
}

void CoreNetwork::doAutoReconnect() {
  if(connectionState() != Network::Disconnected && connectionState() != Network::Reconnecting) {
    qWarning() << "CoreNetwork::doAutoReconnect(): Cannot reconnect while not being disconnected!";
    return;
  }
  if(_autoReconnectCount > 0)
    _autoReconnectCount--;
  connectToIrc(true);
}

void CoreNetwork::sendPing() {
  uint now = QDateTime::currentDateTime().toTime_t();
  if(_pingCount != 0) {
    qDebug() << "UserId:" << userId() << "Network:" << networkName() << "missed" << _pingCount << "pings."
	     << "BA:" << socket.bytesAvailable() << "BTW:" << socket.bytesToWrite();
  }
  if(_pingCount >= _maxPingCount && now - _lastPingTime <= (uint)(_pingTimer.interval() / 1000) + 1) {
    // the second check compares the actual elapsed time since the last ping and the pingTimer interval
    // if the interval is shorter then the actual elapsed time it means that this thread was somehow blocked
    // and unable to even handle a ping answer. So we ignore those misses.
    disconnectFromIrc(false, QString("No Ping reply in %1 seconds.").arg(_maxPingCount * _pingTimer.interval() / 1000), true /* withReconnect */);
  } else {
    _lastPingTime = now;
    _pingCount++;
    userInputHandler()->handlePing(BufferInfo(), QString());
  }
}

void CoreNetwork::enablePingTimeout() {
  resetPingTimeout();
  _pingTimer.start();
}

void CoreNetwork::disablePingTimeout() {
  _pingTimer.stop();
  resetPingTimeout();
}

void CoreNetwork::sendAutoWho() {
  // Don't send autowho if there are still some pending
  if(_autoWhoPending.count())
    return;

  while(!_autoWhoQueue.isEmpty()) {
    QString chan = _autoWhoQueue.takeFirst();
    IrcChannel *ircchan = ircChannel(chan);
    if(!ircchan) continue;
    if(_autoWhoNickLimit > 0 && ircchan->ircUsers().count() > _autoWhoNickLimit) continue;
    _autoWhoPending[chan]++;
    putRawLine("WHO " + serverEncode(chan));
    if(_autoWhoQueue.isEmpty() && _autoWhoEnabled && !_autoWhoCycleTimer.isActive()) {
      // Timer was stopped, means a new cycle is due immediately
      _autoWhoCycleTimer.start();
      startAutoWhoCycle();
    }
    break;
  }
}

void CoreNetwork::startAutoWhoCycle() {
  if(!_autoWhoQueue.isEmpty()) {
    _autoWhoCycleTimer.stop();
    return;
  }
  _autoWhoQueue = channels();
}

#ifdef HAVE_SSL
void CoreNetwork::sslErrors(const QList<QSslError> &sslErrors) {
  Q_UNUSED(sslErrors)
  socket.ignoreSslErrors();
  // TODO errorhandling
}
#endif  // HAVE_SSL

void CoreNetwork::fillBucketAndProcessQueue() {
  if(_tokenBucket < _burstSize) {
    _tokenBucket++;
  }

  while(_msgQueue.size() > 0 && _tokenBucket > 0) {
    writeToSocket(_msgQueue.takeFirst());
  }
}

void CoreNetwork::writeToSocket(const QByteArray &data) {
  socket.write(data);
  socket.write("\r\n");
  _tokenBucket--;
}

Network::Server CoreNetwork::usedServer() const {
  if(_lastUsedServerIndex < serverList().count())
    return serverList()[_lastUsedServerIndex];

  if(!serverList().isEmpty())
    return serverList()[0];

  return Network::Server();
}

void CoreNetwork::requestConnect() const {
  if(connectionState() != Disconnected) {
    qWarning() << "Requesting connect while already being connected!";
    return;
  }
  Network::requestConnect();
}

void CoreNetwork::requestDisconnect() const {
  if(connectionState() == Disconnected) {
    qWarning() << "Requesting disconnect while not being connected!";
    return;
  }
  userInputHandler()->handleQuit(BufferInfo(), QString());
}

void CoreNetwork::requestSetNetworkInfo(const NetworkInfo &info) {
  Network::Server currentServer = usedServer();
  setNetworkInfo(info);
  Core::updateNetwork(coreSession()->user(), info);

  // the order of the servers might have changed,
  // so we try to find the previously used server
  _lastUsedServerIndex = 0;
  for(int i = 0; i < serverList().count(); i++) {
    Network::Server server = serverList()[i];
    if(server.host == currentServer.host && server.port == currentServer.port) {
      _lastUsedServerIndex = i;
      break;
    }
  }
}

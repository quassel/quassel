/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "corenetwork.h"

#include <algorithm>

#include <QDebug>
#include <QHostInfo>
#include <QRandomGenerator>  // Added for QRandomGenerator
#include <QTextBoundaryFinder>

#include "core.h"
#include "coreidentity.h"
#include "corenetworkconfig.h"
#include "coresession.h"
#include "coreuserinputhandler.h"
#include "irccap.h"
#include "ircencoder.h"
#include "irctag.h"
#include "networkevent.h"

CoreNetwork::CoreNetwork(const NetworkId& networkid, CoreSession* session)
    : Network(networkid, session)
    , _coreSession(session)
    , _userInputHandler(new CoreUserInputHandler(this))
    , _metricsServer(Core::instance()->metricsServer())
    , _autoReconnectCount(0)
    , _quitRequested(false)
    , _disconnectExpected(false)
    , _previousConnectionAttemptFailed(false)
    , _lastUsedServerIndex(0)
    , _requestedUserModes('-')
{
    _debugLogRawIrc = (Quassel::isOptionSet("debug-irc") || Quassel::isOptionSet("debug-irc-id"));
    _debugLogRawNetId = Quassel::optionValue("debug-irc-id").toInt();

    _autoReconnectTimer.setSingleShot(true);
    connect(&_socketCloseTimer, &QTimer::timeout, this, &CoreNetwork::onSocketCloseTimeout);

    setPingInterval(networkConfig()->pingInterval());
    connect(&_pingTimer, &QTimer::timeout, this, &CoreNetwork::sendPing);

    setAutoWhoDelay(networkConfig()->autoWhoDelay());
    setAutoWhoInterval(networkConfig()->autoWhoInterval());

    QHash<QString, QString> channels = coreSession()->persistentChannels(networkId());
    for (const QString& chan : channels.keys()) {
        _channelKeys[chan.toLower()] = channels[chan];
    }

    QHash<QString, QByteArray> bufferCiphers = coreSession()->bufferCiphers(networkId());
    for (const QString& buffer : bufferCiphers.keys()) {
        storeChannelCipherKey(buffer.toLower(), bufferCiphers[buffer]);
    }

    connect(networkConfig(), &NetworkConfig::pingTimeoutEnabledSet, this, &CoreNetwork::enablePingTimeout);
    connect(networkConfig(), &NetworkConfig::pingIntervalSet, this, &CoreNetwork::setPingInterval);
    connect(networkConfig(), &NetworkConfig::autoWhoEnabledSet, this, &CoreNetwork::setAutoWhoEnabled);
    connect(networkConfig(), &NetworkConfig::autoWhoIntervalSet, this, &CoreNetwork::setAutoWhoInterval);
    connect(networkConfig(), &NetworkConfig::autoWhoDelaySet, this, &CoreNetwork::setAutoWhoDelay);

    connect(&_autoReconnectTimer, &QTimer::timeout, this, &CoreNetwork::doAutoReconnect);
    connect(&_autoWhoTimer, &QTimer::timeout, this, &CoreNetwork::sendAutoWho);
    connect(&_autoWhoCycleTimer, &QTimer::timeout, this, &CoreNetwork::startAutoWhoCycle);
    connect(&_tokenBucketTimer, &QTimer::timeout, this, &CoreNetwork::checkTokenBucket);

    connect(&socket, &QAbstractSocket::connected, this, &CoreNetwork::onSocketInitialized);
    connect(&socket, &QAbstractSocket::errorOccurred, this, &CoreNetwork::onSocketError);  // Fixed signal
    connect(&socket, &QAbstractSocket::stateChanged, this, &CoreNetwork::onSocketStateChanged);
    connect(&socket, &QIODevice::readyRead, this, &CoreNetwork::onSocketHasData);
    connect(&socket, &QSslSocket::encrypted, this, &CoreNetwork::onSocketInitialized);
    connect(&socket, selectOverload<const QList<QSslError>&>(&QSslSocket::sslErrors), this, &CoreNetwork::onSslErrors);
    connect(this, &CoreNetwork::newEvent, coreSession()->eventManager(), &EventManager::postEvent);

    if (Quassel::isOptionSet("oidentd")) {
        connect(this, &CoreNetwork::socketInitialized, Core::instance()->oidentdConfigGenerator(), &OidentdConfigGenerator::addSocket);
        connect(this, &CoreNetwork::socketDisconnected, Core::instance()->oidentdConfigGenerator(), &OidentdConfigGenerator::removeSocket);
    }

    if (Quassel::isOptionSet("ident-daemon")) {
        connect(this, &CoreNetwork::socketInitialized, Core::instance()->identServer(), &IdentServer::addSocket);
        connect(this, &CoreNetwork::socketDisconnected, Core::instance()->identServer(), &IdentServer::removeSocket);
    }
}

CoreNetwork::~CoreNetwork()
{
    qDebug() << Q_FUNC_INFO << "Destroying CoreNetwork with ID:" << networkId();
    disconnect(&socket, nullptr, this, nullptr);
    if (!forceDisconnect()) {
        qWarning() << QString{"Could not disconnect from network %1 (network ID: %2, user ID: %3)"}
                          .arg(networkName())
                          .arg(networkId().toInt())
                          .arg(userId().toInt());
    }
}

bool CoreNetwork::forceDisconnect(int msecs)
{
    if (socket.state() == QAbstractSocket::UnconnectedState) {
        return true;
    }
    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        return socket.waitForDisconnected(msecs);
    }
    return true;
}

QString CoreNetwork::channelDecode(const QString& bufferName, const QByteArray& string) const
{
    if (!bufferName.isEmpty()) {
        IrcChannel* channel = ircChannel(bufferName);
        if (channel)
            return channel->decodeString(string);
    }
    return decodeString(string);
}

QString CoreNetwork::userDecode(const QString& userNick, const QByteArray& string) const
{
    IrcUser* user = ircUser(userNick);
    if (user)
        return user->decodeString(string);
    return decodeString(string);
}

QByteArray CoreNetwork::channelEncode(const QString& bufferName, const QString& string) const
{
    if (!bufferName.isEmpty()) {
        IrcChannel* channel = ircChannel(bufferName);
        if (channel)
            return channel->encodeString(string);
    }
    return encodeString(string);
}

QByteArray CoreNetwork::userEncode(const QString& userNick, const QString& string) const
{
    IrcUser* user = ircUser(userNick);
    if (user)
        return user->encodeString(string);
    return encodeString(string);
}

void CoreNetwork::connectToIrc(bool reconnecting)
{
    qDebug() << "[DEBUG] CoreNetwork::connectToIrc called for network" << networkId() << "reconnecting:" << reconnecting;
    
    if (_shuttingDown) {
        qDebug() << "[DEBUG] CoreNetwork::connectToIrc - shutting down, aborting connection";
        return;
    }
    
    // Reset shutdown flag when explicitly connecting
    _shuttingDown = false;

    if (Core::instance()->identServer()) {
        _socketId = Core::instance()->identServer()->addWaitingSocket();
    }

    if (_metricsServer) {
        _metricsServer->addNetwork(userId());
    }

    if (!reconnecting && useAutoReconnect() && _autoReconnectCount == 0) {
        _autoReconnectTimer.setInterval(autoReconnectInterval() * 1000);
        if (unlimitedReconnectRetries())
            _autoReconnectCount = -1;
        else
            _autoReconnectCount = autoReconnectRetries();
    }
    if (serverList().isEmpty()) {
        qWarning() << "Server list empty, ignoring connect request!";
        return;
    }
    CoreIdentity* identity = identityPtr();
    if (!identity) {
        qWarning() << "Invalid identity configures, ignoring connect request!";
        return;
    }

    _quitReason.clear();

    _capsQueuedIndividual.clear();
    _capsQueuedBundled.clear();
    clearCaps();
    _capNegotiationActive = false;
    _capInitialNegotiationEnded = false;

    if (useRandomServer()) {
        _lastUsedServerIndex = QRandomGenerator::global()->bounded(serverList().size());  // Fixed qrand
    }
    else if (_previousConnectionAttemptFailed) {
        showMessage(NetworkInternalMessage(Message::Server, BufferInfo::StatusBuffer, "", tr("Connection failed. Cycling to next server...")));
        if (++_lastUsedServerIndex >= serverList().size()) {
            _lastUsedServerIndex = 0;
        }
    }
    else {
        _lastUsedServerIndex = 0;
    }

    Server server = usedServer();
    displayStatusMsg(tr("Connecting to %1:%2...").arg(server.host).arg(server.port));
    showMessage(
        NetworkInternalMessage(Message::Server, BufferInfo::StatusBuffer, "", tr("Connecting to %1:%2...").arg(server.host).arg(server.port)));

    if (server.useProxy) {
        QNetworkProxy proxy((QNetworkProxy::ProxyType)server.proxyType, server.proxyHost, server.proxyPort, server.proxyUser, server.proxyPass);
        socket.setProxy(proxy);
    }
    else {
        socket.setProxy(QNetworkProxy::NoProxy);
    }

    enablePingTimeout();

    setPongTimestampValid(false);

    if (!server.useProxy) {
        QHostInfo::fromName(server.host);
    }
    if (server.useSsl) {
        qDebug() << "[DEBUG] Connecting with SSL to" << server.host << "port" << server.port;
        CoreIdentity* identity = identityPtr();
        if (identity) {
            socket.setLocalCertificate(identity->sslCert());
            socket.setPrivateKey(identity->sslKey());
        }
        socket.connectToHostEncrypted(server.host, server.port);
    }
    else {
        qDebug() << "[DEBUG] Connecting without SSL to" << server.host << "port" << server.port;
        socket.connectToHost(server.host, server.port);
    }
}

void CoreNetwork::disconnectFromIrc(bool requested, const QString& reason, bool withReconnect)
{
    qDebug() << Q_FUNC_INFO << "Disconnecting network:" << networkId() << "at address:" << this;
    _disconnectExpected = true;
    _quitRequested = requested;
    if (!withReconnect) {
        _autoReconnectTimer.stop();
        _autoReconnectCount = 0;
    }
    disablePingTimeout();
    _msgQueue.clear();
    if (_metricsServer) {
        _metricsServer->messageQueue(userId(), 0);
    }
    _shuttingDown = true;
    qDebug() << Q_FUNC_INFO << "Clearing event queue for network:" << networkId();
    coreSession()->eventManager()->postEvent(nullptr);

    _disconnectExpected = true;
    _quitRequested = requested;
    if (!withReconnect) {
        _autoReconnectTimer.stop();
        _autoReconnectCount = 0;
    }
    disablePingTimeout();
    _msgQueue.clear();
    if (_metricsServer) {
        _metricsServer->messageQueue(userId(), 0);
    }

    IrcUser* me_ = ircUser(myNick());  // Fixed me()
    if (me_) {
        QString awayMsg;
        if (me_->away())  // Fixed isAway
            awayMsg = me_->awayMessage();
        Core::setAwayMessage(userId(), networkId(), awayMsg);
    }

    if (reason.isEmpty() && identityPtr())
        _quitReason = identityPtr()->quitReason();
    else
        _quitReason = reason;

    showMessage(NetworkInternalMessage(Message::Server,
                                       BufferInfo::StatusBuffer,
                                       "",
                                       tr("Disconnecting. (%1)").arg((!requested && !withReconnect) ? tr("Core Shutdown") : _quitReason)));
    if (socket.state() == QAbstractSocket::UnconnectedState) {
        onSocketDisconnected();
    }
    else {
        if (socket.state() == QAbstractSocket::ConnectedState) {
            userInputHandler()->issueQuit(_quitReason, _shuttingDown);
        }
        else {
            socket.close();
        }
        if (socket.state() != QAbstractSocket::UnconnectedState) {
            _socketCloseTimer.start(10000);
        }
    }
}

void CoreNetwork::onSocketCloseTimeout()
{
    qWarning() << QString{"Timed out quitting network %1 (network ID: %2, user ID: %3)"}
                      .arg(networkName())
                      .arg(networkId().toInt())
                      .arg(userId().toInt());
    socket.abort();
}

void CoreNetwork::shutdown()
{
    _shuttingDown = true;
    disconnectFromIrc(false, {}, false);
}

void CoreNetwork::userInput(const BufferInfo& buf, QString msg)
{
    userInputHandler()->handleUserInput(buf, msg);
}

void CoreNetwork::putRawLine(const QByteArray& s, bool prepend)
{
    if (_tokenBucket > 0 || (_skipMessageRates && _msgQueue.isEmpty())) {
        writeToSocket(s);
    }
    else {
        if (prepend) {
            _msgQueue.prepend(s);
        }
        else {
            _msgQueue.append(s);
        }
        if (_metricsServer) {
            _metricsServer->messageQueue(userId(), _msgQueue.size());
        }
    }
}

void CoreNetwork::putCmd(
    const QString& cmd, const QList<QByteArray>& params, const QByteArray& prefix, const QHash<IrcTagKey, QString>& tags, bool prepend)
{
    putRawLine(IrcEncoder::writeMessage(tags, prefix, cmd, params), prepend);
}

void CoreNetwork::putCmd(const QString& cmd,
                         const QList<QList<QByteArray>>& params,
                         const QByteArray& prefix,
                         const QHash<IrcTagKey, QString>& tags,
                         bool prependAll)
{
    QListIterator<QList<QByteArray>> i(params);
    while (i.hasNext()) {
        QList<QByteArray> msg = i.next();
        putCmd(cmd, msg, prefix, tags, prependAll);
    }
}

void CoreNetwork::setChannelJoined(const QString& channel)
{
    queueAutoWhoOneshot(channel);

    Core::setChannelPersistent(userId(), networkId(), channel, true);
    Core::setPersistentChannelKey(userId(), networkId(), channel, _channelKeys[channel.toLower()]);
}

void CoreNetwork::setChannelParted(const QString& channel)
{
    removeChannelKey(channel);
    _autoWhoQueue.removeAll(channel.toLower());
    _autoWhoPending.remove(channel.toLower());

    Core::setChannelPersistent(userId(), networkId(), channel, false);
}

void CoreNetwork::addChannelKey(const QString& channel, const QString& key)
{
    if (key.isEmpty()) {
        removeChannelKey(channel);
    }
    else {
        _channelKeys[channel.toLower()] = key;
    }
}

void CoreNetwork::removeChannelKey(const QString& channel)
{
    _channelKeys.remove(channel.toLower());
}

#ifdef HAVE_QCA2
Cipher* CoreNetwork::cipher(const QString& target)
{
    if (target.isEmpty())
        return nullptr;

    if (!Cipher::neededFeaturesAvailable())
        return nullptr;

    auto* channel = qobject_cast<CoreIrcChannel*>(ircChannel(target));
    if (channel) {
        return channel->cipher();
    }
    auto* user = qobject_cast<CoreIrcUser*>(ircUser(target));
    if (user) {
        return user->cipher();
    }
    else if (!isChannelName(target)) {
        return qobject_cast<CoreIrcUser*>(newIrcUser(target))->cipher();
    }
    return nullptr;
}

QByteArray CoreNetwork::cipherKey(const QString& target) const
{
    auto* c = qobject_cast<CoreIrcChannel*>(ircChannel(target));
    if (c)
        return c->cipher()->key();

    auto* u = qobject_cast<CoreIrcUser*>(ircUser(target));
    if (u)
        return u->cipher()->key();

    return QByteArray();
}

void CoreNetwork::setCipherKey(const QString& target, const QByteArray& key)
{
    auto* c = qobject_cast<CoreIrcChannel*>(ircChannel(target));
    if (c) {
        c->setEncrypted(c->cipher()->setKey(key));
        coreSession()->setBufferCipher(networkId(), target, key);
        return;
    }

    auto* u = qobject_cast<CoreIrcUser*>(ircUser(target));
    if (!u && !isChannelName(target))
        u = qobject_cast<CoreIrcUser*>(newIrcUser(target));

    if (u) {
        u->setEncrypted(u->cipher()->setKey(key));
        coreSession()->setBufferCipher(networkId(), target, key);
        return;
    }
}

bool CoreNetwork::cipherUsesCBC(const QString& target)
{
    auto* c = qobject_cast<CoreIrcChannel*>(ircChannel(target));
    if (c)
        return c->cipher()->usesCBC();
    auto* u = qobject_cast<CoreIrcUser*>(ircUser(target));
    if (u)
        return u->cipher()->usesCBC();

    return false;
}
#endif /* HAVE_QCA2 */

bool CoreNetwork::setAutoWhoDone(const QString& name)
{
    QString chanOrNick = name.toLower();
    if (_autoWhoPending.value(chanOrNick, 0) <= 0)
        return false;
    if (--_autoWhoPending[chanOrNick] <= 0)
        _autoWhoPending.remove(chanOrNick);
    return true;
}

void CoreNetwork::setMyNick(const QString& mynick)
{
    qDebug() << "[DEBUG] CoreNetwork::setMyNick called for network" << networkId() << "nick:" << mynick << "connectionState:" << connectionState();
    Network::setMyNick(mynick);
    if (connectionState() == Network::Initializing) {
        qDebug() << "[DEBUG] CoreNetwork::setMyNick calling networkInitialized()";
        networkInitialized();
    } else {
        qDebug() << "[DEBUG] CoreNetwork::setMyNick NOT calling networkInitialized() - wrong state:" << connectionState();
    }
}

void CoreNetwork::onSocketHasData()
{
    while (socket.canReadLine()) {
        QByteArray s = socket.readLine();
        if (_metricsServer) {
            _metricsServer->receiveDataNetwork(userId(), s.size());
        }
        if (s.endsWith("\r\n"))
            s.chop(2);
        else if (s.endsWith("\n"))
            s.chop(1);
        if (_shuttingDown) {
            qWarning() << Q_FUNC_INFO << "CoreNetwork is shutting down, skipping event emission for network:" << networkId();
            return;
        }
        qDebug() << Q_FUNC_INFO << "Emitting NetworkDataEvent for network:" << networkId() << "at address:" << this;
        NetworkDataEvent* event = new NetworkDataEvent(EventManager::NetworkIncoming, this, s, this);
        event->setTimestamp(QDateTime::currentDateTimeUtc());
        qDebug() << Q_FUNC_INFO << "Created NetworkDataEvent at address:" << event << "with network:" << event->network();
        emit newEvent(event);
    }
}

void CoreNetwork::onSocketError(QAbstractSocket::SocketError error)
{
    if (_disconnectExpected && error == QAbstractSocket::RemoteHostClosedError) {
        return;
    }

    _previousConnectionAttemptFailed = true;
    qWarning() << qPrintable(tr("Could not connect to %1 (%2)").arg(networkName(), socket.errorString()));
    emit connectionError(socket.errorString());  // Fixed emitConnectionError
    showMessage(NetworkInternalMessage(Message::Error, BufferInfo::StatusBuffer, "", tr("Connection failure: %1").arg(socket.errorString())));
    if (socket.state() < QAbstractSocket::ConnectedState) {
        onSocketDisconnected();
    }
}

void CoreNetwork::onSocketInitialized()
{
    CoreIdentity* identity = identityPtr();
    if (!identity) {
        qCritical() << "Identity invalid!";
        disconnectFromIrc();
        return;
    }

    Server server = usedServer();

    if (!server.useSsl || !socket.isEncrypted()) {
        emit socketInitialized(identity, localAddress(), localPort(), peerAddress(), peerPort(), _socketId);
    }

    if (server.useSsl && !socket.isEncrypted()) {
        return;
    }

    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);

    updateRateLimiting(true);
    resetTokenBucket();

    showMessage(NetworkInternalMessage(Message::Server, BufferInfo::StatusBuffer, "", tr("Requesting capability list...")));
    putRawLine(serverEncode(QString("CAP LS 302")));

    if (!server.password.isEmpty()) {
        putRawLine(serverEncode(QString("PASS %1").arg(server.password)));
    }
    QString nick;
    if (identity->nicks().isEmpty()) {
        nick = "quassel";
        qWarning() << "CoreNetwork::socketInitialized(): no nicks supplied for identity Id" << identity->id();
    }
    else {
        nick = identity->nicks()[0];
    }
    putRawLine(serverEncode(QString("NICK %1").arg(nick)));
    putRawLine(serverEncode(QString("USER %1 8 * :%2").arg(coreSession()->strictCompliantIdent(identity), identity->realName())));
}

void CoreNetwork::onSocketDisconnected()
{
    qDebug() << "[DEBUG] CoreNetwork::onSocketDisconnected() called for network" << networkId();
    disablePingTimeout();
    _msgQueue.clear();
    if (_metricsServer) {
        _metricsServer->messageQueue(userId(), 0);
    }

    _autoWhoCycleTimer.stop();
    _autoWhoTimer.stop();
    _autoWhoQueue.clear();
    _autoWhoPending.clear();

    _socketCloseTimer.stop();

    _tokenBucketTimer.stop();

    IrcUser* me_ = ircUser(myNick());  // Fixed me()
    if (me_) {
        for (const QString& channel : me_->channels()) {
            showMessage(NetworkInternalMessage(Message::Quit, BufferInfo::ChannelBuffer, channel, _quitReason, me_->hostmask()));
        }
    }

    qDebug() << "[DEBUG] CoreNetwork::onSocketDisconnected() about to call setConnected(false), current state:" << isConnected();
    setConnected(false);
    qDebug() << "[DEBUG] CoreNetwork::onSocketDisconnected() called setConnected(false), new state:" << isConnected();
    emit disconnected(networkId());
    emit socketDisconnected(identityPtr(), localAddress(), localPort(), peerAddress(), peerPort(), _socketId);
    _disconnectExpected = false;
    if (_quitRequested) {
        _quitRequested = false;
        setConnectionState(Network::Disconnected);
        Core::setNetworkConnected(userId(), networkId(), false);
    }
    else if (_autoReconnectCount != 0) {
        setConnectionState(Network::Reconnecting);
        if (_autoReconnectCount == -1 || _autoReconnectCount == autoReconnectRetries())
            doAutoReconnect();
        else
            _autoReconnectTimer.start();
    }

    if (_metricsServer) {
        _metricsServer->removeNetwork(userId());
    }
}

void CoreNetwork::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    Network::ConnectionState state;
    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        state = Network::Disconnected;
        onSocketDisconnected();
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

void CoreNetwork::networkInitialized()
{
    qDebug() << "[DEBUG] CoreNetwork::networkInitialized() called for network" << networkId();
    setConnectionState(Network::Initialized);
    setConnected(true);
    _disconnectExpected = false;
    _quitRequested = false;

    updateRateLimiting();

    if (useAutoReconnect()) {
        _autoReconnectCount = unlimitedReconnectRetries() ? -1 : autoReconnectRetries();
    }

    QString awayMsg = Core::awayMessage(userId(), networkId());
    if (!awayMsg.isEmpty()) {
        userInputHandler()->handleAway(BufferInfo(), awayMsg, true);
    }

    sendPerform();

    _sendPings = true;

    if (networkConfig()->autoWhoEnabled()) {
        _autoWhoCycleTimer.start();
        _autoWhoTimer.start();
        startAutoWhoCycle();
    }

    Core::bufferInfo(userId(), networkId(), BufferInfo::StatusBuffer);
    Core::setNetworkConnected(userId(), networkId(), true);
}

void CoreNetwork::sendPerform()
{
    BufferInfo statusBuf = BufferInfo::fakeStatusBuffer(networkId());

    if (useAutoIdentify() && !autoIdentifyService().isEmpty() && !autoIdentifyPassword().isEmpty()) {
        userInputHandler()->handleMsg(statusBuf, QString("%1 IDENTIFY %2").arg(autoIdentifyService(), autoIdentifyPassword()));
    }

    IrcUser* me_ = ircUser(myNick());  // Fixed me()
    if (me_) {
        if (!me_->userModes().isEmpty()) {
            restoreUserModes();
        }
        else {
            connect(me_, &IrcUser::userModesSet, this, &CoreNetwork::restoreUserModes);
            connect(me_, &IrcUser::userModesAdded, this, &CoreNetwork::restoreUserModes);
        }
    }

    for (const QString& line : perform()) {
        if (!line.isEmpty())
            userInput(statusBuf, line);
    }

    if (rejoinChannels()) {
        QStringList channels, keys;
        for (const QString& chan : coreSession()->persistentChannels(networkId()).keys()) {
            QString key = channelKey(chan);
            if (!key.isEmpty()) {
                channels.prepend(chan);
                keys.prepend(key);
            }
            else {
                channels.append(chan);
            }
        }
        QString joinString = QString("%1 %2").arg(channels.join(",")).arg(keys.join(",")).trimmed();
        if (!joinString.isEmpty())
            userInputHandler()->handleJoin(statusBuf, joinString);
    }
}

void CoreNetwork::restoreUserModes()
{
    IrcUser* me_ = ircUser(myNick());  // Fixed me()
    Q_ASSERT(me_);

    disconnect(me_, &IrcUser::userModesSet, this, &CoreNetwork::restoreUserModes);
    disconnect(me_, &IrcUser::userModesAdded, this, &CoreNetwork::restoreUserModes);

    QString modesDelta = Core::userModes(userId(), networkId());
    QString currentModes = me_->userModes();

    QString addModes, removeModes;
    if (modesDelta.contains('-')) {
        addModes = modesDelta.section('-', 0, 0);
        removeModes = modesDelta.section('-', 1);
    }
    else {
        addModes = modesDelta;
    }

    if (!currentModes.isEmpty()) {
        addModes.remove(QRegularExpression(QString("[%1]").arg(currentModes)));
    }
    if (currentModes.isEmpty())
        removeModes = QString();
    else
        removeModes.remove(QRegularExpression(QString("[^%1]").arg(currentModes)));

    if (addModes.isEmpty() && removeModes.isEmpty())
        return;

    if (!addModes.isEmpty())
        addModes = '+' + addModes;
    if (!removeModes.isEmpty())
        removeModes = '-' + removeModes;

    putRawLine(serverEncode(QString("MODE %1 %2%3").arg(me_->nick()).arg(addModes).arg(removeModes)));
}

void CoreNetwork::updateIssuedModes(const QString& requestedModes)
{
    QString addModes;
    QString removeModes;
    bool addMode = true;

    for (auto requestedMode : requestedModes) {
        if (requestedMode == '+') {
            addMode = true;
            continue;
        }
        if (requestedMode == '-') {
            addMode = false;
            continue;
        }
        if (addMode) {
            addModes += requestedMode;
        }
        else {
            removeModes += requestedMode;
        }
    }

    QString addModesOld = _requestedUserModes.section('-', 0, 0);
    QString removeModesOld = _requestedUserModes.section('-', 1);

    if (!addModesOld.isEmpty()) {
        addModes.remove(QRegularExpression(QString("[%1]").arg(addModesOld)));
    }
    if (!removeModes.isEmpty()) {
        addModesOld.remove(QRegularExpression(QString("[%1]").arg(removeModes)));
    }
    addModes += addModesOld;

    if (!removeModesOld.isEmpty()) {
        removeModes.remove(QRegularExpression(QString("[%1]").arg(removeModesOld)));
    }
    if (!addModes.isEmpty()) {
        removeModesOld.remove(QRegularExpression(QString("[%1]").arg(addModes)));
    }
    removeModes += removeModesOld;

    _requestedUserModes = QString("%1-%2").arg(addModes).arg(removeModes);
}

void CoreNetwork::updatePersistentModes(QString addModes, QString removeModes)
{
    QString persistentUserModes = Core::userModes(userId(), networkId());

    QString requestedAdd = _requestedUserModes.section('-', 0, 0);
    QString requestedRemove = _requestedUserModes.section('-', 1);

    QString persistentAdd, persistentRemove;
    if (persistentUserModes.contains('-')) {
        persistentAdd = persistentUserModes.section('-', 0, 0);
        persistentRemove = persistentUserModes.section('-', 1);
    }
    else {
        persistentAdd = persistentUserModes;
    }

    if (requestedAdd.isEmpty())
        addModes = QString();
    else
        addModes.remove(QRegularExpression(QString("[^%1]").arg(requestedAdd)));

    if (requestedRemove.isEmpty())
        removeModes = QString();
    else
        removeModes.remove(QRegularExpression(QString("[^%1]").arg(requestedRemove)));

    if (!addModes.isEmpty()) {
        persistentAdd.remove(QRegularExpression(QString("[%1]").arg(addModes)));
    }
    if (!removeModes.isEmpty()) {
        persistentRemove.remove(QRegularExpression(QString("[%1]").arg(removeModes)));
    }

    if (!removeModes.isEmpty()) {
        persistentAdd.remove(QRegularExpression(QString("[%1]").arg(removeModes)));
    }
    if (!addModes.isEmpty()) {
        persistentRemove.remove(QRegularExpression(QString("[%1]").arg(addModes)));
    }

    if (!addModes.isEmpty()) {
        requestedAdd.remove(QRegularExpression(QString("[%1]").arg(addModes)));
    }
    if (!removeModes.isEmpty()) {
        requestedRemove.remove(QRegularExpression(QString("[%1]").arg(removeModes)));
    }
    _requestedUserModes = QString("%1-%2").arg(requestedAdd).arg(requestedRemove);

    persistentAdd += addModes;
    persistentRemove += removeModes;
    Core::setUserModes(userId(), networkId(), QString("%1-%2").arg(persistentAdd).arg(persistentRemove));
}

void CoreNetwork::resetPersistentModes()
{
    _requestedUserModes = QString('-');
    Core::setUserModes(userId(), networkId(), QString());
}

void CoreNetwork::setUseAutoReconnect(bool use)
{
    Network::setUseAutoReconnect(use);
    if (!use)
        _autoReconnectTimer.stop();
}

void CoreNetwork::setAutoReconnectInterval(quint32 interval)
{
    Network::setAutoReconnectInterval(interval);
    _autoReconnectTimer.setInterval(interval * 1000);
}

void CoreNetwork::setAutoReconnectRetries(quint16 retries)
{
    Network::setAutoReconnectRetries(retries);
    if (_autoReconnectCount != 0) {
        if (unlimitedReconnectRetries())
            _autoReconnectCount = -1;
        else
            _autoReconnectCount = autoReconnectRetries();
    }
}

void CoreNetwork::doAutoReconnect()
{
    if (connectionState() != Network::Disconnected && connectionState() != Network::Reconnecting) {
        qWarning() << "CoreNetwork::doAutoReconnect(): Cannot reconnect while not being disconnected!";
        return;
    }
    if (_autoReconnectCount > 0 || _autoReconnectCount == -1)
        _autoReconnectCount--;
    connectToIrc(true);
}

void CoreNetwork::sendPing()
{
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (_pingCount != 0) {
        qDebug() << "UserId:" << userId() << "Network:" << networkName() << "missed" << _pingCount << "pings."
                 << "BA:" << socket.bytesAvailable() << "BTW:" << socket.bytesToWrite();
    }
    if ((int)_pingCount >= networkConfig()->maxPingCount() && (now - _lastPingTime) <= (_pingTimer.interval() + (1 * 1000))) {
        disconnectFromIrc(false, QString("No Ping reply in %1 seconds.").arg(_pingCount * _pingTimer.interval() / 1000), true);
    }
    else {
        _lastPingTime = now;
        _pingCount++;
        if (_sendPings) {
            _pongReplyPending = true;
            userInputHandler()->handlePing(BufferInfo(), QString());
        }
    }
}

void CoreNetwork::enablePingTimeout(bool enable)
{
    if (!enable)
        disablePingTimeout();
    else {
        resetPingTimeout();
        resetPongReplyPending();
        if (networkConfig()->pingTimeoutEnabled())
            _pingTimer.start();
    }
}

void CoreNetwork::disablePingTimeout()
{
    _pingTimer.stop();
    _sendPings = false;
    resetPingTimeout();
    resetPongReplyPending();
}

void CoreNetwork::setPingInterval(int interval)
{
    _pingTimer.setInterval(interval * 1000);
}

void CoreNetwork::setPongTimestampValid(bool validTimestamp)
{
    _pongTimestampValid = validTimestamp;
}

void CoreNetwork::updateRateLimiting(bool forceUnlimited)
{
    if (useCustomMessageRate() || forceUnlimited) {
        _messageDelay = messageRateDelay();
        _burstSize = messageRateBurstSize();
        if (_burstSize < 1) {
            qWarning() << "Invalid messageRateBurstSize data, cannot have zero message burst size!" << _burstSize;
            _burstSize = 1;
        }
        if (_tokenBucket > _burstSize) {
            _tokenBucket = _burstSize;
        }
        _skipMessageRates = (unlimitedMessageRate() || forceUnlimited);
        if (_skipMessageRates) {
            if (!_msgQueue.isEmpty()) {
                qDebug() << "Outgoing message queue contains messages while disabling rate limiting. Sending remaining queued messages...";
                _tokenBucketTimer.start(100);
            }
            else {
                _tokenBucketTimer.stop();
            }
        }
        else {
            _tokenBucketTimer.start(_messageDelay);
        }
    }
    else {
        _skipMessageRates = false;
        _messageDelay = 2200;
        _burstSize = 5;
        if (_tokenBucket > _burstSize) {
            _tokenBucket = _burstSize;
        }
        _tokenBucketTimer.start(_messageDelay);
    }
}

void CoreNetwork::resetTokenBucket()
{
    _tokenBucket = _burstSize;
}

void CoreNetwork::serverCapAdded(const QString& capability)
{
    if (skipCaps().contains(capability)) {
        return;
    }

    if (capability == IrcCap::SASL) {
        if (useSasl()) {
            queueCap(capability);
        }
    }
    else if (IrcCap::knownCaps.contains(capability)) {
        queueCap(capability);
    }
}

void CoreNetwork::serverCapAcknowledged(const QString& capability)
{
    if (capability == IrcCap::AWAY_NOTIFY) {
        setAutoWhoEnabled(false);
    }

    if (capability == IrcCap::SASL) {
        if (!identityPtr()->sslCert().isNull()) {
            if (availableCaps().contains("sasl-external")) {  // Fixed saslMaybeSupports
                putRawLine(serverEncode("AUTHENTICATE EXTERNAL"));
            }
            else {
                showMessage(
                    NetworkInternalMessage(Message::Error, BufferInfo::StatusBuffer, "", tr("SASL EXTERNAL authentication not supported")));
                sendNextCap();
            }
        }
        else {
            if (availableCaps().contains("sasl-plain")) {  // Fixed saslMaybeSupports
                putRawLine(serverEncode("AUTHENTICATE PLAIN"));
            }
            else {
                showMessage(
                    NetworkInternalMessage(Message::Error, BufferInfo::StatusBuffer, "", tr("SASL PLAIN authentication not supported")));
                sendNextCap();
            }
        }
    }
}

void CoreNetwork::serverCapRemoved(const QString& capability)
{
    if (capability == IrcCap::AWAY_NOTIFY) {
        setAutoWhoEnabled(networkConfig()->autoWhoEnabled());
    }
}

void CoreNetwork::queueCap(const QString& capability)
{
    QString _capLowercase = capability.toLower();

    if (capsRequiringConfiguration.contains(_capLowercase)) {
        if (!_capsQueuedIndividual.contains(_capLowercase)) {
            _capsQueuedIndividual.append(_capLowercase);
        }
    }
    else {
        if (!_capsQueuedBundled.contains(_capLowercase)) {
            _capsQueuedBundled.append(_capLowercase);
        }
    }
}

QString CoreNetwork::takeQueuedCaps()
{
    // Clear the record of the most recently negotiated capability bundle.  Does nothing if the list
    // is empty.
    _capsQueuedLastBundle.clear();

    // First, negotiate all the standalone capabilities that require additional configuration.
    if (!_capsQueuedIndividual.empty()) {
        // We have an individual capability available.  Take the first and pass it back.
        return _capsQueuedIndividual.takeFirst();
    }
    else if (!_capsQueuedBundled.empty()) {
        // We have capabilities available that can be grouped.  Try to fit in as many as within the
        // maximum length.
        // See CoreNetwork::maxCapRequestLength

        // Response must have at least one capability regardless of max length for anything to
        // happen.
        QString capBundle = _capsQueuedBundled.takeFirst();
        QString nextCap("");
        while (!_capsQueuedBundled.empty()) {
            // As long as capabilities remain, get the next...
            nextCap = _capsQueuedBundled.first();
            if ((capBundle.length() + 1 + nextCap.length()) <= maxCapRequestLength) {
                // [capability + 1 for a space + this new capability] fit within length limits
                // Add it to formatted list
                capBundle.append(" " + nextCap);
                // Add it to most recent bundle of requested capabilities (simplifies retry logic)
                _capsQueuedLastBundle.append(nextCap);
                // Then remove it from the queue
                _capsQueuedBundled.removeFirst();
            }
            else {
                // We've reached the length limit for a single capability request, stop adding more
                break;
            }
        }
        // Return this space-separated set of capabilities, removing any extra spaces
        return capBundle.trimmed();
    }
    else {
        // No capabilities left to negotiate, return an empty string.
        return QString();
    }
}

void CoreNetwork::retryCapsIndividually()
{
    // The most recent set of capabilities got denied by the IRC server.  As we don't know what got
    // denied, try each capability individually.
    if (_capsQueuedLastBundle.empty()) {
        // No most recently tried capability set, just return.
        return;
        // Note: there's little point in retrying individually requested caps during negotiation.
        // We know the individual capability was the one that failed, and it's not likely it'll
        // suddenly start working within a few seconds.  'cap-notify' provides a better system for
        // handling capability removal and addition.
    }

    // This should be fairly rare, e.g. services restarting during negotiation, so simplicity wins
    // over efficiency.  If this becomes an issue, implement a binary splicing system instead,
    // keeping track of which halves of the group fail, dividing the set each time.

    // Add most recently tried capability set to individual list, re-requesting them one at a time
    _capsQueuedIndividual.append(_capsQueuedLastBundle);
    // Warn of this issue to explain the slower login.  Servers usually shouldn't trigger this.
    showMessage(NetworkInternalMessage(Message::Server,
                                       BufferInfo::StatusBuffer,
                                       "",
                                       tr("Could not negotiate some capabilities, retrying individually (%1)...")
                                           .arg(_capsQueuedLastBundle.join(", "))));
    // Capabilities are already removed from the capability bundle queue via takeQueuedCaps(), no
    // need to remove them here.
    // Clear the most recently tried set to reduce risk that mistakes elsewhere causes retrying
    // indefinitely.
    _capsQueuedLastBundle.clear();
}

void CoreNetwork::beginCapNegotiation()
{
    QStringList capsSkipped;
    if (!skipCaps().isEmpty() && !availableCaps().isEmpty()) {  // Fixed caps()
        auto sortedCaps = availableCaps();
        sortedCaps.sort();
        std::set_intersection(skipCaps().cbegin(), skipCaps().cend(), sortedCaps.cbegin(), sortedCaps.cend(), std::back_inserter(capsSkipped));
    }

    if (!capsPendingNegotiation()) {
        QString capStatusMsg;
        if (availableCaps().isEmpty()) {  // Fixed caps()
            capStatusMsg = tr("No capabilities available");
        }
        else if (enabledCaps().isEmpty()) {
            capStatusMsg = tr("None of the capabilities provided by the server are supported (found: %1)").arg(availableCaps().join(", "));
        }
        else {
            capStatusMsg = tr("No additional capabilities are supported (found: %1; currently enabled: %2)")
                               .arg(availableCaps().join(", "), enabledCaps().join(", "));
        }
        showMessage(NetworkInternalMessage(Message::Server, BufferInfo::StatusBuffer, "", capStatusMsg));

        if (!capsSkipped.isEmpty()) {
            showMessage(
                NetworkInternalMessage(Message::Server,
                                       BufferInfo::StatusBuffer,
                                       "",
                                       tr("Quassel is configured to ignore some capabilities (skipped: %1)").arg(capsSkipped.join(", "))));
        }

        endCapNegotiation();
        return;
    }

    _capNegotiationActive = true;
    showMessage(NetworkInternalMessage(Message::Server,
                                       BufferInfo::StatusBuffer,
                                       "",
                                       tr("Ready to negotiate (found: %1)").arg(availableCaps().join(", "))));

    if (!capsSkipped.isEmpty()) {
        showMessage(NetworkInternalMessage(Message::Server,
                                           BufferInfo::StatusBuffer,
                                           "",
                                           tr("Quassel is configured to ignore some capabilities (skipped: %1)").arg(capsSkipped.join(", "))));
    }

    QString queuedCapsDisplay = _capsQueuedIndividual.join(", ")
                                + ((!_capsQueuedIndividual.empty() && !_capsQueuedBundled.empty()) ? ", " : "")
                                + _capsQueuedBundled.join(", ");
    showMessage(NetworkInternalMessage(Message::Server,
                                       BufferInfo::StatusBuffer,
                                       "",
                                       tr("Negotiating capabilities (requesting: %1)...").arg(queuedCapsDisplay)));

    sendNextCap();
}

void CoreNetwork::sendNextCap()
{
    if (capsPendingNegotiation()) {
        putRawLine(serverEncode(QString("CAP REQ :%1").arg(takeQueuedCaps())));
    }
    else {
        if (useSasl() && !enabledCaps().contains(IrcCap::SASL))
            showMessage(NetworkInternalMessage(Message::Error,
                                               BufferInfo::StatusBuffer,
                                               "",
                                               tr("SASL authentication currently not supported by server")));

        if (_capNegotiationActive) {
            showMessage(NetworkInternalMessage(Message::Server,
                                               BufferInfo::StatusBuffer,
                                               "",
                                               tr("Capability negotiation finished (enabled: %1)").arg(enabledCaps().join(", "))));
            _capNegotiationActive = false;
        }

        endCapNegotiation();
    }
}

void CoreNetwork::endCapNegotiation()
{
    if (!_capInitialNegotiationEnded) {
        putRawLine(serverEncode(QString("CAP END")));
        _capInitialNegotiationEnded = true;
    }
}

void CoreNetwork::startAutoWhoCycle()
{
    if (!_autoWhoQueue.isEmpty()) {
        _autoWhoCycleTimer.stop();
        return;
    }
    _autoWhoQueue = ircChannels()->keys();
}

void CoreNetwork::queueAutoWhoOneshot(const QString& name)
{
    if (!_autoWhoQueue.contains(name.toLower())) {
        _autoWhoQueue.prepend(name.toLower());
    }
    if (enabledCaps().contains(IrcCap::AWAY_NOTIFY)) {
        setAutoWhoEnabled(true);
    }
}

void CoreNetwork::setAutoWhoDelay(int delay)
{
    _autoWhoTimer.setInterval(delay * 1000);
}

void CoreNetwork::setAutoWhoInterval(int interval)
{
    _autoWhoCycleTimer.setInterval(interval * 1000);
}

void CoreNetwork::setAutoWhoEnabled(bool enabled)
{
    if (enabled && isConnected() && !_autoWhoTimer.isActive())
        _autoWhoTimer.start();
    else if (!enabled) {
        _autoWhoTimer.stop();
        _autoWhoCycleTimer.stop();
    }
}

void CoreNetwork::sendAutoWho()
{
    if (_autoWhoPending.count())
        return;

    while (!_autoWhoQueue.isEmpty()) {
        QString chanOrNick = _autoWhoQueue.takeFirst();
        IrcChannel* ircchan = ircChannel(chanOrNick);
        IrcUser* ircuser = ircUser(chanOrNick);
        if (ircchan) {
            if (networkConfig()->autoWhoNickLimit() > 0 && ircchan->userList().size() >= networkConfig()->autoWhoNickLimit()
                && !enabledCaps().contains(IrcCap::AWAY_NOTIFY))
                continue;
            _autoWhoPending[chanOrNick.toLower()]++;
        }
        else if (ircuser) {
            _autoWhoPending[ircuser->nick().toLower()]++;
        }
        else {
            qDebug() << "Skipping who polling of unknown channel or nick" << chanOrNick;
            continue;
        }
        if (supports().contains("WHOX")) {
            putRawLine(serverEncode(QString("WHO %1 n%chtsunfra,%2").arg(chanOrNick, QString::number(IrcCap::ACCOUNT_NOTIFY_WHOX_NUM))));
        }
        else {
            putRawLine(serverEncode(QString("WHO %1").arg(chanOrNick)));
        }
        break;
    }

    if (_autoWhoQueue.isEmpty() && networkConfig()->autoWhoEnabled() && !_autoWhoCycleTimer.isActive()
        && !enabledCaps().contains(IrcCap::AWAY_NOTIFY)) {
        _autoWhoCycleTimer.start();
        startAutoWhoCycle();
    }
    else if (enabledCaps().contains(IrcCap::AWAY_NOTIFY) && _autoWhoCycleTimer.isActive()) {
        _autoWhoCycleTimer.stop();
    }
}

void CoreNetwork::onSslErrors(const QList<QSslError>& sslErrors)
{
    Server server = usedServer();
    if (server.sslVerify) {
        QString sslErrorMessage = tr("Encrypted connection couldn't be verified, disconnecting since verification is required");
        if (!sslErrors.empty()) {
            sslErrorMessage.append(tr(" (Reason: %1)").arg(sslErrors.first().errorString()));
        }
        showMessage(NetworkInternalMessage(Message::Error, BufferInfo::StatusBuffer, "", sslErrorMessage));

        disconnectFromIrc(false, QString("Encrypted connection not verified"), true);
    }
    else {
        QString sslErrorMessage = tr("Encrypted connection couldn't be verified, continuing since verification is not required");
        if (!sslErrors.empty()) {
            sslErrorMessage.append(tr(" (Reason: %1)").arg(sslErrors.first().errorString()));
        }
        showMessage(NetworkInternalMessage(Message::Info, BufferInfo::StatusBuffer, "", sslErrorMessage));

        socket.ignoreSslErrors();
    }
}

void CoreNetwork::checkTokenBucket()
{
    if (_skipMessageRates) {
        if (_msgQueue.isEmpty()) {
            _tokenBucketTimer.stop();
            return;
        }
    }

    fillBucketAndProcessQueue();
}

void CoreNetwork::fillBucketAndProcessQueue()
{
    if (_tokenBucket < _burstSize) {
        _tokenBucket++;
    }

    while (!_msgQueue.isEmpty() && _tokenBucket > 0) {
        writeToSocket(_msgQueue.takeFirst());
        if (_metricsServer) {
            _metricsServer->messageQueue(userId(), _msgQueue.size());
        }
    }
}

void CoreNetwork::writeToSocket(const QByteArray& data)
{
    if (_debugLogRawIrc && (_debugLogRawNetId == -1 || networkId().toInt() == _debugLogRawNetId)) {
        qDebug() << "IRC net" << networkId() << ">>" << data;
    }
    socket.write(data);
    socket.write("\r\n");
    if (_metricsServer) {
        _metricsServer->transmitDataNetwork(userId(), data.size() + 2);
    }
    if (!_skipMessageRates) {
        _tokenBucket--;
    }
}

Network::Server CoreNetwork::usedServer() const
{
    if (_lastUsedServerIndex < serverList().size())
        return serverList()[_lastUsedServerIndex];

    if (!serverList().isEmpty())
        return serverList()[0];

    return Network::Server();
}

void CoreNetwork::requestConnect()
{
    qDebug() << "[DEBUG] CoreNetwork::requestConnect called for network" << networkId();
    if (_shuttingDown) {
        qDebug() << "[DEBUG] CoreNetwork::requestConnect - was shutting down, resetting flag";
        _shuttingDown = false;
    }
    if (connectionState() != Disconnected) {
        qWarning() << "Requesting connect while already being connected!";
        return;
    }
    qDebug() << "[DEBUG] CoreNetwork::requestConnect - invoking connectToIrc";
    QMetaObject::invokeMethod(this, "connectToIrc", Qt::QueuedConnection);
}

void CoreNetwork::requestDisconnect()
{
    qDebug() << "[DEBUG] CoreNetwork::requestDisconnect called for network" << networkId();
    if (_shuttingDown) {
        qDebug() << "[DEBUG] CoreNetwork::requestDisconnect - shutting down, aborting";
        return;
    }
    if (connectionState() == Disconnected) {
        qWarning() << "Requesting disconnect while not being connected!";
        return;
    }
    qDebug() << "[DEBUG] CoreNetwork::requestDisconnect - handling quit";
    userInputHandler()->handleQuit(BufferInfo(), QString());
}

void CoreNetwork::requestSetNetworkInfo(const NetworkInfo& info)
{
    Network::Server currentServer = usedServer();
    setNetworkName(info.networkName);
    setIdentity(info.identity);
    setCodecForServer(info.codecForServer);
    setCodecForEncoding(info.codecForEncoding);
    setCodecForDecoding(info.codecForDecoding);
    setServerList(info.serverList);
    setUseRandomServer(info.useRandomServer);
    setPerform(info.perform);
    setSkipCaps(info.skipCaps);
    setUseAutoIdentify(info.useAutoIdentify);
    setAutoIdentifyService(info.autoIdentifyService);
    setAutoIdentifyPassword(info.autoIdentifyPassword);
    setUseSasl(info.useSasl);
    setSaslAccount(info.saslAccount);
    setSaslPassword(info.saslPassword);
    setUseAutoReconnect(info.useAutoReconnect);
    setAutoReconnectInterval(info.autoReconnectInterval);
    setAutoReconnectRetries(info.autoReconnectRetries);
    setUnlimitedReconnectRetries(info.unlimitedReconnectRetries);
    setRejoinChannels(info.rejoinChannels);
    setUseCustomMessageRate(info.useCustomMessageRate);
    setMessageRateBurstSize(info.messageRateBurstSize);
    setMessageRateDelay(info.messageRateDelay);
    setUnlimitedMessageRate(info.unlimitedMessageRate);
    Core::updateNetwork(coreSession()->user(), info);

    _lastUsedServerIndex = 0;
    for (int i = 0; i < serverList().size(); i++) {
        Network::Server server = serverList()[i];
        if (server.host == currentServer.host && server.port == currentServer.port) {
            _lastUsedServerIndex = i;
            break;
        }
    }
}

QList<QList<QByteArray>> CoreNetwork::splitMessage(const QString& cmd,
                                                   const QString& message,
                                                   const std::function<QList<QByteArray>(QString&)>& cmdGenerator)
{
    QString wrkMsg(message);
    QList<QList<QByteArray>> msgsToSend;

    do {
        int splitPos = wrkMsg.size();
        QList<QByteArray> initialSplitMsgEnc = cmdGenerator(wrkMsg);
        int initialOverrun = userInputHandler()->lastParamOverrun(cmd, initialSplitMsgEnc);

        if (initialOverrun) {
            QString splitMsg(wrkMsg);
            QTextBoundaryFinder qtbf(QTextBoundaryFinder::Word, splitMsg);
            qtbf.setPosition(initialSplitMsgEnc[1].size() - initialOverrun);
            QList<QByteArray> splitMsgEnc;
            int overrun = initialOverrun;

            while (overrun) {
                splitPos = qtbf.toPreviousBoundary();

                if (splitPos > 0) {
                    splitMsg = splitMsg.left(splitPos);
                    splitMsgEnc = cmdGenerator(splitMsg);
                    overrun = userInputHandler()->lastParamOverrun(cmd, splitMsgEnc);
                }
                else {
                    if (qtbf.type() == QTextBoundaryFinder::Word) {
                        splitMsg = wrkMsg;
                        splitPos = splitMsg.size();
                        QTextBoundaryFinder graphemeQtbf(QTextBoundaryFinder::Grapheme, splitMsg);
                        graphemeQtbf.setPosition(initialSplitMsgEnc[1].size() - initialOverrun);
                        qtbf = graphemeQtbf;
                    }
                    else {
                        qWarning() << "Unexpected failure to split message!";
                        return msgsToSend;
                    }
                }
            }

            wrkMsg.remove(0, splitPos);
            msgsToSend.append(splitMsgEnc);
        }
        else {
            wrkMsg.remove(0, splitPos);
            msgsToSend.append(initialSplitMsgEnc);
        }
    } while (wrkMsg.size() > 0);

    return msgsToSend;
}

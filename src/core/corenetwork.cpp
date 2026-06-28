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
#include <QTextBoundaryFinder>

#include "core.h"
#include "coreidentity.h"
#include "corenetworkconfig.h"
#include "coresession.h"
#include "coreuserinputhandler.h"
#include "ircencoder.h"
#include "irccap.h"
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
    // Check if raw IRC logging is enabled
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
    connect(&socket, selectOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, &CoreNetwork::onSocketError);
    connect(&socket, &QAbstractSocket::stateChanged, this, &CoreNetwork::onSocketStateChanged);
    connect(&socket, &QIODevice::readyRead, this, &CoreNetwork::onSocketHasData);
    connect(&socket, &QSslSocket::encrypted, this, &CoreNetwork::onSocketInitialized);
    connect(&socket, selectOverload<const QList<QSslError>&>(&QSslSocket::sslErrors), this, &CoreNetwork::onSslErrors);
    connect(this, &CoreNetwork::newEvent, coreSession()->eventManager(), &EventManager::postEvent);

    // Custom rate limiting
    // These react to the user changing settings in the client
    connect(this, &Network::useCustomMessageRateSet, this, &CoreNetwork::updateRateLimiting);
    connect(this, &Network::messageRateBurstSizeSet, this, &CoreNetwork::updateRateLimiting);
    connect(this, &Network::messageRateDelaySet, this, &CoreNetwork::updateRateLimiting);
    connect(this, &Network::unlimitedMessageRateSet, this, &CoreNetwork::updateRateLimiting);

    // IRCv3 capability handling
    // These react to CAP messages from the server
    connect(this, &Network::capAdded, this, &CoreNetwork::serverCapAdded);
    connect(this, &Network::capAcknowledged, this, &CoreNetwork::serverCapAcknowledged);
    connect(this, &Network::capRemoved, this, &CoreNetwork::serverCapRemoved);

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
    // Ensure we don't get any more signals from the socket while shutting down
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
        // Socket already disconnected.
        return true;
    }
    // Request a socket-level disconnect if not already happened
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
    if (_shuttingDown) {
        return;
    }

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

    // cleaning up old quit reason
    _quitReason.clear();

    // Reset capability negotiation tracking, also handling server changes during reconnect
    _capsQueuedIndividual.clear();
    _capsQueuedBundled.clear();
    clearCaps();
    _capNegotiationActive = false;
    _capInitialNegotiationEnded = false;

    // use a random server?
    if (useRandomServer()) {
        _lastUsedServerIndex = qrand() % serverList().size();
    }
    else if (_previousConnectionAttemptFailed) {
        // cycle to next server if previous connection attempt failed
        _previousConnectionAttemptFailed = false;
        showMessage(NetworkInternalMessage(
            Message::Server,
            BufferInfo::StatusBuffer,
            "",
            tr("Connection failed. Cycling to next server...")
        ));
        if (++_lastUsedServerIndex >= serverList().size()) {
            _lastUsedServerIndex = 0;
        }
    }
    else {
        // Start out with the top server in the list
        _lastUsedServerIndex = 0;
    }

    Server server = usedServer();
    displayStatusMsg(tr("Connecting to %1:%2...").arg(server.host).arg(server.port));
    showMessage(NetworkInternalMessage(
        Message::Server,
        BufferInfo::StatusBuffer,
        "",
        tr("Connecting to %1:%2...").arg(server.host).arg(server.port)
    ));

    if (server.useProxy) {
        QNetworkProxy proxy((QNetworkProxy::ProxyType) server.proxyType, server.proxyHost, server.proxyPort, server.proxyUser, server.proxyPass);
        socket.setProxy(proxy);
    }
    else {
        socket.setProxy(QNetworkProxy::NoProxy);
    }

    enablePingTimeout();

    // Reset tracking for valid timestamps in PONG replies
    setPongTimestampValid(false);

    // Qt caches DNS entries for a minute, resulting in round-robin (e.g. for chat.freenode.net) not working if several users
    // connect at a similar time. QHostInfo::fromName(), however, always performs a fresh lookup, overwriting the cache entry.
    if (!server.useProxy) {
        // Avoid hostname lookups when a proxy is specified. The lookups won't use the proxy and may therefore leak the DNS
        // hostname of the server. Qt's DNS cache also isn't used by the proxy so we don't need to refresh the entry.
        QHostInfo::fromName(server.host);
    }
    QHostAddress outgoingIp = coreSession()->outgoingIp();
    if (!outgoingIp.isNull()) {
        qDebug() << "Setting outgoing ip for user " << coreSession()->user()
                 << "to ip" << outgoingIp;
        if (!socket.bind(outgoingIp)) {
            qWarning() << "Binding socket for user "<< coreSession()->user()
                       << "to ip" << outgoingIp
                       << "failed";
            disconnectFromIrc(false, {}, false);
            return;
        }
    }
    if (server.useSsl) {
        CoreIdentity* identity = identityPtr();
        if (identity) {
            socket.setLocalCertificate(identity->sslCert());
            socket.setPrivateKey(identity->sslKey());
        }
        socket.connectToHostEncrypted(server.host, server.port);
    }
    else {
        socket.connectToHost(server.host, server.port);
    }
}

void CoreNetwork::disconnectFromIrc(bool requested, const QString& reason, bool withReconnect)
{
    // Disconnecting from the network, should expect a socket close or error
    _disconnectExpected = true;
    _quitRequested = requested;  // see socketDisconnected();
    if (!withReconnect) {
        _autoReconnectTimer.stop();
        _autoReconnectCount = 0;  // prohibiting auto reconnect
    }
    disablePingTimeout();
    _msgQueue.clear();
    if (_metricsServer) {
        _metricsServer->messageQueue(userId(), 0);
    }

    IrcUser* me_ = me();
    if (me_) {
        QString awayMsg;
        if (me_->isAway())
            awayMsg = me_->awayMessage();
        Core::setAwayMessage(userId(), networkId(), awayMsg);
    }

    if (reason.isEmpty() && identityPtr())
        _quitReason = identityPtr()->quitReason();
    else
        _quitReason = reason;

    showMessage(NetworkInternalMessage(
        Message::Server,
        BufferInfo::StatusBuffer,
        "",
        tr("Disconnecting. (%1)").arg((!requested && !withReconnect) ? tr("Core Shutdown") : _quitReason)
    ));
    if (socket.state() == QAbstractSocket::UnconnectedState) {
        onSocketDisconnected();
    }
    else {
        if (socket.state() == QAbstractSocket::ConnectedState) {
            // If shutting down, prioritize the QUIT command
            userInputHandler()->issueQuit(_quitReason, _shuttingDown);
        }
        else {
            socket.close();
        }
        if (socket.state() != QAbstractSocket::UnconnectedState) {
            // Wait for up to 10 seconds for the socket to close cleanly, then it will be forcefully aborted
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
        // If there's tokens remaining, ...
        // Or rate limits don't apply AND no messages are in queue (to prevent out-of-order), ...
        // Send the message now.
        writeToSocket(s);
    }
    else {
        // Otherwise, queue the message for later
        if (prepend) {
            // Jump to the start, skipping other messages
            _msgQueue.prepend(s);
        }
        else {
            // Add to back, waiting in order
            _msgQueue.append(s);
        }
        if (_metricsServer) {
            _metricsServer->messageQueue(userId(), _msgQueue.size());
        }
    }
}

void CoreNetwork::putCmd(const QString& cmd, const QList<QByteArray>& params, const QByteArray& prefix, const QHash<IrcTagKey, QString>& tags, bool prepend)
{
    putRawLine(IrcEncoder::writeMessage(tags, prefix, cmd, params), prepend);
}

void CoreNetwork::putCmd(const QString& cmd, const QList<QList<QByteArray>>& params, const QByteArray& prefix, const QHash<IrcTagKey, QString>& tags, bool prependAll)
{
    QListIterator<QList<QByteArray>> i(params);
    while (i.hasNext()) {
        QList<QByteArray> msg = i.next();
        putCmd(cmd, msg, prefix, tags, prependAll);
    }
}

void CoreNetwork::setChannelJoined(const QString& channel)
{
    queueAutoWhoOneshot(channel);  // check this new channel first

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
    Network::setMyNick(mynick);
    if (connectionState() == Network::Initializing)
        networkInitialized();
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
        NetworkDataEvent* event = new NetworkDataEvent(EventManager::NetworkIncoming, this, s);
        event->setTimestamp(QDateTime::currentDateTimeUtc());
        emit newEvent(event);
    }
}

void CoreNetwork::onSocketError(QAbstractSocket::SocketError error)
{
    // Ignore socket closed errors if expected
    if (_disconnectExpected && error == QAbstractSocket::RemoteHostClosedError) {
        return;
    }

    _previousConnectionAttemptFailed = true;
    qWarning() << qPrintable(tr("Could not connect to %1 (%2)").arg(networkName(), socket.errorString()));
    emit connectionError(socket.errorString());
    showMessage(NetworkInternalMessage(
        Message::Error,
        BufferInfo::StatusBuffer,
        "",
        tr("Connection failure: %1").arg(socket.errorString())
    ));
    emitConnectionError(socket.errorString());
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

    // Non-SSL connections enter here only once, always emit socketInitialized(...) in these cases
    // SSL connections call socketInitialized() twice, only emit socketInitialized(...) on the first (not yet encrypted) run
    if (!server.useSsl || !socket.isEncrypted()) {
        emit socketInitialized(identity, localAddress(), localPort(), peerAddress(), peerPort(), _socketId);
    }

    if (server.useSsl && !socket.isEncrypted()) {
        // We'll finish setup once we're encrypted, and called again
        return;
    }

    socket.setSocketOption(QAbstractSocket::KeepAliveOption, true);

    // Update the TokenBucket, force-enabling unlimited message rates for initial registration and
    // capability negotiation.  networkInitialized() will call updateRateLimiting() without the
    // force flag to apply user preferences.  When making changes, ensure that this still happens!
    // As Quassel waits for CAP ACK/NAK and AUTHENTICATE replies, this shouldn't ever fill the IRC
    // server receive queue and cause a kill.  "Shouldn't" being the operative word; the real world
    // is a scary place.
    updateRateLimiting(true);
    // Fill up the token bucket as we're connecting from scratch
    resetTokenBucket();

    // Request capabilities as per IRCv3.2 specifications
    // Older servers should ignore this; newer servers won't downgrade to RFC1459
    showMessage(NetworkInternalMessage(
        Message::Server,
        BufferInfo::StatusBuffer,
        "",
        tr("Requesting capability list...")
    ));
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
    // Only allow strict-compliant idents when strict mode is enabled
    putRawLine(serverEncode(QString("USER %1 8 * :%2").arg(coreSession()->strictCompliantIdent(identity), identity->realName())));
}

void CoreNetwork::onSocketDisconnected()
{
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

    IrcUser* me_ = me();
    if (me_) {
        for (const QString& channel : me_->channels()) {
            showMessage(NetworkInternalMessage(
                Message::Quit,
                BufferInfo::ChannelBuffer,
                channel,
                _quitReason, me_->hostmask()
            ));
        }
    }

    setConnected(false);
    emit disconnected(networkId());
    emit socketDisconnected(identityPtr(), localAddress(), localPort(), peerAddress(), peerPort(), _socketId);
    // Reset disconnect expectations
    _disconnectExpected = false;
    if (_quitRequested) {
        _quitRequested = false;
        setConnectionState(Network::Disconnected);
        Core::setNetworkConnected(userId(), networkId(), false);
    }
    else if (_autoReconnectCount != 0) {
        setConnectionState(Network::Reconnecting);
        if (_autoReconnectCount == -1 || _autoReconnectCount == autoReconnectRetries())
            doAutoReconnect();  // first try is immediate
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
    setConnectionState(Network::Initialized);
    setConnected(true);
    _disconnectExpected = false;
    _quitRequested = false;

    // Update the TokenBucket with specified rate-limiting settings, removing the force-unlimited
    // flag used for initial registration and capability negotiation.
    updateRateLimiting();

    if (useAutoReconnect()) {
        // reset counter
        _autoReconnectCount = unlimitedReconnectRetries() ? -1 : autoReconnectRetries();
    }

    // restore away state
    QString awayMsg = Core::awayMessage(userId(), networkId());
    if (!awayMsg.isEmpty()) {
        // Don't re-apply any timestamp formatting in order to preserve escaped percent signs, e.g.
        // '%%%%%%%%' -> '%%%%'  If processed again, it'd result in '%%'.
        userInputHandler()->handleAway(BufferInfo(), awayMsg, true);
    }

    sendPerform();

    _sendPings = true;

    if (networkConfig()->autoWhoEnabled()) {
        _autoWhoCycleTimer.start();
        _autoWhoTimer.start();
        startAutoWhoCycle();  // FIXME wait for autojoin to be completed
    }

    Core::bufferInfo(userId(), networkId(), BufferInfo::StatusBuffer);  // create status buffer
    Core::setNetworkConnected(userId(), networkId(), true);
}

void CoreNetwork::sendPerform()
{
    BufferInfo statusBuf = BufferInfo::fakeStatusBuffer(networkId());

    // do auto identify
    if (useAutoIdentify() && !autoIdentifyService().isEmpty() && !autoIdentifyPassword().isEmpty()) {
        userInputHandler()->handleMsg(statusBuf, QString("%1 IDENTIFY %2").arg(autoIdentifyService(), autoIdentifyPassword()));
    }

    // restore old user modes if server default mode is set.
    IrcUser* me_ = me();
    if (me_) {
        if (!me_->userModes().isEmpty()) {
            restoreUserModes();
        }
        else {
            connect(me_, &IrcUser::userModesSet, this, &CoreNetwork::restoreUserModes);
            connect(me_, &IrcUser::userModesAdded, this, &CoreNetwork::restoreUserModes);
        }
    }

    // send perform list
    for (const QString& line : perform()) {
        if (!line.isEmpty())
            userInput(statusBuf, line);
    }

    // rejoin channels we've been in
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
    IrcUser* me_ = me();
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

    addModes.remove(QRegExp(QString("[%1]").arg(currentModes)));
    if (currentModes.isEmpty())
        removeModes = QString();
    else
        removeModes.remove(QRegExp(QString("[^%1]").arg(currentModes)));

    if (addModes.isEmpty() && removeModes.isEmpty())
        return;

    if (!addModes.isEmpty())
        addModes = '+' + addModes;
    if (!removeModes.isEmpty())
        removeModes = '-' + removeModes;

    // don't use InputHandler::handleMode() as it keeps track of our persistent mode changes
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

    addModes.remove(QRegExp(QString("[%1]").arg(addModesOld)));     // deduplicate
    addModesOld.remove(QRegExp(QString("[%1]").arg(removeModes)));  // update
    addModes += addModesOld;

    removeModes.remove(QRegExp(QString("[%1]").arg(removeModesOld)));  // deduplicate
    removeModesOld.remove(QRegExp(QString("[%1]").arg(addModes)));     // update
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

    // remove modes we didn't issue
    if (requestedAdd.isEmpty())
        addModes = QString();
    else
        addModes.remove(QRegExp(QString("[^%1]").arg(requestedAdd)));

    if (requestedRemove.isEmpty())
        removeModes = QString();
    else
        removeModes.remove(QRegExp(QString("[^%1]").arg(requestedRemove)));

    // deduplicate
    persistentAdd.remove(QRegExp(QString("[%1]").arg(addModes)));
    persistentRemove.remove(QRegExp(QString("[%1]").arg(removeModes)));

    // update
    persistentAdd.remove(QRegExp(QString("[%1]").arg(removeModes)));
    persistentRemove.remove(QRegExp(QString("[%1]").arg(addModes)));

    // update issued mode list
    requestedAdd.remove(QRegExp(QString("[%1]").arg(addModes)));
    requestedRemove.remove(QRegExp(QString("[%1]").arg(removeModes)));
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
        _autoReconnectCount--;  // -2 means we delay the next reconnect
    connectToIrc(true);
}

void CoreNetwork::sendPing()
{
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (_pingCount != 0) {
        qDebug() << "UserId:" << userId() << "Network:" << networkName() << "missed" << _pingCount << "pings."
                 << "BA:" << socket.bytesAvailable() << "BTW:" << socket.bytesToWrite();
    }
    if ((int) _pingCount >= networkConfig()->maxPingCount() && (now - _lastPingTime) <= (_pingTimer.interval() + (1 * 1000))) {
        // In transitioning to 64-bit time, the interval no longer needs converted down to seconds.
        // However, to reduce the risk of breaking things by changing past behavior, we still allow
        // up to 1 second missed instead of enforcing a stricter 1 millisecond allowance.
        //
        // the second check compares the actual elapsed time since the last ping and the pingTimer interval
        // if the interval is shorter then the actual elapsed time it means that this thread was somehow blocked
        // and unable to even handle a ping answer. So we ignore those misses.
        disconnectFromIrc(false,
                          QString("No Ping reply in %1 seconds.").arg(_pingCount * _pingTimer.interval() / 1000),
                          true /* withReconnect */);
    }
    else {
        _lastPingTime = now;
        _pingCount++;
        // Don't send pings until the network is initialized
        if (_sendPings) {
            // Mark as waiting for a reply
            _pongReplyPending = true;
            // Send default timestamp ping
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

/******** Custom Rate Limiting ********/

void CoreNetwork::updateRateLimiting(bool forceUnlimited)
{
    // Verify and apply custom rate limiting options, always resetting the delay and burst size
    // (safe-guarding against accidentally starting the timer), but don't reset the token bucket as
    // this may be called while connected to a server.

    if (useCustomMessageRate() || forceUnlimited) {
        // Custom message rates enabled, or chosen by means of forcing unlimited.  Let's go for it!

        _messageDelay = messageRateDelay();

        _burstSize = messageRateBurstSize();
        if (_burstSize < 1) {
            qWarning() << "Invalid messageRateBurstSize data, cannot have zero message burst size!" << _burstSize;
            // Can't go slower than one message at a time
            _burstSize = 1;
        }

        if (_tokenBucket > _burstSize) {
            // Don't let the token bucket exceed the maximum
            _tokenBucket = _burstSize;
            // To fill up the token bucket, use resetRateLimiting().  Don't do that here, otherwise
            // changing the rate-limit settings while connected to a server will incorrectly reset
            // the token bucket.
        }

        // Toggle the timer according to whether or not rate limiting is enabled
        // If we're here, either useCustomMessageRate or forceUnlimited is true.  Thus, the logic is
        // _skipMessageRates = ((useCustomMessageRate && unlimitedMessageRate) || forceUnlimited)
        // Override user preferences if called with force unlimited, only used during connect.
        _skipMessageRates = (unlimitedMessageRate() || forceUnlimited);
        if (_skipMessageRates) {
            // If the message queue already contains messages, they need sent before disabling the
            // timer.  Set the timer to a rapid pace and let it disable itself.
            if (!_msgQueue.isEmpty()) {
                qDebug() << "Outgoing message queue contains messages while disabling rate "
                            "limiting.  Sending remaining queued messages...";
                // Promptly run the timer again to clear the messages.  Rate limiting is disabled,
                // so nothing should cause this to block.. in theory.  However, don't directly call
                // fillBucketAndProcessQueue() in order to keep it on a separate thread.
                //
                // TODO If testing shows this isn't needed, it can be simplified to a direct call.
                // Hesitant to change it without a wide variety of situations to verify behavior.
                _tokenBucketTimer.start(100);
            }
            else {
                // No rate limiting, disable the timer
                _tokenBucketTimer.stop();
            }
        }
        else {
            // Rate limiting enabled, enable the timer
            _tokenBucketTimer.start(_messageDelay);
        }
    }
    else {
        // Custom message rates disabled.  Go for the default.

        _skipMessageRates = false;  // Enable rate-limiting by default
        _messageDelay = 2200;       // This seems to be a safe value (2.2 seconds delay)
        _burstSize = 5;             // 5 messages at once
        if (_tokenBucket > _burstSize) {
            // TokenBucket to avoid sending too much at once.  Don't let the token bucket exceed the
            // maximum.
            _tokenBucket = _burstSize;
            // To fill up the token bucket, use resetRateLimiting().  Don't do that here, otherwise
            // changing the rate-limit settings while connected to a server will incorrectly reset
            // the token bucket.
        }
        // Rate limiting enabled, enable the timer
        _tokenBucketTimer.start(_messageDelay);
    }
}

void CoreNetwork::resetTokenBucket()
{
    // Fill up the token bucket to the maximum
    _tokenBucket = _burstSize;
}

/******** IRCv3 Capability Negotiation ********/

void CoreNetwork::serverCapAdded(const QString& capability)
{
    // Exclude skipped capabilities
    if (skipCaps().contains(capability)) {
        return;
    }

    // Check if it's a known capability; if so, add it to the list
    // Handle special cases first
    if (capability == IrcCap::SASL) {
        // Only request SASL if it's enabled
        if (useSasl()) {
            queueCap(capability);
        }
    }
    else if (IrcCap::knownCaps.contains(capability)) {
        // Handling for general known capabilities
        queueCap(capability);
    }
}

void CoreNetwork::serverCapAcknowledged(const QString& capability)
{
    // This may be called multiple times in certain situations.

    // Handle core-side configuration
    if (capability == IrcCap::AWAY_NOTIFY) {
        // away-notify enabled, stop the autoWho timers, handle manually
        setAutoWhoEnabled(false);
    }

    // Handle capabilities that require further messages sent to the IRC server
    // If you change this list, ALSO change the list in CoreNetwork::capsRequiringConfiguration
    if (capability == IrcCap::SASL) {
        // If SASL mechanisms specified, limit to what's accepted for authentication
        // if the current identity has a cert set, use SASL EXTERNAL
        // FIXME use event
        if (!identityPtr()->sslCert().isNull()) {
            if (saslMaybeSupports(IrcCap::SaslMech::EXTERNAL)) {
                // EXTERNAL authentication supported, send request
                putRawLine(serverEncode("AUTHENTICATE EXTERNAL"));
            }
            else {
                showMessage(NetworkInternalMessage(
                    Message::Error,
                    BufferInfo::StatusBuffer,
                    "",
                    tr("SASL EXTERNAL authentication not supported")
                ));
                sendNextCap();
            }
        }
        else {
            if (saslMaybeSupports(IrcCap::SaslMech::PLAIN)) {
                // PLAIN authentication supported, send request
                // Only working with PLAIN atm, blowfish later
                putRawLine(serverEncode("AUTHENTICATE PLAIN"));
            }
            else {
                showMessage(NetworkInternalMessage(
                    Message::Error,
                    BufferInfo::StatusBuffer,
                    "",
                    tr("SASL PLAIN authentication not supported")
                ));
                sendNextCap();
            }
        }
    }
}

void CoreNetwork::serverCapRemoved(const QString& capability)
{
    // This may be called multiple times in certain situations.

    // Handle special cases here
    if (capability == IrcCap::AWAY_NOTIFY) {
        // away-notify disabled, enable autoWho according to configuration
        setAutoWhoEnabled(networkConfig()->autoWhoEnabled());
    }
}

void CoreNetwork::queueCap(const QString& capability)
{
    // IRCv3 specs all use lowercase capability names
    QString _capLowercase = capability.toLower();

    if (capsRequiringConfiguration.contains(_capLowercase)) {
        // The capability requires additional configuration before being acknowledged (e.g. SASL),
        // so we should negotiate it separately from all other capabilities.  Otherwise new
        // capabilities will be requested while still configuring the previous one.
        if (!_capsQueuedIndividual.contains(_capLowercase)) {
            _capsQueuedIndividual.append(_capLowercase);
        }
    }
    else {
        // The capability doesn't need any special configuration, so it should be safe to try
        // bundling together with others.  "Should" being the imperative word, as IRC servers can do
        // anything.
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
    showMessage(NetworkInternalMessage(
        Message::Server,
        BufferInfo::StatusBuffer,
        "",
        tr("Could not negotiate some capabilities, retrying individually (%1)...").arg(_capsQueuedLastBundle.join(", "))
    ));
    // Capabilities are already removed from the capability bundle queue via takeQueuedCaps(), no
    // need to remove them here.
    // Clear the most recently tried set to reduce risk that mistakes elsewhere causes retrying
    // indefinitely.
    _capsQueuedLastBundle.clear();
}

void CoreNetwork::beginCapNegotiation()
{
    // Check if any available capabilities have been disabled
    QStringList capsSkipped;
    if (!skipCaps().isEmpty() && !caps().isEmpty()) {
        // Find the entries that are common to skipCaps() and caps().  This represents any
        // capabilities supported by the server that were skipped.

        // Both skipCaps() and caps() are already lowercase
        // std::set_intersection requires sorted lists, and we can't modify the original lists.
        //
        // skipCaps() should already be sorted.  caps() is intentionally not sorted elsewhere so
        // Quassel can show the capabilities in the order transmitted by the network.
        auto sortedCaps = caps();
        sortedCaps.sort();

        // Find the intersection between skipped caps and server-supplied caps
        std::set_intersection(skipCaps().cbegin(), skipCaps().cend(),
                              sortedCaps.cbegin(), sortedCaps.cend(),
                              std::back_inserter(capsSkipped));
    }

    if (!capsPendingNegotiation()) {
        // No capabilities are queued for request, determine the reason why
        QString capStatusMsg;
        if (caps().empty()) {
            // The server doesn't provide any capabilities, but supports CAP LS
            capStatusMsg = tr("No capabilities available");
        }
        else if (capsEnabled().empty()) {
            // The server supports capabilities (caps() is not empty) but Quassel doesn't support
            // anything offered.  This should be uncommon.
            capStatusMsg =
                    tr("None of the capabilities provided by the server are supported (found: %1)")
                    .arg(caps().join(", "));
        }
        else {
            // Quassel has enabled some capabilities, but there are no further capabilities that can
            // be negotiated.
            // (E.g. the user has manually run "/cap ls 302" after initial negotiation.)
            capStatusMsg =
                    tr("No additional capabilities are supported (found: %1; currently enabled: %2)")
                    .arg(caps().join(", "), capsEnabled().join(", "));
        }
        // Inform the user of the situation
        showMessage(NetworkInternalMessage(
            Message::Server,
            BufferInfo::StatusBuffer,
            "",
            capStatusMsg
        ));

        if (!capsSkipped.isEmpty()) {
            // Mention that some capabilities are skipped
            showMessage(NetworkInternalMessage(
                Message::Server,
                BufferInfo::StatusBuffer,
                "",
                tr("Quassel is configured to ignore some capabilities (skipped: %1)").arg(capsSkipped.join(", "))
            ));
        }

        // End any ongoing capability negotiation, allowing connection to continue
        endCapNegotiation();
        return;
    }

    _capNegotiationActive = true;
    showMessage(NetworkInternalMessage(
        Message::Server,
        BufferInfo::StatusBuffer,
        "",
        tr("Ready to negotiate (found: %1)").arg(caps().join(", "))
    ));

    if (!capsSkipped.isEmpty()) {
        // Mention that some capabilities are skipped
        showMessage(NetworkInternalMessage(
            Message::Server,
            BufferInfo::StatusBuffer,
            "",
            tr("Quassel is configured to ignore some capabilities (skipped: %1)").arg(capsSkipped.join(", "))
        ));
    }

    // Build a list of queued capabilities, starting with individual, then bundled, only adding the
    // comma separator between the two if needed (both individual and bundled caps exist).
    QString queuedCapsDisplay = _capsQueuedIndividual.join(", ")
                                + ((!_capsQueuedIndividual.empty() && !_capsQueuedBundled.empty()) ? ", " : "")
                                + _capsQueuedBundled.join(", ");
    showMessage(NetworkInternalMessage(
        Message::Server,
        BufferInfo::StatusBuffer,
        "",
        tr("Negotiating capabilities (requesting: %1)...").arg(queuedCapsDisplay)
    ));

    sendNextCap();
}

void CoreNetwork::sendNextCap()
{
    if (capsPendingNegotiation()) {
        // Request the next set of capabilities and remove them from the list
        putRawLine(serverEncode(QString("CAP REQ :%1").arg(takeQueuedCaps())));
    }
    else {
        // No pending desired capabilities, capability negotiation finished
        // If SASL requested but not available, print a warning
        if (useSasl() && !capEnabled(IrcCap::SASL))
            showMessage(NetworkInternalMessage(
                Message::Error,
                BufferInfo::StatusBuffer,
                "",
                tr("SASL authentication currently not supported by server")
            ));

        if (_capNegotiationActive) {
            showMessage(NetworkInternalMessage(
                Message::Server,
                BufferInfo::StatusBuffer,
                "",
                tr("Capability negotiation finished (enabled: %1)").arg(capsEnabled().join(", "))
            ));
            _capNegotiationActive = false;
        }

        endCapNegotiation();
    }
}

void CoreNetwork::endCapNegotiation()
{
    // If nick registration is already complete, CAP END is not required
    if (!_capInitialNegotiationEnded) {
        putRawLine(serverEncode(QString("CAP END")));
        _capInitialNegotiationEnded = true;
    }
}

/******** AutoWHO ********/

void CoreNetwork::startAutoWhoCycle()
{
    if (!_autoWhoQueue.isEmpty()) {
        _autoWhoCycleTimer.stop();
        return;
    }
    _autoWhoQueue = channels();
}

void CoreNetwork::queueAutoWhoOneshot(const QString& name)
{
    // Prepend so these new channels/nicks are the first to be checked
    // Don't allow duplicates
    if (!_autoWhoQueue.contains(name.toLower())) {
        _autoWhoQueue.prepend(name.toLower());
    }
    if (capEnabled(IrcCap::AWAY_NOTIFY)) {
        // When away-notify is active, the timer's stopped.  Start a new cycle to who this channel.
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
    // Don't send autowho if there are still some pending
    if (_autoWhoPending.count())
        return;

    while (!_autoWhoQueue.isEmpty()) {
        QString chanOrNick = _autoWhoQueue.takeFirst();
        // Check if it's a known channel or nick
        IrcChannel* ircchan = ircChannel(chanOrNick);
        IrcUser* ircuser = ircUser(chanOrNick);
        if (ircchan) {
            // Apply channel limiting rules
            // If using away-notify, don't impose channel size limits in order to capture away
            // state of everyone.  Auto-who won't run on a timer so network impact is minimal.
            if (networkConfig()->autoWhoNickLimit() > 0 && ircchan->ircUsers().count() >= networkConfig()->autoWhoNickLimit()
                && !capEnabled(IrcCap::AWAY_NOTIFY))
                continue;
            _autoWhoPending[chanOrNick.toLower()]++;
        }
        else if (ircuser) {
            // Checking a nick, add it to the pending list
            _autoWhoPending[ircuser->nick().toLower()]++;
        }
        else {
            // Not a channel or a nick, skip it
            qDebug() << "Skipping who polling of unknown channel or nick" << chanOrNick;
            continue;
        }
        if (supports("WHOX")) {
            // Use WHO extended to poll away users and/or user accounts
            // Explicitly only match on nickname ("n"), don't rely on server defaults
            //
            // WHO <nickname> n%chtsunfra,<unique_number>
            //
            // See http://faerion.sourceforge.net/doc/irc/whox.var
            // And https://github.com/quakenet/snircd/blob/master/doc/readme.who
            // And https://github.com/hexchat/hexchat/blob/57478b65758e6b697b1d82ce21075e74aa475efc/src/common/proto-irc.c#L752
            putRawLine(serverEncode(
                QString("WHO %1 n%chtsunfra,%2")
                    .arg(chanOrNick, QString::number(IrcCap::ACCOUNT_NOTIFY_WHOX_NUM))
            ));
        }
        else {
            // Fall back to normal WHO
            //
            // Note: According to RFC 1459, "WHO <phrase>" can fall back to searching realname,
            // hostmask, etc.  There's nothing we can do about that :(
            //
            // See https://tools.ietf.org/html/rfc1459#section-4.5.1
            putRawLine(serverEncode(QString("WHO %1").arg(chanOrNick)));
        }
        break;
    }

    if (_autoWhoQueue.isEmpty() && networkConfig()->autoWhoEnabled() && !_autoWhoCycleTimer.isActive() && !capEnabled(IrcCap::AWAY_NOTIFY)) {
        // Timer was stopped, means a new cycle is due immediately
        // Don't run a new cycle if using away-notify; server will notify as appropriate
        _autoWhoCycleTimer.start();
        startAutoWhoCycle();
    }
    else if (capEnabled(IrcCap::AWAY_NOTIFY) && _autoWhoCycleTimer.isActive()) {
        // Don't run another who cycle if away-notify is enabled
        _autoWhoCycleTimer.stop();
    }
}

void CoreNetwork::onSslErrors(const QList<QSslError>& sslErrors)
{
    Server server = usedServer();
    if (server.sslVerify) {
        // Treat the SSL error as a hard error
        QString sslErrorMessage = tr("Encrypted connection couldn't be verified, disconnecting "
                                     "since verification is required");
        if (!sslErrors.empty()) {
            // Add the error reason if known
            sslErrorMessage.append(tr(" (Reason: %1)").arg(sslErrors.first().errorString()));
        }
        showMessage(NetworkInternalMessage(
            Message::Error,
            BufferInfo::StatusBuffer,
            "",
            sslErrorMessage
        ));

        // Disconnect, triggering a reconnect in case it's a temporary issue with certificate
        // validity, network trouble, etc.
        disconnectFromIrc(false, QString("Encrypted connection not verified"), true /* withReconnect */);
    }
    else {
        // Treat the SSL error as a warning, continue to connect anyways
        QString sslErrorMessage = tr("Encrypted connection couldn't be verified, continuing "
                                     "since verification is not required");
        if (!sslErrors.empty()) {
            // Add the error reason if known
            sslErrorMessage.append(tr(" (Reason: %1)").arg(sslErrors.first().errorString()));
        }
        showMessage(NetworkInternalMessage(
            Message::Info,
            BufferInfo::StatusBuffer,
            "",
            sslErrorMessage
        ));

        // Proceed with the connection
        socket.ignoreSslErrors();
    }
}

void CoreNetwork::checkTokenBucket()
{
    if (_skipMessageRates) {
        if (_msgQueue.empty()) {
            // Message queue emptied; stop the timer and bail out
            _tokenBucketTimer.stop();
            return;
        }
        // Otherwise, we're emptying the queue, continue on as normal
    }

    // Process whatever messages are pending
    fillBucketAndProcessQueue();
}

void CoreNetwork::fillBucketAndProcessQueue()
{
    // If there's less tokens than burst size, refill the token bucket by 1
    if (_tokenBucket < _burstSize) {
        _tokenBucket++;
    }

    // As long as there's tokens available and messages remaining, sending messages from the queue
    while (!_msgQueue.empty() && _tokenBucket > 0) {
        writeToSocket(_msgQueue.takeFirst());
        if (_metricsServer) {
            _metricsServer->messageQueue(userId(), _msgQueue.size());
        }
    }
}

void CoreNetwork::writeToSocket(const QByteArray& data)
{
    // Log the message if enabled and network ID matches or allows all
    if (_debugLogRawIrc && (_debugLogRawNetId == -1 || networkId().toInt() == _debugLogRawNetId)) {
        // Include network ID
        qDebug() << "IRC net" << networkId() << ">>" << data;
    }
    socket.write(data);
    socket.write("\r\n");
    if (_metricsServer) {
        _metricsServer->transmitDataNetwork(userId(), data.size() + 2);
    }
    if (!_skipMessageRates) {
        // Only subtract from the token bucket if message rate limiting is enabled
        _tokenBucket--;
    }
}

Network::Server CoreNetwork::usedServer() const
{
    if (_lastUsedServerIndex < serverList().count())
        return serverList()[_lastUsedServerIndex];

    if (!serverList().isEmpty())
        return serverList()[0];

    return Network::Server();
}

void CoreNetwork::requestConnect() const
{
    if (_shuttingDown) {
        return;
    }
    if (connectionState() != Disconnected) {
        qWarning() << "Requesting connect while already being connected!";
        return;
    }
    QMetaObject::invokeMethod(const_cast<CoreNetwork*>(this), "connectToIrc", Qt::QueuedConnection);
}

void CoreNetwork::requestDisconnect() const
{
    if (_shuttingDown) {
        return;
    }
    if (connectionState() == Disconnected) {
        qWarning() << "Requesting disconnect while not being connected!";
        return;
    }
    userInputHandler()->handleQuit(BufferInfo(), QString());
}

void CoreNetwork::requestSetNetworkInfo(const NetworkInfo& info)
{
    Network::Server currentServer = usedServer();
    setNetworkInfo(info);
    Core::updateNetwork(coreSession()->user(), info);

    // the order of the servers might have changed,
    // so we try to find the previously used server
    _lastUsedServerIndex = 0;
    for (int i = 0; i < serverList().count(); i++) {
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

    // do while (wrkMsg.size() > 0)
    do {
        // First, check to see if the whole message can be sent at once.  The
        // cmdGenerator function is passed in by the caller and is used to encode
        // and encrypt (if applicable) the message, since different callers might
        // want to use different encoding or encode different values.
        int splitPos = wrkMsg.size();
        QList<QByteArray> initialSplitMsgEnc = cmdGenerator(wrkMsg);
        int initialOverrun = userInputHandler()->lastParamOverrun(cmd, initialSplitMsgEnc);

        if (initialOverrun) {
            // If the message was too long to be sent, first try splitting it along
            // word boundaries with QTextBoundaryFinder.
            QString splitMsg(wrkMsg);
            QTextBoundaryFinder qtbf(QTextBoundaryFinder::Word, splitMsg);
            qtbf.setPosition(initialSplitMsgEnc[1].size() - initialOverrun);
            QList<QByteArray> splitMsgEnc;
            int overrun = initialOverrun;

            while (overrun) {
                splitPos = qtbf.toPreviousBoundary();

                // splitPos==-1 means the QTBF couldn't find a split point at all and
                // splitPos==0 means the QTBF could only find a boundary at the beginning of
                // the string.  Neither one of these works for us.
                if (splitPos > 0) {
                    // If a split point could be found, split the message there, calculate the
                    // overrun, and continue with the loop.
                    splitMsg = splitMsg.left(splitPos);
                    splitMsgEnc = cmdGenerator(splitMsg);
                    overrun = userInputHandler()->lastParamOverrun(cmd, splitMsgEnc);
                }
                else {
                    // If a split point could not be found (the beginning of the message
                    // is reached without finding a split point short enough to send) and we
                    // are still in Word mode, switch to Grapheme mode.  We also need to restore
                    // the full wrkMsg to splitMsg, since splitMsg may have been cut down during
                    // the previous attempt to find a split point.
                    if (qtbf.type() == QTextBoundaryFinder::Word) {
                        splitMsg = wrkMsg;
                        splitPos = splitMsg.size();
                        QTextBoundaryFinder graphemeQtbf(QTextBoundaryFinder::Grapheme, splitMsg);
                        graphemeQtbf.setPosition(initialSplitMsgEnc[1].size() - initialOverrun);
                        qtbf = graphemeQtbf;
                    }
                    else {
                        // If the QTBF fails to find a split point in Grapheme mode, we give up.
                        // This should never happen, but it should be handled anyway.
                        qWarning() << "Unexpected failure to split message!";
                        return msgsToSend;
                    }
                }
            }

            // Once a message of sendable length has been found, remove it from the wrkMsg and
            // add it to the list of messages to be sent.
            wrkMsg.remove(0, splitPos);
            msgsToSend.append(splitMsgEnc);
        }
        else {
            // If the entire remaining message is short enough to be sent all at once, remove
            // it from the wrkMsg and add it to the list of messages to be sent.
            wrkMsg.remove(0, splitPos);
            msgsToSend.append(initialSplitMsgEnc);
        }
    } while (wrkMsg.size() > 0);

    return msgsToSend;
}

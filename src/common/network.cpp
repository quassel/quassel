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

#include "network.h"

#include <QDebug>
#include <QStringList>

#include "util.h"

// ====================
//  Static Members
// ====================
std::optional<QStringConverter> Network::_defaultCodecForServer;
std::optional<QStringConverter> Network::_defaultCodecForEncoding;
std::optional<QStringConverter> Network::_defaultCodecForDecoding;

// ====================
//  Public:
// ====================
Network::Network(const NetworkId& networkId, QObject* parent)
    : SyncableObject(parent)
    , _networkId(networkId)
{
    setObjectName(QString::number(networkId.toInt()));

    _channelPrefixes << "#" << "&" << "!" << "+";
    _statusMsg = "@+";
}

Network::~Network()
{
    removeChansAndUsers();
}

bool Network::Server::operator==(const Server& other) const
{
    return (host == other.host && port == other.port && password == other.password && useSsl == other.useSsl && sslVerify == other.sslVerify
            && sslVersion == other.sslVersion && useProxy == other.useProxy && proxyType == other.proxyType && proxyHost == other.proxyHost
            && proxyPort == other.proxyPort && proxyUser == other.proxyUser && proxyPass == other.proxyPass);
}

bool Network::Server::operator!=(const Server& other) const
{
    return !(*this == other);
}

bool Network::isChannelName(const QString& channelname) const
{
    return ::isChannelName(channelname);
}

bool Network::isStatusMsg(const QString& target) const
{
    QString prefixes = statusMsg();
    if (prefixes.isEmpty())
        prefixes = "@+";
    return prefixes.contains(target[0]);
}

QString Network::prefixToMode(const QString& prefix) const
{
    int index = _prefixes.indexOf(prefix);
    if (index == -1)
        return QString();
    return _prefixModes[index];
}

QString Network::modeToPrefix(const QString& mode) const
{
    int index = _prefixModes.indexOf(mode);
    if (index == -1)
        return QString();
    return _prefixes[index];
}

QString Network::sortPrefixModes(const QString& modes) const
{
    // If we don't have prefix modes, we can't sort anything
    if (_prefixModes.isEmpty())
        return modes;

    QString sortedModes;
    // Look through known prefix modes in order, to maintain priority
    for (const QString& mode : _prefixModes) {
        if (modes.contains(mode))
            sortedModes += mode;
    }

    // Now append anything we didn't know about
    for (const QChar& c : modes) {
        if (!_prefixModes.contains(c))
            sortedModes += c;
    }

    return sortedModes;
}

Network::ChannelModeType Network::channelModeType(const QString& mode)
{
    // We have nothing to go on
    if (_supports.isEmpty())
        return NOT_A_CHANMODE;

    QString chanmodes = _supports.value("CHANMODES", "beI,kfL,lj,psmntirRcOAQKVCuzNSMTG");
    QStringList chanmodesList = chanmodes.split(',');

    if (chanmodesList.count() < 4) {
        qWarning() << "Received malformed CHANMODES for" << networkName() << ":" << chanmodes;
        return NOT_A_CHANMODE;
    }

    if (chanmodesList[0].contains(mode))
        return A_CHANMODE;
    if (chanmodesList[1].contains(mode))
        return B_CHANMODE;
    if (chanmodesList[2].contains(mode))
        return C_CHANMODE;
    if (chanmodesList[3].contains(mode))
        return D_CHANMODE;

    return NOT_A_CHANMODE;
}

IrcUser* Network::newIrcUser(const QString& hostmask, const QVariantMap& initData)
{
    if (_ircUsers.contains(hostmask.toLower())) {
        return _ircUsers.value(hostmask.toLower());
    }

    IrcUser* ircUser = new IrcUser(hostmask, this);
    if (!initData.isEmpty())
        ircUser->fromVariantMap(initData);

    _ircUsers[ircUser->nick().toLower()] = ircUser;
    connect(ircUser, &IrcUser::nickSet, this, &Network::ircUserNickSet);
    connect(ircUser, &QObject::destroyed, this, &Network::ircUserDestroyed);
    SYNC_OTHER(newIrcUser, ARG(hostmask), ARG(initData))
    emit newIrcUserSynced(ircUser);
    return ircUser;
}

IrcChannel* Network::newIrcChannel(const QString& channelname, const QVariantMap& initData)
{
    if (_ircChannels.contains(channelname.toLower())) {
        return _ircChannels.value(channelname.toLower());
    }

    IrcChannel* ircChannel = new IrcChannel(channelname, this);
    if (!initData.isEmpty())
        ircChannel->fromVariantMap(initData);

    _ircChannels[channelname.toLower()] = ircChannel;
    return ircChannel;
}

void Network::removeIrcUser(IrcUser* ircuser)
{
    if (!_ircUsers.contains(ircuser->nick().toLower()))
        return;

    _ircUsers.remove(ircuser->nick().toLower());
    emit ircUserRemoved(ircuser);
}

void Network::removeIrcChannel(IrcChannel* ircChannel)
{
    if (!_ircChannels.contains(ircChannel->name().toLower()))
        return;

    _ircChannels.remove(ircChannel->name().toLower());
    emit ircChannelRemoved(ircChannel);
}

void Network::removeChansAndUsers()
{
    while (!_ircUsers.isEmpty()) {
        delete _ircUsers.begin().value();
    }
    while (!_ircChannels.isEmpty()) {
        delete _ircChannels.begin().value();
    }
}

IrcUser* Network::ircUser(const QString& nickname) const
{
    return _ircUsers.value(nickname.toLower(), nullptr);
}

IrcChannel* Network::ircChannel(const QString& channelname) const
{
    return _ircChannels.value(channelname.toLower(), nullptr);
}

QVariantMap Network::initIrcUsersAndChannels() const
{
    QVariantMap usersAndChannels;
    QVariantMap users;
    QVariantMap channels;

    QHash<QString, IrcUser*>::const_iterator userIter = _ircUsers.constBegin();
    while (userIter != _ircUsers.constEnd()) {
        users[userIter.key()] = userIter.value()->toVariantMap();
        ++userIter;
    }

    QHash<QString, IrcChannel*>::const_iterator channelIter = _ircChannels.constBegin();
    while (channelIter != _ircChannels.constEnd()) {
        channels[channelIter.key()] = channelIter.value()->toVariantMap();
        ++channelIter;
    }

    usersAndChannels["IrcUsers"] = users;
    usersAndChannels["IrcChannels"] = channels;
    return usersAndChannels;
}

void Network::initSetIrcUsersAndChannels(const QVariantMap& usersAndChannels)
{
    QVariantMap users = usersAndChannels.value("IrcUsers").toMap();
    QVariantMap channels = usersAndChannels.value("IrcChannels").toMap();

    QVariantMap::const_iterator userIter = users.constBegin();
    while (userIter != users.constEnd()) {
        newIrcUser(userIter.key(), userIter.value().toMap());
        ++userIter;
    }

    QVariantMap::const_iterator channelIter = channels.constBegin();
    while (channelIter != channels.constEnd()) {
        newIrcChannel(channelIter.key(), channelIter.value().toMap());
        ++channelIter;
    }
}

// codec stuff
QByteArray Network::codecForServer() const
{
    if (_codecForServer)
        return _codecForServer->name();
    return QByteArray();
}

QByteArray Network::codecForEncoding() const
{
    if (_codecForEncoding)
        return _codecForEncoding->name();
    return QByteArray();
}

QByteArray Network::codecForDecoding() const
{
    if (_codecForDecoding)
        return _codecForDecoding->name();
    return QByteArray();
}

QByteArray Network::defaultCodecForServer()
{
    if (_defaultCodecForServer)
        return _defaultCodecForServer->name();
    return QByteArray();
}

QByteArray Network::defaultCodecForEncoding()
{
    if (_defaultCodecForEncoding)
        return _defaultCodecForEncoding->name();
    return QByteArray();
}

QByteArray Network::defaultCodecForDecoding()
{
    if (_defaultCodecForDecoding)
        return _defaultCodecForDecoding->name();
    return QByteArray();
}

void Network::setCodecForServer(const QByteArray& codecName)
{
    _codecForServer = QStringConverter::encodingForName(codecName);
    QByteArray name = codecForServer();
    SYNC_OTHER(setCodecForServer, ARG(name))
    emit configChanged();
}

void Network::setCodecForServer(std::optional<QStringConverter> codec)
{
    _codecForServer = codec;
    QByteArray codecName = codecForServer();
    SYNC_OTHER(setCodecForServer, ARG(codecName))
    emit configChanged();
}

void Network::setCodecForEncoding(const QByteArray& codecName)
{
    _codecForEncoding = QStringConverter::encodingForName(codecName);
    QByteArray name = codecForEncoding();
    SYNC_OTHER(setCodecForEncoding, ARG(name))
    emit configChanged();
}

void Network::setCodecForEncoding(std::optional<QStringConverter> codec)
{
    _codecForEncoding = codec;
    QByteArray codecName = codecForEncoding();
    SYNC_OTHER(setCodecForEncoding, ARG(codecName))
    emit configChanged();
}

void Network::setCodecForDecoding(const QByteArray& codecName)
{
    _codecForDecoding = QStringConverter::encodingForName(codecName);
    QByteArray name = codecForDecoding();
    SYNC_OTHER(setCodecForDecoding, ARG(name))
    emit configChanged();
}

void Network::setCodecForDecoding(std::optional<QStringConverter> codec)
{
    _codecForDecoding = codec;
    QByteArray codecName = codecForDecoding();
    SYNC_OTHER(setCodecForDecoding, ARG(codecName))
    emit configChanged();
}

void Network::setDefaultCodecForServer(const QByteArray& name)
{
    _defaultCodecForServer = QStringConverter::encodingForName(name);
}

void Network::setDefaultCodecForEncoding(const QByteArray& name)
{
    _defaultCodecForEncoding = QStringConverter::encodingForName(name);
}

void Network::setDefaultCodecForDecoding(const QByteArray& name)
{
    _defaultCodecForDecoding = QStringConverter::encodingForName(name);
}

QByteArray Network::encodeString(const QString& string) const
{
    if (_codecForEncoding) {
        QStringEncoder encoder(*_codecForEncoding);
        return encoder.encode(string);
    }
    if (_defaultCodecForEncoding) {
        QStringEncoder encoder(*_defaultCodecForEncoding);
        return encoder.encode(string);
    }
    return string.toLatin1();
}

QString Network::decodeString(const QByteArray& text) const
{
    if (_codecForDecoding) {
        QStringDecoder decoder(*_codecForDecoding);
        return decoder.decode(text);
    }
    if (_defaultCodecForDecoding) {
        QStringDecoder decoder(*_defaultCodecForDecoding);
        return decoder.decode(text);
    }
    return ::decodeString(text, _defaultCodecForDecoding);
}

QByteArray Network::encodeServerString(const QString& string) const
{
    if (_codecForServer) {
        QStringEncoder encoder(*_codecForServer);
        return encoder.encode(string);
    }
    if (_defaultCodecForServer) {
        QStringEncoder encoder(*_defaultCodecForServer);
        return encoder.encode(string);
    }
    return string.toLatin1();
}

QString Network::decodeServerString(const QByteArray& text) const
{
    if (_codecForServer) {
        QStringDecoder decoder(*_codecForServer);
        return decoder.decode(text);
    }
    if (_defaultCodecForServer) {
        QStringDecoder decoder(*_defaultCodecForServer);
        return decoder.decode(text);
    }
    return ::decodeString(text, _defaultCodecForServer);
}

// ====================
//  Public Slots:
// ====================
void Network::setNetworkName(const QString& networkName)
{
    if (_networkName != networkName) {
        _networkName = networkName;
        SYNC(ARG(networkName))
        emit configChanged();
    }
}

void Network::setCurrentServer(const QString& currentServer)
{
    if (_currentServer != currentServer) {
        _currentServer = currentServer;
        SYNC(ARG(currentServer))
        emit configChanged();
    }
}

void Network::setMyNick(const QString& myNick)
{
    if (_myNick != myNick) {
        _myNick = myNick;
        SYNC(ARG(myNick))
        emit myNickSet(myNick);
    }
}

void Network::setLatency(int latency)
{
    if (_latency != latency) {
        _latency = latency;
        SYNC(ARG(latency))
    }
}

void Network::setIdentity(IdentityId identity)
{
    if (_identity != identity) {
        _identity = identity;
        SYNC(ARG(identity))
        emit configChanged();
    }
}

void Network::setConnected(bool connected)
{
    if (_connected != connected) {
        _connected = connected;
        if (connected)
            emit connected();
        else
            emit disconnected();
        SYNC(ARG(connected))
    }
}

void Network::setConnectionState(int state)
{
    if (_connectionState != state) {
        _connectionState = state;
        SYNC(ARG(state))
        emit connectionStateSet(static_cast<ConnectionState>(state));
    }
}

void Network::setServerList(const Network::ServerList& serverList)
{
    if (_serverList != serverList) {
        _serverList = serverList;
        SYNC(ARG(serverList))
        emit configChanged();
    }
}

void Network::setUseRandomServer(bool useRandomServer)
{
    if (_useRandomServer != useRandomServer) {
        _useRandomServer = useRandomServer;
        SYNC(ARG(useRandomServer))
        emit configChanged();
    }
}

void Network::setPerform(const QStringList& perform)
{
    if (_perform != perform) {
        _perform = perform;
        SYNC(ARG(perform))
        emit configChanged();
    }
}

void Network::setSkipCaps(const QStringList& skipCaps)
{
    if (_skipCaps != skipCaps) {
        _skipCaps = skipCaps;
        SYNC(ARG(skipCaps))
        emit configChanged();
    }
}

void Network::setUseAutoIdentify(bool useAutoIdentify)
{
    if (_useAutoIdentify != useAutoIdentify) {
        _useAutoIdentify = useAutoIdentify;
        SYNC(ARG(useAutoIdentify))
        emit configChanged();
    }
}

void Network::setAutoIdentifyService(const QString& autoIdentifyService)
{
    if (_autoIdentifyService != autoIdentifyService) {
        _autoIdentifyService = autoIdentifyService;
        SYNC(ARG(autoIdentifyService))
        emit configChanged();
    }
}

void Network::setAutoIdentifyPassword(const QString& autoIdentifyPassword)
{
    if (_autoIdentifyPassword != autoIdentifyPassword) {
        _autoIdentifyPassword = autoIdentifyPassword;
        SYNC(ARG(autoIdentifyPassword))
        emit configChanged();
    }
}

void Network::setUseSasl(bool useSasl)
{
    if (_useSasl != useSasl) {
        _useSasl = useSasl;
        SYNC(ARG(useSasl))
        emit configChanged();
    }
}

void Network::setSaslAccount(const QString& saslAccount)
{
    if (_saslAccount != saslAccount) {
        _saslAccount = saslAccount;
        SYNC(ARG(saslAccount))
        emit configChanged();
    }
}

void Network::setSaslPassword(const QString& saslPassword)
{
    if (_saslPassword != saslPassword) {
        _saslPassword = saslPassword;
        SYNC(ARG(saslPassword))
        emit configChanged();
    }
}

void Network::setUseAutoReconnect(bool useAutoReconnect)
{
    if (_useAutoReconnect != useAutoReconnect) {
        _useAutoReconnect = useAutoReconnect;
        SYNC(ARG(useAutoReconnect))
        emit configChanged();
    }
}

void Network::setAutoReconnectInterval(quint32 autoReconnectInterval)
{
    if (_autoReconnectInterval != autoReconnectInterval) {
        _autoReconnectInterval = autoReconnectInterval;
        SYNC(ARG(autoReconnectInterval))
        emit configChanged();
    }
}

void Network::setAutoReconnectRetries(quint16 autoReconnectRetries)
{
    if (_autoReconnectRetries != autoReconnectRetries) {
        _autoReconnectRetries = autoReconnectRetries;
        SYNC(ARG(autoReconnectRetries))
        emit configChanged();
    }
}

void Network::setUnlimitedReconnectRetries(bool unlimitedReconnectRetries)
{
    if (_unlimitedReconnectRetries != unlimitedReconnectRetries) {
        _unlimitedReconnectRetries = unlimitedReconnectRetries;
        SYNC(ARG(unlimitedReconnectRetries))
        emit configChanged();
    }
}

void Network::setRejoinChannels(bool rejoinChannels)
{
    if (_rejoinChannels != rejoinChannels) {
        _rejoinChannels = rejoinChannels;
        SYNC(ARG(rejoinChannels))
        emit configChanged();
    }
}

void Network::setUseCustomMessageRate(bool useCustomMessageRate)
{
    if (_useCustomMessageRate != useCustomMessageRate) {
        _useCustomMessageRate = useCustomMessageRate;
        SYNC(ARG(useCustomMessageRate))
        emit configChanged();
    }
}

void Network::setMessageRateBurstSize(quint32 messageRateBurstSize)
{
    if (_messageRateBurstSize != messageRateBurstSize) {
        _messageRateBurstSize = messageRateBurstSize;
        SYNC(ARG(messageRateBurstSize))
        emit configChanged();
    }
}

void Network::setMessageRateDelay(quint32 messageRateDelay)
{
    if (_messageRateDelay != messageRateDelay) {
        _messageRateDelay = messageRateDelay;
        SYNC(ARG(messageRateDelay))
        emit configChanged();
    }
}

void Network::setUnlimitedMessageRate(bool unlimitedMessageRate)
{
    if (_unlimitedMessageRate != unlimitedMessageRate) {
        _unlimitedMessageRate = unlimitedMessageRate;
        SYNC(ARG(unlimitedMessageRate))
        emit configChanged();
    }
}

void Network::setChannelPrefixes(const QString& prefixes)
{
    if (_channelPrefixes.join("") != prefixes) {
        _channelPrefixes = prefixes.split("", Qt::SkipEmptyParts);
        SYNC(ARG(prefixes))
    }
}

void Network::setPrefixModes(const QString& modes)
{
    if (_prefixModes.join("") != modes) {
        _prefixModes = modes.split("", Qt::SkipEmptyParts);
        SYNC(ARG(modes))
    }
}

void Network::setPrefixes(const QString& modes, const QString& prefixes)
{
    if (_prefixModes.join("") != modes || _prefixes.join("") != prefixes) {
        _prefixModes = modes.split("", Qt::SkipEmptyParts);
        _prefixes = prefixes.split("", Qt::SkipEmptyParts);
        SYNC_OTHER(setPrefixes, ARG(modes), ARG(prefixes))
    }
}

void Network::setStatusMsg(const QString& statusMsg)
{
    if (_statusMsg != statusMsg) {
        _statusMsg = statusMsg;
        SYNC(ARG(statusMsg))
    }
}

void Network::addSupport(const QString& param, const QString& value)
{
    QMutexLocker locker(&_capsMutex);
    if (_supports.value(param) != value) {
        _supports[param] = value;
        SYNC_OTHER(addSupport, ARG(param), ARG(value))
    }
}

void Network::removeSupport(const QString& param)
{
    QMutexLocker locker(&_capsMutex);
    if (_supports.contains(param)) {
        _supports.remove(param);
        SYNC_OTHER(removeSupport, ARG(param))
    }
}

void Network::setCapNegotiationStatus(const QString& status)
{
    QMutexLocker locker(&_capsMutex);
    if (_capNegotiationStatus != status) {
        _capNegotiationStatus = status;
        SYNC(ARG(status))
    }
}

void Network::addCap(const QString& capability)
{
    QMutexLocker locker(&_capsMutex);
    if (!_availableCaps.contains(capability)) {
        _availableCaps.append(capability);
        SYNC_OTHER(addCap, ARG(capability))
    }
}

void Network::acknowledgeCap(const QString& capability)
{
    QMutexLocker locker(&_capsMutex);
    if (!_enabledCaps.contains(capability)) {
        _enabledCaps.append(capability);
        SYNC_OTHER(acknowledgeCap, ARG(capability))
    }
}

void Network::removeCap(const QString& capability)
{
    QMutexLocker locker(&_capsMutex);
    if (_availableCaps.contains(capability) || _enabledCaps.contains(capability)) {
        _availableCaps.removeAll(capability);
        _enabledCaps.removeAll(capability);
        SYNC_OTHER(removeCap, ARG(capability))
    }
}

void Network::clearCaps()
{
    QMutexLocker locker(&_capsMutex);
    if (!_availableCaps.isEmpty() || !_enabledCaps.isEmpty()) {
        _availableCaps.clear();
        _enabledCaps.clear();
        SYNC_OTHER(clearCaps, NO_ARG)
    }
}

void Network::updateAutoAway(bool active, const QDateTime& lastAwayMessageTime)
{
    if (_autoAwayActive != active || _lastAwayMessageTime != lastAwayMessageTime) {
        _autoAwayActive = active;
        _lastAwayMessageTime = lastAwayMessageTime;
        SYNC_OTHER(updateAutoAway, ARG(active), ARG(lastAwayMessageTime))
    }
}

// ====================
//  Private Slots:
// ====================
void Network::ircUserNickSet(const QString& newnick)
{
    IrcUser* ircuser = qobject_cast<IrcUser*>(sender());
    Q_ASSERT(ircuser);
    if (_ircUsers.contains(ircuser->nick().toLower())) {
        _ircUsers.remove(ircuser->nick().toLower());
        _ircUsers[newnick.toLower()] = ircuser;
    }
}

void Network::ircUserDestroyed()
{
    IrcUser* ircuser = static_cast<IrcUser*>(sender());
    removeIrcUser(ircuser);
}

QDataStream& operator<<(QDataStream& out, const NetworkInfo& info)
{
    QVariantMap m;
    m["NetworkId"] = QVariant::fromValue(info.networkId);
    m["NetworkName"] = info.networkName;
    m["Identity"] = QVariant::fromValue(info.identity);
    m["CodecForServer"] = info.codecForServer;
    m["CodecForEncoding"] = info.codecForEncoding;
    m["CodecForDecoding"] = info.codecForDecoding;
    m["ServerList"] = QVariant::fromValue(info.serverList);
    m["UseRandomServer"] = info.useRandomServer;
    m["Perform"] = info.perform;
    m["SkipCaps"] = info.skipCaps;
    m["UseAutoIdentify"] = info.useAutoIdentify;
    m["AutoIdentifyService"] = info.autoIdentifyService;
    m["AutoIdentifyPassword"] = info.autoIdentifyPassword;
    m["UseSasl"] = info.useSasl;
    m["SaslAccount"] = info.saslAccount;
    m["SaslPassword"] = info.saslPassword;
    m["UseAutoReconnect"] = info.useAutoReconnect;
    m["AutoReconnectInterval"] = info.autoReconnectInterval;
    m["AutoReconnectRetries"] = info.autoReconnectRetries;
    m["UnlimitedReconnectRetries"] = info.unlimitedReconnectRetries;
    m["RejoinChannels"] = info.rejoinChannels;
    m["UseCustomMessageRate"] = info.useCustomMessageRate;
    m["MessageRateBurstSize"] = info.messageRateBurstSize;
    m["MessageRateDelay"] = info.messageRateDelay;
    m["UnlimitedMessageRate"] = info.unlimitedMessageRate;
    out << m;
    return out;
}

QDataStream& operator>>(QDataStream& in, NetworkInfo& info)
{
    QVariantMap m;
    in >> m;
    info.fromVariantMap(m);
    return in;
}

QDataStream& operator<<(QDataStream& out, const Network::Server& server)
{
    QVariantMap m;
    m["Host"] = server.host;
    m["Port"] = server.port;
    m["Password"] = server.password;
    m["UseSSL"] = server.useSsl;
    m["sslVerify"] = server.sslVerify;
    m["sslVersion"] = server.sslVersion;
    m["UseProxy"] = server.useProxy;
    m["ProxyType"] = server.proxyType;
    m["ProxyHost"] = server.proxyHost;
    m["ProxyPort"] = server.proxyPort;
    m["ProxyUser"] = server.proxyUser;
    m["ProxyPass"] = server.proxyPass;
    out << m;
    return out;
}

QDataStream& operator>>(QDataStream& in, Network::Server& server)
{
    QVariantMap m;
    in >> m;
    server.host = m.value("Host").toString();
    server.port = m.value("Port").toUInt();
    server.password = m.value("Password").toString();
    server.useSsl = m.value("UseSSL").toBool();
    server.sslVerify = m.value("sslVerify").toBool();
    server.sslVersion = m.value("sslVersion").toInt();
    server.useProxy = m.value("UseProxy").toBool();
    server.proxyType = m.value("ProxyType").toInt();
    server.proxyHost = m.value("ProxyHost").toString();
    server.proxyPort = m.value("ProxyPort").toUInt();
    server.proxyUser = m.value("ProxyUser").toString();
    server.proxyPass = m.value("ProxyPass").toString();
    return in;
}

QVariantMap NetworkInfo::toVariantMap() const
{
    QVariantMap m;
    m["NetworkId"] = QVariant::fromValue(networkId);
    m["NetworkName"] = networkName;
    m["Identity"] = QVariant::fromValue(identity);
    m["CodecForServer"] = codecForServer;
    m["CodecForEncoding"] = codecForEncoding;
    m["CodecForDecoding"] = codecForDecoding;
    m["ServerList"] = QVariant::fromValue(serverList);
    m["UseRandomServer"] = useRandomServer;
    m["Perform"] = perform;
    m["SkipCaps"] = skipCaps;
    m["UseAutoIdentify"] = useAutoIdentify;
    m["AutoIdentifyService"] = autoIdentifyService;
    m["AutoIdentifyPassword"] = autoIdentifyPassword;
    m["UseSasl"] = useSasl;
    m["SaslAccount"] = saslAccount;
    m["SaslPassword"] = saslPassword;
    m["UseAutoReconnect"] = useAutoReconnect;
    m["AutoReconnectInterval"] = autoReconnectInterval;
    m["AutoReconnectRetries"] = autoReconnectRetries;
    m["UnlimitedReconnectRetries"] = unlimitedReconnectRetries;
    m["RejoinChannels"] = rejoinChannels;
    m["UseCustomMessageRate"] = useCustomMessageRate;
    m["MessageRateBurstSize"] = messageRateBurstSize;
    m["MessageRateDelay"] = messageRateDelay;
    m["UnlimitedMessageRate"] = unlimitedMessageRate;
    return m;
}

void NetworkInfo::fromVariantMap(const QVariantMap& m)
{
    networkId = m.value("NetworkId").value<NetworkId>();
    networkName = m.value("NetworkName").toString();
    identity = m.value("Identity").value<IdentityId>();
    codecForServer = m.value("CodecForServer").toByteArray();
    codecForEncoding = m.value("CodecForEncoding").toByteArray();
    codecForDecoding = m.value("CodecForDecoding").toByteArray();
    serverList = m.value("ServerList").value<Network::ServerList>();
    useRandomServer = m.value("UseRandomServer").toBool();
    perform = m.value("Perform").toStringList();
    skipCaps = m.value("SkipCaps").toStringList();
    useAutoIdentify = m.value("UseAutoIdentify").toBool();
    autoIdentifyService = m.value("AutoIdentifyService").toString();
    autoIdentifyPassword = m.value("AutoIdentifyPassword").toString();
    useSasl = m.value("UseSasl").toBool();
    saslAccount = m.value("SaslAccount").toString();
    saslPassword = m.value("SaslPassword").toString();
    useAutoReconnect = m.value("UseAutoReconnect").toBool();
    autoReconnectInterval = m.value("AutoReconnectInterval").toUInt();
    autoReconnectRetries = m.value("AutoReconnectRetries").toInt();
    unlimitedReconnectRetries = m.value("UnlimitedReconnectRetries").toBool();
    rejoinChannels = m.value("RejoinChannels").toBool();
    useCustomMessageRate = m.value("UseCustomMessageRate").toBool();
    messageRateBurstSize = m.value("MessageRateBurstSize").toUInt();
    messageRateDelay = m.value("MessageRateDelay").toUInt();
    unlimitedMessageRate = m.value("UnlimitedMessageRate").toBool();
}

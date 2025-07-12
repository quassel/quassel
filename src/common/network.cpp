/***************************************************************************
 *   Copyright (C) 2005-2025 by the Quassel Project                        *
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
#include <QMetaEnum>

#include "ircchannel.h"
#include "ircuser.h"
#include "signalproxy.h"
#include "util.h"

// Static members
std::optional<QStringConverter::Encoding> Network::_defaultCodecForServer;
std::optional<QStringConverter::Encoding> Network::_defaultCodecForEncoding;
std::optional<QStringConverter::Encoding> Network::_defaultCodecForDecoding;

Network::Network(const NetworkId& networkId, QObject* parent)
    : SyncableObject(parent)
    , _networkId(networkId)
{
    static_cast<QObject*>(this)->setObjectName(QString::number(networkId.toInt()));
    
    // Initialize default channel prefixes (RFC 2812 and common extensions)
    // These will be updated when the server sends RPL_ISUPPORT (005)
    _channelPrefixes = {"#", "&"};
}

NetworkInfo Network::toNetworkInfo() const
{
    NetworkInfo info;
    info.networkId = networkId();
    info.networkName = networkName();
    info.identity = identity();
    info.codecForServer = codecForServer();
    info.codecForEncoding = codecForEncoding();
    info.codecForDecoding = codecForDecoding();
    info.serverList = serverList();
    info.useRandomServer = useRandomServer();
    info.perform = perform();
    info.skipCaps = skipCaps();
    info.useAutoIdentify = useAutoIdentify();
    info.autoIdentifyService = autoIdentifyService();
    info.autoIdentifyPassword = autoIdentifyPassword();
    info.useSasl = useSasl();
    info.saslAccount = saslAccount();
    info.saslPassword = saslPassword();
    info.useAutoReconnect = useAutoReconnect();
    info.autoReconnectInterval = autoReconnectInterval();
    info.autoReconnectRetries = autoReconnectRetries();
    info.unlimitedReconnectRetries = unlimitedReconnectRetries();
    info.rejoinChannels = rejoinChannels();
    info.useCustomMessageRate = useCustomMessageRate();
    info.messageRateBurstSize = messageRateBurstSize();
    info.messageRateDelay = messageRateDelay();
    info.unlimitedMessageRate = unlimitedMessageRate();
    return info;
}

Network::~Network()
{
    removeChansAndUsers();
}

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

void Network::setCodecForServer(const QByteArray& codecName)
{
    auto encoding = QStringConverter::encodingForName(codecName.constData());
    _codecForServer = encoding;
}

void Network::setCodecForEncoding(const QByteArray& codecName)
{
    auto encoding = QStringConverter::encodingForName(codecName.constData());
    _codecForEncoding = encoding;
}

void Network::setCodecForDecoding(const QByteArray& codecName)
{
    auto encoding = QStringConverter::encodingForName(codecName.constData());
    _codecForDecoding = encoding;
}

void Network::setDefaultCodecForServer(const QByteArray& name)
{
    auto encoding = QStringConverter::encodingForName(name.constData());
    _defaultCodecForServer = encoding;
}

void Network::setDefaultCodecForEncoding(const QByteArray& name)
{
    auto encoding = QStringConverter::encodingForName(name.constData());
    _defaultCodecForEncoding = encoding;
}

void Network::setDefaultCodecForDecoding(const QByteArray& name)
{
    auto encoding = QStringConverter::encodingForName(name.constData());
    _defaultCodecForDecoding = encoding;
}

QByteArray Network::codecForServer() const
{
    return _codecForServer ? QByteArray(QStringConverter::nameForEncoding(*_codecForServer)) : defaultCodecForServer();
}

QByteArray Network::codecForEncoding() const
{
    return _codecForEncoding ? QByteArray(QStringConverter::nameForEncoding(*_codecForEncoding)) : defaultCodecForEncoding();
}

QByteArray Network::codecForDecoding() const
{
    return _codecForDecoding ? QByteArray(QStringConverter::nameForEncoding(*_codecForDecoding)) : defaultCodecForDecoding();
}

QByteArray Network::defaultCodecForServer()
{
    return _defaultCodecForServer ? QByteArray(QStringConverter::nameForEncoding(*_defaultCodecForServer)) : QByteArray("UTF-8");
}

QByteArray Network::defaultCodecForEncoding()
{
    return _defaultCodecForEncoding ? QByteArray(QStringConverter::nameForEncoding(*_defaultCodecForEncoding)) : QByteArray("UTF-8");
}

QByteArray Network::defaultCodecForDecoding()
{
    return _defaultCodecForDecoding ? QByteArray(QStringConverter::nameForEncoding(*_defaultCodecForDecoding)) : QByteArray("UTF-8");
}

QByteArray Network::encodeString(const QString& string) const
{
    auto codec = _codecForEncoding.value_or(_defaultCodecForEncoding.value_or(QStringConverter::Utf8));
    QStringEncoder encoder(codec);
    return encoder.encode(string);
}

QString Network::decodeString(const QByteArray& string) const
{
    auto codec = _codecForDecoding.value_or(_defaultCodecForDecoding.value_or(QStringConverter::Utf8));
    QStringDecoder decoder(codec);
    return decoder.decode(string);
}

QByteArray Network::encodeServerString(const QString& string) const
{
    auto codec = _codecForServer.value_or(_defaultCodecForServer.value_or(QStringConverter::Utf8));
    QStringEncoder encoder(codec);
    return encoder.encode(string);
}

QString Network::decodeServerString(const QByteArray& string) const
{
    auto codec = _codecForServer.value_or(_defaultCodecForServer.value_or(QStringConverter::Utf8));
    QStringDecoder decoder(codec);
    return decoder.decode(string);
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
        qDebug() << "[DEBUG] Network::setConnected" << networkId() << "changing from" << _connected << "to" << connected;
        _connected = connected;
        SYNC(ARG(connected))
        _connectionState = connected ? ConnectionState::Initialized : ConnectionState::Disconnected;

        emit connected ? Network::connected() : Network::disconnected();
        emit connectionStateSet(static_cast<ConnectionState>(_connectionState));
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
    if (_channelPrefixes != prefixes.split("", Qt::SkipEmptyParts)) {
        _channelPrefixes = prefixes.split("", Qt::SkipEmptyParts);
        SYNC(ARG(prefixes))
    }
}

void Network::setPrefixModes(const QString& modes)
{
    if (_prefixModes != modes.split("", Qt::SkipEmptyParts)) {
        _prefixModes = modes.split("", Qt::SkipEmptyParts);
        SYNC(ARG(modes))
    }
}

void Network::setPrefixes(const QString& modes, const QString& prefixes)
{
    QStringList modeList = modes.split("", Qt::SkipEmptyParts);
    QStringList prefixList = prefixes.split("", Qt::SkipEmptyParts);
    if (_prefixModes != modeList || _prefixes != prefixList) {
        _prefixModes = modeList;
        _prefixes = prefixList;
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
    if (!_supports.contains(param) || _supports[param] != value) {
        _supports[param] = value;
        SYNC_OTHER(addSupport, ARG(param), ARG(value))
    }
}

void Network::removeSupport(const QString& param)
{
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

void Network::addCap(const QString& capability, const QString& value)
{
    QMutexLocker locker(&_capsMutex);
    if (!_availableCaps.contains(capability)) {
        _availableCaps << capability;
        SYNC_OTHER(addCap, ARG(capability), ARG(value))
    }
}

void Network::acknowledgeCap(const QString& capability)
{
    QMutexLocker locker(&_capsMutex);
    if (_availableCaps.contains(capability) && !_enabledCaps.contains(capability)) {
        _enabledCaps << capability;
        SYNC_OTHER(acknowledgeCap, ARG(capability))
    }
}

void Network::removeCap(const QString& capability)
{
    QMutexLocker locker(&_capsMutex);
    if (_availableCaps.contains(capability)) {
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

bool Network::isChannelName(const QString& channelname) const
{
    if (channelname.isEmpty())
        return false;

    return _channelPrefixes.contains(channelname[0]);
}

bool Network::isStatusMsg(const QString& target) const
{
    if (target.isEmpty())
        return false;

    QStringList prefixes = _statusMsg.isEmpty() ? QStringList() << "@" << "+" : _statusMsg.split("", Qt::SkipEmptyParts);
    return prefixes.contains(target[0]);
}

QString Network::prefixToMode(const QString& prefix) const
{
    int index = _prefixes.indexOf(prefix);
    if (index == -1)
        return QString();
    return _prefixModes.value(index, QString());
}

QString Network::modeToPrefix(const QString& mode) const
{
    int index = _prefixModes.indexOf(mode);
    if (index == -1)
        return QString();
    return _prefixes.value(index, QString());
}

QString Network::sortPrefixModes(const QString& modes) const
{
    QString sortedModes;
    for (const QString& mode : _prefixModes) {
        if (modes.contains(mode))
            sortedModes += mode;
    }
    for (const QChar& mode : modes) {
        if (!_prefixModes.contains(mode))
            sortedModes += mode;
    }
    return sortedModes;
}

Network::ChannelModeType Network::channelModeType(const QString& mode) const
{
    if (_supports.contains("CHANMODES")) {
        QStringList chanmodes = _supports["CHANMODES"].split(",");
        if (chanmodes.size() >= 4) {
            if (chanmodes[0].contains(mode))
                return A_CHANMODE;
            if (chanmodes[1].contains(mode))
                return B_CHANMODE;
            if (chanmodes[2].contains(mode))
                return C_CHANMODE;
            if (chanmodes[3].contains(mode))
                return D_CHANMODE;
        }
    }
    return NOT_A_CHANMODE;
}

IrcUser* Network::ircUser(const QString& nickname) const
{
    QString nick = nickname.toLower();
    if (_ircUsers.contains(nick))
        return _ircUsers[nick];
    return nullptr;
}

IrcChannel* Network::ircChannel(const QString& channelname) const
{
    QString chan = channelname.toLower();
    if (_ircChannels.contains(chan))
        return _ircChannels[chan];
    return nullptr;
}

IrcUser* Network::newIrcUser(const QString& hostmask, const QVariantMap& initData)
{
    QString nick = nickFromMask(hostmask).toLower();
    if (_ircUsers.contains(nick))
        return _ircUsers[nick];

    IrcUser* ircUser = new IrcUser(hostmask, this);
    _ircUsers[nick] = ircUser;
    QString objectName = QString("%1/%2").arg(_networkId.toInt()).arg(nickFromMask(hostmask));
    static_cast<QObject*>(ircUser)->setObjectName(objectName);
    connect(ircUser, &IrcUser::nickSet, this, &Network::ircUserNickSet);
    connect(ircUser, &QObject::destroyed, this, &Network::ircUserDestroyed);

    qDebug() << "[DEBUG] Network::newIrcUser creating:" << hostmask << "objectName:" << objectName << "nick:" << nick;

    if (!initData.isEmpty())
        ircUser->fromVariantMap(initData);

    if (_proxy) {
        qDebug() << "[DEBUG] Synchronizing IrcUser:" << objectName << "with proxy";
        _proxy->synchronize(ircUser);
        // Notify client side to create the IrcUser object
        SYNC_OTHER(newIrcUserCreated, ARG(hostmask), ARG(initData))
    } else {
        qDebug() << "[DEBUG] No proxy available for IrcUser:" << objectName;
    }

    qDebug() << "[DEBUG] Network::newIrcUser emitting newIrcUserSynced for:" << objectName;
    emit newIrcUserSynced(ircUser);
    return ircUser;
}

IrcChannel* Network::newIrcChannel(const QString& channelname, const QVariantMap& initData)
{
    QString chan = channelname.toLower();
    if (_ircChannels.contains(chan))
        return _ircChannels[chan];

    IrcChannel* ircChannel = new IrcChannel(channelname, this);
    _ircChannels[chan] = ircChannel;
    QString objectName = QString("%1/%2").arg(_networkId.toInt()).arg(channelname);
    static_cast<QObject*>(ircChannel)->setObjectName(objectName);

    qDebug() << "[DEBUG] Creating IrcChannel:" << channelname << "objectName:" << objectName << "chan:" << chan;

    if (!initData.isEmpty())
        ircChannel->fromVariantMap(initData);

    if (_proxy) {
        qDebug() << "[DEBUG] Synchronizing IrcChannel:" << objectName << "with proxy";
        _proxy->synchronize(ircChannel);
        
        // Also sync the Network object to ensure client receives updated channel list
        qDebug() << "[DEBUG] Triggering Network sync update for new IrcChannel";
        SYNC_OTHER(newIrcChannelCreated, ARG(channelname), ARG(initData))
    } else {
        qDebug() << "[DEBUG] No proxy available for IrcChannel:" << objectName;
    }

    emit newIrcChannelSynced(ircChannel);
    return ircChannel;
}

void Network::removeIrcUser(IrcUser* ircUser)
{
    if (!ircUser)
        return;

    QString nick = ircUser->nick().toLower();
    if (_ircUsers.contains(nick)) {
        _ircUsers.remove(nick);
        emit ircUserRemoved(ircUser);
        ircUser->deleteLater();
    }
}

void Network::removeIrcChannel(IrcChannel* ircChannel)
{
    if (!ircChannel)
        return;

    QString chan = ircChannel->name().toLower();
    if (_ircChannels.contains(chan)) {
        _ircChannels.remove(chan);
        emit ircChannelRemoved(ircChannel);
        ircChannel->deleteLater();
    }
}

void Network::removeChansAndUsers()
{
    QList<IrcUser*> users = _ircUsers.values();
    _ircUsers.clear();
    for (IrcUser* user : users) {
        emit ircUserRemoved(user);
        user->deleteLater();
    }

    QList<IrcChannel*> channels = _ircChannels.values();
    _ircChannels.clear();
    for (IrcChannel* channel : channels) {
        emit ircChannelRemoved(channel);
        channel->deleteLater();
    }
}

void Network::ircUserNickSet(const QString& newnick)
{
    IrcUser* ircUser = qobject_cast<IrcUser*>(sender());
    if (!ircUser)
        return;

    QString oldnick = ircUser->nick().toLower();
    QString newnickLower = newnick.toLower();
    if (_ircUsers.contains(oldnick)) {
        _ircUsers.remove(oldnick);
        _ircUsers[newnickLower] = ircUser;
        static_cast<QObject*>(ircUser)->setObjectName(QString("%1/%2").arg(_networkId.toInt()).arg(newnick));
    }
}

void Network::ircUserDestroyed()
{
    IrcUser* ircUser = qobject_cast<IrcUser*>(sender());
    if (ircUser)
        removeIrcUser(ircUser);
}

QVariantMap Network::initIrcUsersAndChannels() const
{
    QVariantMap result;
    QVariantMap users;
    for (auto it = _ircUsers.constBegin(); it != _ircUsers.constEnd(); ++it) {
        users[it.key()] = it.value()->toVariantMap();
    }
    QVariantMap channels;
    for (auto it = _ircChannels.constBegin(); it != _ircChannels.constEnd(); ++it) {
        channels[it.key()] = it.value()->toVariantMap();
    }
    result["IrcUsers"] = users;
    result["IrcChannels"] = channels;
    
    qDebug() << "[DEBUG] initIrcUsersAndChannels returning:" << users.size() << "users and" << channels.size() << "channels";
    qDebug() << "[DEBUG] User keys:" << users.keys();
    qDebug() << "[DEBUG] Channel keys:" << channels.keys();
    
    return result;
}

void Network::initSetIrcUsersAndChannels(const QVariantMap& usersAndChannels)
{
    qDebug() << "[DEBUG] initSetIrcUsersAndChannels called on network:" << networkId() << "with" << usersAndChannels.size() << "items";
    
    QVariantMap users = usersAndChannels["IrcUsers"].toMap();
    qDebug() << "[DEBUG] Processing" << users.size() << "IrcUsers:" << users.keys();
    for (auto it = users.constBegin(); it != users.constEnd(); ++it) {
        qDebug() << "[DEBUG] Creating IrcUser from init data:" << it.key();
        newIrcUser(it.key(), it.value().toMap());
    }
    
    QVariantMap channels = usersAndChannels["IrcChannels"].toMap();
    qDebug() << "[DEBUG] Processing" << channels.size() << "IrcChannels:" << channels.keys();
    for (auto it = channels.constBegin(); it != channels.constEnd(); ++it) {
        qDebug() << "[DEBUG] Creating IrcChannel from init data:" << it.key();
        newIrcChannel(it.key(), it.value().toMap());
    }
}

void Network::newIrcUserCreated(const QString& hostmask, const QVariantMap& initData)
{
    qDebug() << "[DEBUG] newIrcUserCreated called for user:" << hostmask << "on network:" << networkId();
    
    // This method is called on the client side when the server creates a new IrcUser
    // We need to create the IrcUser object on the client side too
    QString nick = nickFromMask(hostmask).toLower();
    if (!_ircUsers.contains(nick)) {
        qDebug() << "[DEBUG] Creating IrcUser on client side:" << hostmask;
        newIrcUser(hostmask, initData);
    } else {
        qDebug() << "[DEBUG] IrcUser already exists on client side:" << hostmask;
    }
}

void Network::newIrcChannelCreated(const QString& channelname, const QVariantMap& initData)
{
    qDebug() << "[DEBUG] newIrcChannelCreated called for channel:" << channelname << "on network:" << networkId();
    
    // This method is called on the client side when the server creates a new IrcChannel
    // We need to create the IrcChannel object on the client side too
    if (!ircChannel(channelname)) {
        qDebug() << "[DEBUG] Creating IrcChannel on client side:" << channelname;
        newIrcChannel(channelname, initData);
    } else {
        qDebug() << "[DEBUG] IrcChannel already exists on client side:" << channelname;
    }
}

QVariantMap Network::toVariantMap()
{
    qDebug() << "[DEBUG] Network::toVariantMap() called for network" << networkId();
    
    QVariantMap map = SyncableObject::toVariantMap();
    
    // Include IrcUsers and IrcChannels data
    QVariantMap usersAndChannels = initIrcUsersAndChannels();
    
    // Qt6 doesn't have unite(), use insert() instead
    for (auto it = usersAndChannels.constBegin(); it != usersAndChannels.constEnd(); ++it) {
        map.insert(it.key(), it.value());
    }
    
    qDebug() << "[DEBUG] Network::toVariantMap() including" << usersAndChannels.size() << "items from initIrcUsersAndChannels";
    
    return map;
}

void Network::fromVariantMap(const QVariantMap& map)
{
    qDebug() << "[DEBUG] Network::fromVariantMap() called for network" << networkId() << "with" << map.size() << "items";
    qDebug() << "[DEBUG] Map keys:" << map.keys();
    
    SyncableObject::fromVariantMap(map);
    
    // Handle IrcUsers and IrcChannels data
    if (map.contains("IrcUsers") || map.contains("IrcChannels")) {
        QVariantMap usersAndChannels;
        if (map.contains("IrcUsers")) {
            usersAndChannels["IrcUsers"] = map["IrcUsers"];
            qDebug() << "[DEBUG] fromVariantMap found IrcUsers:" << map["IrcUsers"].toMap().keys();
        }
        if (map.contains("IrcChannels")) {
            usersAndChannels["IrcChannels"] = map["IrcChannels"];
            qDebug() << "[DEBUG] fromVariantMap found IrcChannels:" << map["IrcChannels"].toMap().keys();
        }
        
        qDebug() << "[DEBUG] Network::fromVariantMap() calling initSetIrcUsersAndChannels with" << usersAndChannels.size() << "items";
        initSetIrcUsersAndChannels(usersAndChannels);
    }
}

bool Network::Server::operator==(const Server& other) const
{
    return host == other.host && port == other.port && password == other.password && useSsl == other.useSsl && sslVerify == other.sslVerify
           && sslVersion == other.sslVersion && useProxy == other.useProxy && proxyType == other.proxyType && proxyHost == other.proxyHost
           && proxyPort == other.proxyPort && proxyUser == other.proxyUser && proxyPass == other.proxyPass;
}

bool Network::Server::operator!=(const Server& other) const
{
    return !(*this == other);
}

QDataStream& operator<<(QDataStream& out, const NetworkInfo& info)
{
    out << info.networkId << info.networkName << info.identity << info.codecForServer << info.codecForEncoding << info.codecForDecoding
        << info.serverList << info.useRandomServer << info.perform << info.skipCaps << info.useAutoIdentify << info.autoIdentifyService
        << info.autoIdentifyPassword << info.useSasl << info.saslAccount << info.saslPassword << info.useAutoReconnect
        << info.autoReconnectInterval << info.autoReconnectRetries << info.unlimitedReconnectRetries << info.rejoinChannels
        << info.useCustomMessageRate << info.messageRateBurstSize << info.messageRateDelay << info.unlimitedMessageRate;
    return out;
}

QDataStream& operator>>(QDataStream& in, NetworkInfo& info)
{
    in >> info.networkId >> info.networkName >> info.identity >> info.codecForServer >> info.codecForEncoding >> info.codecForDecoding
        >> info.serverList >> info.useRandomServer >> info.perform >> info.skipCaps >> info.useAutoIdentify >> info.autoIdentifyService
        >> info.autoIdentifyPassword >> info.useSasl >> info.saslAccount >> info.saslPassword >> info.useAutoReconnect
        >> info.autoReconnectInterval >> info.autoReconnectRetries >> info.unlimitedReconnectRetries >> info.rejoinChannels
        >> info.useCustomMessageRate >> info.messageRateBurstSize >> info.messageRateDelay >> info.unlimitedMessageRate;
    return in;
}

QDataStream& operator<<(QDataStream& out, const Network::Server& server)
{
    out << server.host << server.port << server.password << server.useSsl << server.sslVerify << server.sslVersion << server.useProxy
        << server.proxyType << server.proxyHost << server.proxyPort << server.proxyUser << server.proxyPass;
    return out;
}

QDataStream& operator>>(QDataStream& in, Network::Server& server)
{
    in >> server.host >> server.port >> server.password >> server.useSsl >> server.sslVerify >> server.sslVersion >> server.useProxy
        >> server.proxyType >> server.proxyHost >> server.proxyPort >> server.proxyUser >> server.proxyPass;
    return in;
}

QVariantMap NetworkInfo::toVariantMap() const
{
    QVariantMap map;
    map["networkId"] = networkId.toInt();
    map["networkName"] = networkName;
    map["identity"] = identity.toInt();
    map["codecForServer"] = codecForServer;
    map["codecForEncoding"] = codecForEncoding;
    map["codecForDecoding"] = codecForDecoding;
    map["serverList"] = QVariant::fromValue(serverList);
    map["useRandomServer"] = useRandomServer;
    map["perform"] = perform;
    map["skipCaps"] = skipCaps;
    map["useAutoIdentify"] = useAutoIdentify;
    map["autoIdentifyService"] = autoIdentifyService;
    map["autoIdentifyPassword"] = autoIdentifyPassword;
    map["useSasl"] = useSasl;
    map["saslAccount"] = saslAccount;
    map["saslPassword"] = saslPassword;
    map["useAutoReconnect"] = useAutoReconnect;
    map["autoReconnectInterval"] = autoReconnectInterval;
    map["autoReconnectRetries"] = autoReconnectRetries;
    map["unlimitedReconnectRetries"] = unlimitedReconnectRetries;
    map["rejoinChannels"] = rejoinChannels;
    map["useCustomMessageRate"] = useCustomMessageRate;
    map["messageRateBurstSize"] = messageRateBurstSize;
    map["messageRateDelay"] = messageRateDelay;
    map["unlimitedMessageRate"] = unlimitedMessageRate;
    return map;
}


void NetworkInfo::fromVariantMap(const QVariantMap& map)
{
    networkId = NetworkId(map.value("networkId").toInt());
    networkName = map.value("networkName").toString();
    identity = IdentityId(map.value("identity").toInt());
    codecForServer = map.value("codecForServer").toByteArray();
    codecForEncoding = map.value("codecForEncoding").toByteArray();
    codecForDecoding = map.value("codecForDecoding").toByteArray();
    serverList = map.value("serverList").value<Network::ServerList>();
    useRandomServer = map.value("useRandomServer").toBool();
    perform = map.value("perform").toStringList();
    skipCaps = map.value("skipCaps").toStringList();
    useAutoIdentify = map.value("useAutoIdentify").toBool();
    autoIdentifyService = map.value("autoIdentifyService").toString();
    autoIdentifyPassword = map.value("autoIdentifyPassword").toString();
    useSasl = map.value("useSasl").toBool();
    saslAccount = map.value("saslAccount").toString();
    saslPassword = map.value("saslPassword").toString();
    useAutoReconnect = map.value("useAutoReconnect").toBool();
    autoReconnectInterval = map.value("autoReconnectInterval").toUInt();
    autoReconnectRetries = map.value("autoReconnectRetries").toUInt();
    unlimitedReconnectRetries = map.value("unlimitedReconnectRetries").toBool();
    rejoinChannels = map.value("rejoinChannels").toBool();
    useCustomMessageRate = map.value("useCustomMessageRate").toBool();
    messageRateBurstSize = map.value("messageRateBurstSize").toUInt();
    messageRateDelay = map.value("messageRateDelay").toUInt();
    unlimitedMessageRate = map.value("unlimitedMessageRate").toBool();
}

void Network::requestConnect()
{
    qDebug() << "[DEBUG] Network::requestConnect called for network" << networkId();
    qDebug() << "[DEBUG] Network::requestConnect proxy:" << proxy() << "thread:" << QThread::currentThread();
    // Call the SignalProxy method to emit the signal
    if (proxy()) {
        qDebug() << "[DEBUG] Network::requestConnect calling SignalProxy::requestNetworkConnect";
        proxy()->requestNetworkConnect(networkId());
        qDebug() << "[DEBUG] Network::requestConnect finished calling SignalProxy::requestNetworkConnect";
    } else {
        qDebug() << "[DEBUG] Network::requestConnect - no proxy available!";
    }
}

void Network::requestDisconnect()
{
    qDebug() << "[DEBUG] Network::requestDisconnect called for network" << networkId();
    // Call the SignalProxy method to emit the signal
    if (proxy()) {
        qDebug() << "[DEBUG] Network::requestDisconnect calling SignalProxy::requestNetworkDisconnect";
        proxy()->requestNetworkDisconnect(networkId());
    }
}

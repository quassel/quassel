/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <algorithm>

#include <QTextCodec>

#include "peer.h"

QTextCodec* Network::_defaultCodecForServer = nullptr;
QTextCodec* Network::_defaultCodecForEncoding = nullptr;
QTextCodec* Network::_defaultCodecForDecoding = nullptr;

// ====================
//  Public:
// ====================

Network::Network(const NetworkId& networkid, QObject* parent)
    : SyncableObject(parent)
    , _proxy(nullptr)
    , _networkId(networkid)
    , _identity(0)
    , _myNick(QString())
    , _latency(0)
    , _networkName(QString("<not initialized>"))
    , _currentServer(QString())
    , _connected(false)
    , _connectionState(Disconnected)
    , _prefixes(QString())
    , _prefixModes(QString())
    , _useRandomServer(false)
    , _useAutoIdentify(false)
    , _useSasl(false)
    , _useAutoReconnect(false)
    , _autoReconnectInterval(60)
    , _autoReconnectRetries(10)
    , _unlimitedReconnectRetries(false)
    , _useCustomMessageRate(false)
    , _messageRateBurstSize(5)
    , _messageRateDelay(2200)
    , _unlimitedMessageRate(false)
    , _codecForServer(nullptr)
    , _codecForEncoding(nullptr)
    , _codecForDecoding(nullptr)
    , _autoAwayActive(false)
{
    setObjectName(QString::number(networkid.toInt()));
}

Network::~Network()
{
    emit aboutToBeDestroyed();
}

bool Network::isChannelName(const QString& channelname) const
{
    if (channelname.isEmpty())
        return false;

    if (supports("CHANTYPES"))
        return support("CHANTYPES").contains(channelname[0]);
    else
        return QString("#&!+").contains(channelname[0]);
}

bool Network::isStatusMsg(const QString& target) const
{
    if (target.isEmpty())
        return false;

    if (supports("STATUSMSG"))
        return support("STATUSMSG").contains(target[0]);
    else
        return QString("@+").contains(target[0]);
}

NetworkInfo Network::networkInfo() const
{
    NetworkInfo info;
    info.networkName = networkName();
    info.networkId = networkId();
    info.identity = identity();
    info.codecForServer = codecForServer();
    info.codecForEncoding = codecForEncoding();
    info.codecForDecoding = codecForDecoding();
    info.serverList = serverList();
    info.useRandomServer = useRandomServer();
    info.perform = perform();
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

void Network::setNetworkInfo(const NetworkInfo& info)
{
    // we don't set our ID!

    setNetworkName(info.networkName);
    setIdentity(info.identity);
    setCodecForServer(info.codecForServer);
    setCodecForEncoding(info.codecForEncoding);
    setCodecForDecoding(info.codecForDecoding);
    setServerList(info.serverList);
    setUseRandomServer(info.useRandomServer);
    setPerform(info.perform);
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
}

QString Network::prefixToMode(const QString& prefix) const
{
    if (prefixes().contains(prefix))
        return QString(prefixModes()[prefixes().indexOf(prefix)]);
    else
        return QString();
}

QString Network::modeToPrefix(const QString& mode) const
{
    if (prefixModes().contains(mode))
        return QString(prefixes()[prefixModes().indexOf(mode)]);
    else
        return QString();
}

QString Network::sortPrefixModes(const QString& modes) const
{
    // If modes is empty or we don't have any modes, nothing can be sorted, bail out early
    if (modes.isEmpty() || prefixModes().isEmpty()) {
        return modes;
    }

    // Store a copy of the modes for modification
    // QString should be efficient and not copy memory if nothing changes, but if mistaken,
    // std::is_sorted could be called first.
    QString sortedModes = QString(modes);

    // Sort modes as if a QChar array
    // See https://en.cppreference.com/w/cpp/algorithm/sort
    // Defining lambda with [&] implicitly captures variables by reference
    std::sort(sortedModes.begin(), sortedModes.end(), [&](const QChar& lmode, const QChar& rmode) {
        // Compare characters according to prefix modes
        // Return true if lmode comes before rmode (is "less than")

        // Check for unknown modes...
        if (!prefixModes().contains(lmode)) {
            // Left mode not in prefix list, send to end
            return false;
        }
        else if (!prefixModes().contains(rmode)) {
            // Right mode not in prefix list, send to end
            return true;
        }
        else {
            // Both characters known, sort according to index in prefixModes()
            return (prefixModes().indexOf(lmode) < prefixModes().indexOf(rmode));
        }
    });

    return sortedModes;
}

QStringList Network::nicks() const
{
    // we don't use _ircUsers.keys() since the keys may be
    // not up to date after a nick change
    QStringList nicks;
    foreach (IrcUser* ircuser, _ircUsers.values()) {
        nicks << ircuser->nick();
    }
    return nicks;
}

QString Network::prefixes() const
{
    if (_prefixes.isNull())
        determinePrefixes();

    return _prefixes;
}

QString Network::prefixModes() const
{
    if (_prefixModes.isNull())
        determinePrefixes();

    return _prefixModes;
}

// example Unreal IRCD: CHANMODES=beI,kfL,lj,psmntirRcOAQKVCuzNSMTG
Network::ChannelModeType Network::channelModeType(const QString& mode)
{
    if (mode.isEmpty())
        return NOT_A_CHANMODE;

    QString chanmodes = support("CHANMODES");
    if (chanmodes.isEmpty())
        return NOT_A_CHANMODE;

    ChannelModeType modeType = A_CHANMODE;
    for (int i = 0; i < chanmodes.count(); i++) {
        if (chanmodes[i] == mode[0])
            break;
        else if (chanmodes[i] == ',')
            modeType = (ChannelModeType)(modeType << 1);
    }
    if (modeType > D_CHANMODE) {
        qWarning() << "Network" << networkId() << "supplied invalid CHANMODES:" << chanmodes;
        modeType = NOT_A_CHANMODE;
    }
    return modeType;
}

QString Network::support(const QString& param) const
{
    QString support_ = param.toUpper();
    if (_supports.contains(support_))
        return _supports[support_];
    else
        return QString();
}

bool Network::saslMaybeSupports(const QString& saslMechanism) const
{
    if (!capAvailable(IrcCap::SASL)) {
        // If SASL's not advertised at all, it's likely the mechanism isn't supported, as per specs.
        // Unfortunately, we don't know for sure, but Quassel won't request SASL without it being
        // advertised, anyways.
        // This may also occur if the network's disconnected or negotiation hasn't yet happened.
        return false;
    }

    // Get the SASL capability value
    QString saslCapValue = capValue(IrcCap::SASL);
    // SASL mechanisms are only specified in capability values as part of SASL 3.2.  In SASL 3.1,
    // it's handled differently.  If we don't know via capability value, assume it's supported to
    // reduce the risk of breaking existing setups.
    // See: http://ircv3.net/specs/extensions/sasl-3.1.html
    // And: http://ircv3.net/specs/extensions/sasl-3.2.html
    return (saslCapValue.length() == 0) || (saslCapValue.contains(saslMechanism, Qt::CaseInsensitive));
}

IrcUser* Network::newIrcUser(const QString& hostmask, const QVariantMap& initData)
{
    QString nick(nickFromMask(hostmask).toLower());
    if (!_ircUsers.contains(nick)) {
        IrcUser* ircuser = ircUserFactory(hostmask);
        if (!initData.isEmpty()) {
            ircuser->fromVariantMap(initData);
            ircuser->setInitialized();
        }

        if (proxy())
            proxy()->synchronize(ircuser);
        else
            qWarning() << "unable to synchronize new IrcUser" << hostmask << "forgot to call Network::setProxy(SignalProxy *)?";

        connect(ircuser, &IrcUser::nickSet, this, &Network::ircUserNickChanged);

        _ircUsers[nick] = ircuser;

        // This method will be called with a nick instead of hostmask by setInitIrcUsersAndChannels().
        // Not a problem because initData contains all we need; however, making sure here to get the real
        // hostmask out of the IrcUser afterwards.
        QString mask = ircuser->hostmask();
        SYNC_OTHER(addIrcUser, ARG(mask));
        // emit ircUserAdded(mask);
        emit ircUserAdded(ircuser);
    }

    return _ircUsers[nick];
}

IrcUser* Network::ircUser(QString nickname) const
{
    nickname = nickname.toLower();
    if (_ircUsers.contains(nickname))
        return _ircUsers[nickname];
    else
        return nullptr;
}

void Network::removeIrcUser(IrcUser* ircuser)
{
    QString nick = _ircUsers.key(ircuser);
    if (nick.isNull())
        return;

    _ircUsers.remove(nick);
    disconnect(ircuser, nullptr, this, nullptr);
    ircuser->deleteLater();
}

void Network::removeIrcChannel(IrcChannel* channel)
{
    QString chanName = _ircChannels.key(channel);
    if (chanName.isNull())
        return;

    _ircChannels.remove(chanName);
    disconnect(channel, nullptr, this, nullptr);
    channel->deleteLater();
}

void Network::removeChansAndUsers()
{
    QList<IrcUser*> users = ircUsers();
    _ircUsers.clear();
    QList<IrcChannel*> channels = ircChannels();
    _ircChannels.clear();

    qDeleteAll(users);
    qDeleteAll(channels);
}

IrcChannel* Network::newIrcChannel(const QString& channelname, const QVariantMap& initData)
{
    if (!_ircChannels.contains(channelname.toLower())) {
        IrcChannel* channel = ircChannelFactory(channelname);
        if (!initData.isEmpty()) {
            channel->fromVariantMap(initData);
            channel->setInitialized();
        }

        if (proxy())
            proxy()->synchronize(channel);
        else
            qWarning() << "unable to synchronize new IrcChannel" << channelname << "forgot to call Network::setProxy(SignalProxy *)?";

        _ircChannels[channelname.toLower()] = channel;

        SYNC_OTHER(addIrcChannel, ARG(channelname))
        // emit ircChannelAdded(channelname);
        emit ircChannelAdded(channel);
    }
    return _ircChannels[channelname.toLower()];
}

IrcChannel* Network::ircChannel(QString channelname) const
{
    channelname = channelname.toLower();
    if (_ircChannels.contains(channelname))
        return _ircChannels[channelname];
    else
        return nullptr;
}

QByteArray Network::defaultCodecForServer()
{
    if (_defaultCodecForServer)
        return _defaultCodecForServer->name();
    return QByteArray();
}

void Network::setDefaultCodecForServer(const QByteArray& name)
{
    _defaultCodecForServer = QTextCodec::codecForName(name);
}

QByteArray Network::defaultCodecForEncoding()
{
    if (_defaultCodecForEncoding)
        return _defaultCodecForEncoding->name();
    return QByteArray();
}

void Network::setDefaultCodecForEncoding(const QByteArray& name)
{
    _defaultCodecForEncoding = QTextCodec::codecForName(name);
}

QByteArray Network::defaultCodecForDecoding()
{
    if (_defaultCodecForDecoding)
        return _defaultCodecForDecoding->name();
    return QByteArray();
}

void Network::setDefaultCodecForDecoding(const QByteArray& name)
{
    _defaultCodecForDecoding = QTextCodec::codecForName(name);
}

QByteArray Network::codecForServer() const
{
    return _codecForServer ? _codecForServer->name() : QByteArray{};
}

void Network::setCodecForServer(const QByteArray& name)
{
    if (codecForServer() != name) {
        _codecForServer = QTextCodec::codecForName(name);
        emit codecForServerSet(codecForServer());
        emit configChanged();
    }
}

QByteArray Network::codecForEncoding() const
{
    return _codecForEncoding ? _codecForEncoding->name() : QByteArray{};
}

void Network::setCodecForEncoding(const QByteArray& name)
{
    if (codecForEncoding() != name) {
        _codecForEncoding = QTextCodec::codecForName(name);
        emit codecForEncodingSet(codecForEncoding());
        emit configChanged();
    }
}

QByteArray Network::codecForDecoding() const
{
    return _codecForDecoding ? _codecForDecoding->name() : QByteArray{};
}

void Network::setCodecForDecoding(const QByteArray& name)
{
    if (codecForDecoding() != name) {
        _codecForDecoding = QTextCodec::codecForName(name);
        emit codecForDecodingSet(codecForDecoding());
        emit configChanged();
    }
}

// FIXME use server encoding if appropriate
QString Network::decodeString(const QByteArray& text) const
{
    if (_codecForDecoding) {
        return ::decodeString(text, _codecForDecoding);
    }
    if (_defaultCodecForDecoding) {
        return ::decodeString(text, _defaultCodecForDecoding);
    }
    return QString::fromLatin1(text);
}

QByteArray Network::encodeString(const QString& string) const
{
    if (_codecForEncoding) {
        return _codecForEncoding->fromUnicode(string);
    }
    if (_defaultCodecForEncoding) {
        return _defaultCodecForEncoding->fromUnicode(string);
    }
    return string.toLatin1();
}

QString Network::decodeServerString(const QByteArray& text) const
{
    if (_codecForServer) {
        return ::decodeString(text, _codecForServer);
    }
    if (_defaultCodecForServer) {
        return ::decodeString(text, _defaultCodecForServer);
    }
    return QString::fromLatin1(text);
}

QByteArray Network::encodeServerString(const QString& string) const
{
    if (_codecForServer) {
        return _codecForServer->fromUnicode(string);
    }
    if (_defaultCodecForServer) {
        return _defaultCodecForServer->fromUnicode(string);
    }
    return string.toLatin1();
}

// ====================
//  Public Slots:
// ====================
void Network::setNetworkName(const QString& networkName)
{
    if (_networkName != networkName) {
        _networkName = networkName;
        emit networkNameSet(networkName);
        emit configChanged();
    }
}

void Network::setCurrentServer(const QString& currentServer)
{
    if (_currentServer != currentServer) {
        _currentServer = currentServer;
        emit currentServerSet(currentServer);
    }
}

void Network::setConnected(bool connected)
{
    if (_connected != connected) {
        _connected = connected;
        if (!connected) {
            setMyNick({});
            setCurrentServer({});
            removeChansAndUsers();
        }
        emit connectedSet(connected);
    }
}

void Network::setConnectionState(int state)
{
    if (_connectionState != static_cast<ConnectionState>(state)) {
        _connectionState = static_cast<ConnectionState>(state);
        emit connectionStateSet(_connectionState);
    }
}

void Network::setMyNick(const QString& nickname)
{
    if (_myNick != nickname) {
        _myNick = nickname;
        if (!_myNick.isEmpty() && !ircUser(myNick())) {
            newIrcUser(myNick());
        }
        emit myNickSet(nickname);
    }
}

void Network::setLatency(int latency)
{
    if (_latency != latency) {
        _latency = latency;
        emit latencySet(latency);
    }
}

void Network::setIdentity(IdentityId id)
{
    if (_identity != id) {
        _identity = id;
        emit identitySet(id);
        emit configChanged();
    }
}

void Network::setServerList(const ServerList& serverList)
{
    if (_serverList != serverList) {
        _serverList = serverList;
        emit serverListSet(_serverList);
        emit configChanged();
    }
}

void Network::setUseRandomServer(bool use)
{
    if (_useRandomServer != use) {
        _useRandomServer = use;
        emit useRandomServerSet(use);
        emit configChanged();
    }
}

void Network::setPerform(const QStringList& perform)
{
    if (_perform != perform) {
        _perform = perform;
        emit performSet(perform);
        emit configChanged();
    }
}

void Network::setUseAutoIdentify(bool use)
{
    if (_useAutoIdentify != use) {
        _useAutoIdentify = use;
        emit useAutoIdentifySet(use);
        emit configChanged();
    }
}

void Network::setAutoIdentifyService(const QString& service)
{
    if (_autoIdentifyService != service) {
        _autoIdentifyService = service;
        emit autoIdentifyServiceSet(service);
        emit configChanged();
    }
}

void Network::setAutoIdentifyPassword(const QString& password)
{
    if (_autoIdentifyPassword != password) {
        _autoIdentifyPassword = password;
        emit autoIdentifyPasswordSet(password);
        emit configChanged();
    }
}

void Network::setUseSasl(bool use)
{
    if (_useSasl != use) {
        _useSasl = use;
        emit useSaslSet(use);
        emit configChanged();
    }
}

void Network::setSaslAccount(const QString& account)
{
    if (_saslAccount != account) {
        _saslAccount = account;
        emit saslAccountSet(account);
        emit configChanged();
    }
}

void Network::setSaslPassword(const QString& password)
{
    if (_saslPassword != password) {
        _saslPassword = password;
        emit saslPasswordSet(password);
        emit configChanged();
    }
}

void Network::setUseAutoReconnect(bool use)
{
    if (_useAutoReconnect != use) {
        _useAutoReconnect = use;
        emit useAutoReconnectSet(use);
        emit configChanged();
    }
}

void Network::setAutoReconnectInterval(quint32 interval)
{
    if (_autoReconnectInterval != interval) {
        _autoReconnectInterval = interval;
        emit autoReconnectIntervalSet(interval);
        emit configChanged();
    }
}

void Network::setAutoReconnectRetries(quint16 retries)
{
    if (_autoReconnectRetries != retries) {
        _autoReconnectRetries = retries;
        emit autoReconnectRetriesSet(retries);
        emit configChanged();
    }
}

void Network::setUnlimitedReconnectRetries(bool unlimited)
{
    if (_unlimitedReconnectRetries != unlimited) {
        _unlimitedReconnectRetries = unlimited;
        emit unlimitedReconnectRetriesSet(unlimited);
        emit configChanged();
    }
}

void Network::setRejoinChannels(bool rejoin)
{
    if (_rejoinChannels != rejoin) {
        _rejoinChannels = rejoin;
        emit rejoinChannelsSet(rejoin);
        emit configChanged();
    }
}

void Network::setUseCustomMessageRate(bool useCustomRate)
{
    if (_useCustomMessageRate != useCustomRate) {
        _useCustomMessageRate = useCustomRate;
        emit useCustomMessageRateSet(_useCustomMessageRate);
        emit configChanged();
    }
}

void Network::setMessageRateBurstSize(quint32 burstSize)
{
    if (burstSize < 1) {
        // Can't go slower than one message at a time.  Also blocks old clients from trying to set
        // this to 0.
        qDebug() << "Received invalid setMessageRateBurstSize data - message burst size must be "
                    "non-zero positive, given"
                 << burstSize;
        return;
    }
    if (_messageRateBurstSize != burstSize) {
        _messageRateBurstSize = burstSize;
        emit messageRateBurstSizeSet(_messageRateBurstSize);
        emit configChanged();
    }
}

void Network::setMessageRateDelay(quint32 messageDelay)
{
    if (messageDelay == 0) {
        // Nonsensical to have no delay - just check the Unlimited box instead.  Also blocks old
        // clients from trying to set this to 0.
        qDebug() << "Received invalid setMessageRateDelay data - message delay must be non-zero "
                    "positive, given"
                 << messageDelay;
        return;
    }
    if (_messageRateDelay != messageDelay) {
        _messageRateDelay = messageDelay;
        emit messageRateDelaySet(_messageRateDelay);
        emit configChanged();
    }
}

void Network::setUnlimitedMessageRate(bool unlimitedRate)
{
    if (_unlimitedMessageRate != unlimitedRate) {
        _unlimitedMessageRate = unlimitedRate;
        emit unlimitedMessageRateSet(_unlimitedMessageRate);
        emit configChanged();
    }
}

void Network::addSupport(const QString& param, const QString& value)
{
    if (!_supports.contains(param)) {
        _supports[param] = value;
        SYNC(ARG(param), ARG(value))
    }
}

void Network::removeSupport(const QString& param)
{
    if (_supports.contains(param)) {
        _supports.remove(param);
        SYNC(ARG(param))
    }
}

QVariantMap Network::supportsToMap() const
{
    QVariantMap supports;
    QHashIterator<QString, QString> iter(_supports);
    while (iter.hasNext()) {
        iter.next();
        supports[iter.key()] = iter.value();
    }
    return supports;
}

void Network::addCap(const QString& capability, const QString& value)
{
    // IRCv3 specs all use lowercase capability names
    QString _capLowercase = capability.toLower();
    if (!_caps.contains(_capLowercase)) {
        _caps[_capLowercase] = value;
        SYNC(ARG(capability), ARG(value))
        emit capAdded(_capLowercase);
    }
}

void Network::acknowledgeCap(const QString& capability)
{
    // IRCv3 specs all use lowercase capability names
    QString _capLowercase = capability.toLower();
    if (!_capsEnabled.contains(_capLowercase)) {
        _capsEnabled.append(_capLowercase);
        SYNC(ARG(capability))
        emit capAcknowledged(_capLowercase);
    }
}

void Network::removeCap(const QString& capability)
{
    // IRCv3 specs all use lowercase capability names
    QString _capLowercase = capability.toLower();
    if (_caps.contains(_capLowercase)) {
        // Remove from the list of available capabilities.
        _caps.remove(_capLowercase);
        // Remove it from the acknowledged list if it was previously acknowledged.  The SYNC call
        // ensures this propogates to the other side.
        // Use removeOne() for speed; no more than one due to contains() check in acknowledgeCap().
        _capsEnabled.removeOne(_capLowercase);
        SYNC(ARG(capability))
        emit capRemoved(_capLowercase);
    }
}

void Network::clearCaps()
{
    // IRCv3 specs all use lowercase capability names
    if (_caps.empty() && _capsEnabled.empty()) {
        // Avoid the sync call if there's nothing to clear (e.g. failed reconnects)
        return;
    }
    // To ease core-side configuration, loop through the list and emit capRemoved for each entry.
    // If performance issues arise, this can be converted to a more-efficient setup without breaking
    // protocol (in theory).
    QString _capLowercase;
    foreach (const QString& capability, _caps) {
        _capLowercase = capability.toLower();
        emit capRemoved(_capLowercase);
    }
    // Clear capabilities from the stored list
    _caps.clear();
    _capsEnabled.clear();

    SYNC(NO_ARG)
}

QVariantMap Network::capsToMap() const
{
    QVariantMap caps;
    QHashIterator<QString, QString> iter(_caps);
    while (iter.hasNext()) {
        iter.next();
        caps[iter.key()] = iter.value();
    }
    return caps;
}

QVariantList Network::capsEnabledToList() const
{
    return toVariantList(capsEnabled());
}

void Network::setCapsEnabled(const QVariantList& capsEnabled)
{
    _capsEnabled = fromVariantList<QString>(capsEnabled);
}

QVariantList Network::serversToList() const
{
    return toVariantList(serverList());
}

void Network::setServerList(const QVariantList& serverList)
{
    _serverList = fromVariantList<Server>(serverList);
}

// There's potentially a lot of users and channels, so it makes sense to optimize the format of this.
// Rather than sending a thousand maps with identical keys, we convert this into one map containing lists
// where each list index corresponds to a particular IrcUser. This saves sending the key names a thousand times.
// Benchmarks have shown space savings of around 56%, resulting in saving several MBs worth of data on sync
// (without compression) with a decent amount of IrcUsers.
QVariantMap Network::ircUsersAndChannels() const
{
    Q_ASSERT(proxy());
    Q_ASSERT(proxy()->targetPeer());
    QVariantMap usersAndChannels;

    if (_ircUsers.count()) {
        QHash<QString, QVariantList> users;
        QHash<QString, IrcUser*>::const_iterator it = _ircUsers.begin();
        QHash<QString, IrcUser*>::const_iterator end = _ircUsers.end();
        while (it != end) {
            QVariantMap map = it.value()->toVariantMap();
            // If the peer doesn't support LongTime, replace the lastAwayMessageTime field
            // with the 32-bit numerical seconds value (lastAwayMessage) used in older versions
            if (!proxy()->targetPeer()->hasFeature(Quassel::Feature::LongTime)) {
#if QT_VERSION >= 0x050800
                int lastAwayMessage = it.value()->lastAwayMessageTime().toSecsSinceEpoch();
#else
                // toSecsSinceEpoch() was added in Qt 5.8.  Manually downconvert to seconds for now.
                // See https://doc.qt.io/qt-5/qdatetime.html#toMSecsSinceEpoch
                int lastAwayMessage = it.value()->lastAwayMessageTime().toMSecsSinceEpoch() / 1000;
#endif
                map.remove("lastAwayMessageTime");
                map["lastAwayMessage"] = lastAwayMessage;
            }

            QVariantMap::const_iterator mapiter = map.begin();
            while (mapiter != map.end()) {
                users[mapiter.key()] << mapiter.value();
                ++mapiter;
            }
            ++it;
        }
        // Can't have a container with a value type != QVariant in a QVariant :(
        // However, working directly on a QVariantMap is awkward for appending, thus the detour via the hash above.
        QVariantMap userMap;
        foreach (const QString& key, users.keys())
            userMap[key] = users[key];
        usersAndChannels["Users"] = userMap;
    }

    if (_ircChannels.count()) {
        QHash<QString, QVariantList> channels;
        QHash<QString, IrcChannel*>::const_iterator it = _ircChannels.begin();
        QHash<QString, IrcChannel*>::const_iterator end = _ircChannels.end();
        while (it != end) {
            const QVariantMap& map = it.value()->toVariantMap();
            QVariantMap::const_iterator mapiter = map.begin();
            while (mapiter != map.end()) {
                channels[mapiter.key()] << mapiter.value();
                ++mapiter;
            }
            ++it;
        }
        QVariantMap channelMap;
        foreach (const QString& key, channels.keys())
            channelMap[key] = channels[key];
        usersAndChannels["Channels"] = channelMap;
    }

    return usersAndChannels;
}

void Network::setIrcUsersAndChannels(const QVariantMap& usersAndChannels)
{
    Q_ASSERT(proxy());
    Q_ASSERT(proxy()->sourcePeer());
    if (isInitialized()) {
        qWarning() << "Network" << networkId()
                   << "received init data for users and channels although there already are known users or channels!";
        return;
    }

    // toMap() and toList() are cheap, so we can avoid copying to lists...
    // However, we really have to make sure to never accidentally detach from the shared data!

    const QVariantMap& users = usersAndChannels["Users"].toMap();

    // sanity check
    int count = users["nick"].toList().count();
    foreach (const QString& key, users.keys()) {
        if (users[key].toList().count() != count) {
            qWarning() << "Received invalid usersAndChannels init data, sizes of attribute lists don't match!";
            return;
        }
    }

    // now create the individual IrcUsers
    for (int i = 0; i < count; i++) {
        QVariantMap map;
        foreach (const QString& key, users.keys())
            map[key] = users[key].toList().at(i);

        // If the peer doesn't support LongTime, upconvert the lastAwayMessageTime field
        // from the 32-bit numerical seconds value used in older versions to QDateTime
        if (!proxy()->sourcePeer()->hasFeature(Quassel::Feature::LongTime)) {
            QDateTime lastAwayMessageTime = QDateTime();
            lastAwayMessageTime.setTimeSpec(Qt::UTC);
#if QT_VERSION >= 0x050800
            lastAwayMessageTime.fromSecsSinceEpoch(map.take("lastAwayMessage").toInt());
#else
            // toSecsSinceEpoch() was added in Qt 5.8.  Manually downconvert to seconds for now.
            // See https://doc.qt.io/qt-5/qdatetime.html#toMSecsSinceEpoch
            lastAwayMessageTime.fromMSecsSinceEpoch(map.take("lastAwayMessage").toInt() * 1000);
#endif
            map["lastAwayMessageTime"] = lastAwayMessageTime;
        }

        newIrcUser(map["nick"].toString(), map);  // newIrcUser() properly handles the hostmask being just the nick
    }

    // same thing for IrcChannels
    const QVariantMap& channels = usersAndChannels["Channels"].toMap();

    // sanity check
    count = channels["name"].toList().count();
    foreach (const QString& key, channels.keys()) {
        if (channels[key].toList().count() != count) {
            qWarning() << "Received invalid usersAndChannels init data, sizes of attribute lists don't match!";
            return;
        }
    }
    // now create the individual IrcChannels
    for (int i = 0; i < count; i++) {
        QVariantMap map;
        foreach (const QString& key, channels.keys())
            map[key] = channels[key].toList().at(i);
        newIrcChannel(map["name"].toString(), map);
    }
}

void Network::setSupports(const QVariantMap& supports)
{
    QMapIterator<QString, QVariant> iter(supports);
    while (iter.hasNext()) {
        iter.next();
        addSupport(iter.key(), iter.value().toString());
    }
}

void Network::setCaps(const QVariantMap& caps)
{
    QMapIterator<QString, QVariant> iter(caps);
    while (iter.hasNext()) {
        iter.next();
        addCap(iter.key(), iter.value().toString());
    }
}

IrcUser* Network::updateNickFromMask(const QString& mask)
{
    QString nick(nickFromMask(mask).toLower());
    IrcUser* ircuser;

    if (_ircUsers.contains(nick)) {
        ircuser = _ircUsers[nick];
        ircuser->updateHostmask(mask);
    }
    else {
        ircuser = newIrcUser(mask);
    }
    return ircuser;
}

void Network::ircUserNickChanged(QString newnick)
{
    QString oldnick = _ircUsers.key(qobject_cast<IrcUser*>(sender()));

    if (oldnick.isNull())
        return;

    if (newnick.toLower() != oldnick)
        _ircUsers[newnick.toLower()] = _ircUsers.take(oldnick);

    if (myNick().toLower() == oldnick)
        setMyNick(newnick);
}

void Network::emitConnectionError(const QString& errorMsg)
{
    emit connectionError(errorMsg);
}

// ====================
//  Private:
// ====================
void Network::determinePrefixes() const
{
    // seems like we have to construct them first
    QString prefix = support("PREFIX");

    if (prefix.startsWith("(") && prefix.contains(")")) {
        _prefixes = prefix.section(")", 1);
        _prefixModes = prefix.mid(1).section(")", 0, 0);
    }
    else {
        QString defaultPrefixes("~&@%+");
        QString defaultPrefixModes("qaohv");

        if (prefix.isEmpty()) {
            _prefixes = defaultPrefixes;
            _prefixModes = defaultPrefixModes;
            return;
        }
        // clear the existing modes, just in case we're run multiple times
        _prefixes = QString();
        _prefixModes = QString();

        // we just assume that in PREFIX are only prefix chars stored
        for (int i = 0; i < defaultPrefixes.size(); i++) {
            if (prefix.contains(defaultPrefixes[i])) {
                _prefixes += defaultPrefixes[i];
                _prefixModes += defaultPrefixModes[i];
            }
        }
        // check for success
        if (!_prefixes.isNull())
            return;

        // well... our assumption was obviously wrong...
        // check if it's only prefix modes
        for (int i = 0; i < defaultPrefixes.size(); i++) {
            if (prefix.contains(defaultPrefixModes[i])) {
                _prefixes += defaultPrefixes[i];
                _prefixModes += defaultPrefixModes[i];
            }
        }
        // now we've done all we've could...
    }
}

/************************************************************************
 * NetworkInfo
 ************************************************************************/

bool NetworkInfo::operator==(const NetworkInfo& other) const
{
    return     networkName               == other.networkName
            && serverList                == other.serverList
            && perform                   == other.perform
            && autoIdentifyService       == other.autoIdentifyService
            && autoIdentifyPassword      == other.autoIdentifyPassword
            && saslAccount               == other.saslAccount
            && saslPassword              == other.saslPassword
            && codecForServer            == other.codecForServer
            && codecForEncoding          == other.codecForEncoding
            && codecForDecoding          == other.codecForDecoding
            && networkId                 == other.networkId
            && identity                  == other.identity
            && messageRateBurstSize      == other.messageRateBurstSize
            && messageRateDelay          == other.messageRateDelay
            && autoReconnectInterval     == other.autoReconnectInterval
            && autoReconnectRetries      == other.autoReconnectRetries
            && rejoinChannels            == other.rejoinChannels
            && useRandomServer           == other.useRandomServer
            && useAutoIdentify           == other.useAutoIdentify
            && useSasl                   == other.useSasl
            && useAutoReconnect          == other.useAutoReconnect
            && unlimitedReconnectRetries == other.unlimitedReconnectRetries
            && useCustomMessageRate      == other.useCustomMessageRate
            && unlimitedMessageRate      == other.unlimitedMessageRate
        ;
}

bool NetworkInfo::operator!=(const NetworkInfo& other) const
{
    return !(*this == other);
}

QDataStream& operator<<(QDataStream& out, const NetworkInfo& info)
{
    QVariantMap i;
    i["NetworkName"]               = info.networkName;
    i["ServerList"]                = toVariantList(info.serverList);
    i["Perform"]                   = info.perform;
    i["AutoIdentifyService"]       = info.autoIdentifyService;
    i["AutoIdentifyPassword"]      = info.autoIdentifyPassword;
    i["SaslAccount"]               = info.saslAccount;
    i["SaslPassword"]              = info.saslPassword;
    i["CodecForServer"]            = info.codecForServer;
    i["CodecForEncoding"]          = info.codecForEncoding;
    i["CodecForDecoding"]          = info.codecForDecoding;
    i["NetworkId"]                 = QVariant::fromValue<NetworkId>(info.networkId);
    i["Identity"]                  = QVariant::fromValue<IdentityId>(info.identity);
    i["MessageRateBurstSize"]      = info.messageRateBurstSize;
    i["MessageRateDelay"]          = info.messageRateDelay;
    i["AutoReconnectInterval"]     = info.autoReconnectInterval;
    i["AutoReconnectRetries"]      = info.autoReconnectRetries;
    i["RejoinChannels"]            = info.rejoinChannels;
    i["UseRandomServer"]           = info.useRandomServer;
    i["UseAutoIdentify"]           = info.useAutoIdentify;
    i["UseSasl"]                   = info.useSasl;
    i["UseAutoReconnect"]          = info.useAutoReconnect;
    i["UnlimitedReconnectRetries"] = info.unlimitedReconnectRetries;
    i["UseCustomMessageRate"]      = info.useCustomMessageRate;
    i["UnlimitedMessageRate"]      = info.unlimitedMessageRate;
    out << i;
    return out;
}

QDataStream& operator>>(QDataStream& in, NetworkInfo& info)
{
    QVariantMap i;
    in >> i;
    info.networkName               = i["NetworkName"].toString();
    info.serverList                = fromVariantList<Network::Server>(i["ServerList"].toList());
    info.perform                   = i["Perform"].toStringList();
    info.autoIdentifyService       = i["AutoIdentifyService"].toString();
    info.autoIdentifyPassword      = i["AutoIdentifyPassword"].toString();
    info.saslAccount               = i["SaslAccount"].toString();
    info.saslPassword              = i["SaslPassword"].toString();
    info.codecForServer            = i["CodecForServer"].toByteArray();
    info.codecForEncoding          = i["CodecForEncoding"].toByteArray();
    info.codecForDecoding          = i["CodecForDecoding"].toByteArray();
    info.networkId                 = i["NetworkId"].value<NetworkId>();
    info.identity                  = i["Identity"].value<IdentityId>();
    info.messageRateBurstSize      = i["MessageRateBurstSize"].toUInt();
    info.messageRateDelay          = i["MessageRateDelay"].toUInt();
    info.autoReconnectInterval     = i["AutoReconnectInterval"].toUInt();
    info.autoReconnectRetries      = i["AutoReconnectRetries"].toInt();
    info.rejoinChannels            = i["RejoinChannels"].toBool();
    info.useRandomServer           = i["UseRandomServer"].toBool();
    info.useAutoIdentify           = i["UseAutoIdentify"].toBool();
    info.useSasl                   = i["UseSasl"].toBool();
    info.useAutoReconnect          = i["UseAutoReconnect"].toBool();
    info.unlimitedReconnectRetries = i["UnlimitedReconnectRetries"].toBool();
    info.useCustomMessageRate      = i["UseCustomMessageRate"].toBool();
    info.unlimitedMessageRate      = i["UnlimitedMessageRate"].toBool();
    return in;
}

QDebug operator<<(QDebug dbg, const NetworkInfo& i)
{
    dbg.nospace() << "(id = " << i.networkId << " name = " << i.networkName << " identity = " << i.identity
                  << " codecForServer = " << i.codecForServer << " codecForEncoding = " << i.codecForEncoding
                  << " codecForDecoding = " << i.codecForDecoding << " serverList = " << i.serverList
                  << " useRandomServer = " << i.useRandomServer << " perform = " << i.perform << " useAutoIdentify = " << i.useAutoIdentify
                  << " autoIdentifyService = " << i.autoIdentifyService << " autoIdentifyPassword = " << i.autoIdentifyPassword
                  << " useSasl = " << i.useSasl << " saslAccount = " << i.saslAccount << " saslPassword = " << i.saslPassword
                  << " useAutoReconnect = " << i.useAutoReconnect << " autoReconnectInterval = " << i.autoReconnectInterval
                  << " autoReconnectRetries = " << i.autoReconnectRetries << " unlimitedReconnectRetries = " << i.unlimitedReconnectRetries
                  << " rejoinChannels = " << i.rejoinChannels << " useCustomMessageRate = " << i.useCustomMessageRate
                  << " messageRateBurstSize = " << i.messageRateBurstSize << " messageRateDelay = " << i.messageRateDelay
                  << " unlimitedMessageRate = " << i.unlimitedMessageRate << ")";
    return dbg.space();
}

QDataStream& operator<<(QDataStream& out, const Network::Server& server)
{
    QVariantMap serverMap;
    serverMap["Host"] = server.host;
    serverMap["Port"] = server.port;
    serverMap["Password"] = server.password;
    serverMap["UseSSL"] = server.useSsl;
    serverMap["sslVerify"] = server.sslVerify;
    serverMap["sslVersion"] = server.sslVersion;
    serverMap["UseProxy"] = server.useProxy;
    serverMap["ProxyType"] = server.proxyType;
    serverMap["ProxyHost"] = server.proxyHost;
    serverMap["ProxyPort"] = server.proxyPort;
    serverMap["ProxyUser"] = server.proxyUser;
    serverMap["ProxyPass"] = server.proxyPass;
    out << serverMap;
    return out;
}

QDataStream& operator>>(QDataStream& in, Network::Server& server)
{
    QVariantMap serverMap;
    in >> serverMap;
    server.host = serverMap["Host"].toString();
    server.port = serverMap["Port"].toUInt();
    server.password = serverMap["Password"].toString();
    server.useSsl = serverMap["UseSSL"].toBool();
    server.sslVerify = serverMap["sslVerify"].toBool();
    server.sslVersion = serverMap["sslVersion"].toInt();
    server.useProxy = serverMap["UseProxy"].toBool();
    server.proxyType = serverMap["ProxyType"].toInt();
    server.proxyHost = serverMap["ProxyHost"].toString();
    server.proxyPort = serverMap["ProxyPort"].toUInt();
    server.proxyUser = serverMap["ProxyUser"].toString();
    server.proxyPass = serverMap["ProxyPass"].toString();
    return in;
}

bool Network::Server::operator==(const Server& other) const
{
    if (host != other.host)
        return false;
    if (port != other.port)
        return false;
    if (password != other.password)
        return false;
    if (useSsl != other.useSsl)
        return false;
    if (sslVerify != other.sslVerify)
        return false;
    if (sslVersion != other.sslVersion)
        return false;
    if (useProxy != other.useProxy)
        return false;
    if (proxyType != other.proxyType)
        return false;
    if (proxyHost != other.proxyHost)
        return false;
    if (proxyPort != other.proxyPort)
        return false;
    if (proxyUser != other.proxyUser)
        return false;
    if (proxyPass != other.proxyPass)
        return false;
    return true;
}

bool Network::Server::operator!=(const Server& other) const
{
    return !(*this == other);
}

QDebug operator<<(QDebug dbg, const Network::Server& server)
{
    dbg.nospace() << "Server(host = " << server.host << ":" << server.port << ", useSsl = " << server.useSsl
                  << ", sslVerify = " << server.sslVerify << ")";
    return dbg.space();
}

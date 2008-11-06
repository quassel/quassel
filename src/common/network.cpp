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
#include "network.h"

#include <QDebug>
#include <QTextCodec>

#include "util.h"

QTextCodec *Network::_defaultCodecForServer = 0;
QTextCodec *Network::_defaultCodecForEncoding = 0;
QTextCodec *Network::_defaultCodecForDecoding = 0;
// ====================
//  Public:
// ====================
Network::Network(const NetworkId &networkid, QObject *parent)
  : SyncableObject(parent),
    _proxy(0),
    _networkId(networkid),
    _identity(0),
    _myNick(QString()),
    _latency(0),
    _networkName(QString("<not initialized>")),
    _currentServer(QString()),
    _connected(false),
    _connectionState(Disconnected),
    _prefixes(QString()),
    _prefixModes(QString()),
    _useRandomServer(false),
    _useAutoIdentify(false),
    _useAutoReconnect(false),
    _autoReconnectInterval(60),
    _autoReconnectRetries(10),
    _unlimitedReconnectRetries(false),
    _codecForServer(0),
    _codecForEncoding(0),
    _codecForDecoding(0),
    _autoAwayActive(false)
{
  setObjectName(QString::number(networkid.toInt()));
}

Network::~Network() {
  emit aboutToBeDestroyed();
}

bool Network::isChannelName(const QString &channelname) const {
  if(channelname.isEmpty())
    return false;

  if(supports("CHANTYPES"))
    return support("CHANTYPES").contains(channelname[0]);
  else
    return QString("#&!+").contains(channelname[0]);
}

NetworkInfo Network::networkInfo() const {
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
  info.useAutoReconnect = useAutoReconnect();
  info.autoReconnectInterval = autoReconnectInterval();
  info.autoReconnectRetries = autoReconnectRetries();
  info.unlimitedReconnectRetries = unlimitedReconnectRetries();
  info.rejoinChannels = rejoinChannels();
  return info;
}

void Network::setNetworkInfo(const NetworkInfo &info) {
  // we don't set our ID!
  if(!info.networkName.isEmpty() && info.networkName != networkName()) setNetworkName(info.networkName);
  if(info.identity > 0 && info.identity != identity()) setIdentity(info.identity);
  if(info.codecForServer != codecForServer()) setCodecForServer(QTextCodec::codecForName(info.codecForServer));
  if(info.codecForEncoding != codecForEncoding()) setCodecForEncoding(QTextCodec::codecForName(info.codecForEncoding));
  if(info.codecForDecoding != codecForDecoding()) setCodecForDecoding(QTextCodec::codecForName(info.codecForDecoding));
  if(info.serverList.count()) setServerList(info.serverList); // FIXME compare components
  if(info.useRandomServer != useRandomServer()) setUseRandomServer(info.useRandomServer);
  if(info.perform != perform()) setPerform(info.perform);
  if(info.useAutoIdentify != useAutoIdentify()) setUseAutoIdentify(info.useAutoIdentify);
  if(info.autoIdentifyService != autoIdentifyService()) setAutoIdentifyService(info.autoIdentifyService);
  if(info.autoIdentifyPassword != autoIdentifyPassword()) setAutoIdentifyPassword(info.autoIdentifyPassword);
  if(info.useAutoReconnect != useAutoReconnect()) setUseAutoReconnect(info.useAutoReconnect);
  if(info.autoReconnectInterval != autoReconnectInterval()) setAutoReconnectInterval(info.autoReconnectInterval);
  if(info.autoReconnectRetries != autoReconnectRetries()) setAutoReconnectRetries(info.autoReconnectRetries);
  if(info.unlimitedReconnectRetries != unlimitedReconnectRetries()) setUnlimitedReconnectRetries(info.unlimitedReconnectRetries);
  if(info.rejoinChannels != rejoinChannels()) setRejoinChannels(info.rejoinChannels);
}

QString Network::prefixToMode(const QString &prefix) {
  if(prefixes().contains(prefix))
    return QString(prefixModes()[prefixes().indexOf(prefix)]);
  else
    return QString();
}

QString Network::modeToPrefix(const QString &mode) {
  if(prefixModes().contains(mode))
    return QString(prefixes()[prefixModes().indexOf(mode)]);
  else
    return QString();
}

QStringList Network::nicks() const {
  // we don't use _ircUsers.keys() since the keys may be
  // not up to date after a nick change
  QStringList nicks;
  foreach(IrcUser *ircuser, _ircUsers.values()) {
    nicks << ircuser->nick();
  }
  return nicks;
}

QString Network::prefixes() {
  if(_prefixes.isNull())
    determinePrefixes();

  return _prefixes;
}

QString Network::prefixModes() {
  if(_prefixModes.isNull())
    determinePrefixes();

  return _prefixModes;
}

// example Unreal IRCD: CHANMODES=beI,kfL,lj,psmntirRcOAQKVCuzNSMTG
Network::ChannelModeType Network::channelModeType(const QString &mode) {
  if(mode.isEmpty())
    return NOT_A_CHANMODE;

  QString chanmodes = support("CHANMODES");
  if(chanmodes.isEmpty())
    return NOT_A_CHANMODE;

  ChannelModeType modeType = A_CHANMODE;
  for(int i = 0; i < chanmodes.count(); i++) {
    if(chanmodes[i] == mode[0])
      break;
    else if(chanmodes[i] == ',')
      modeType = (ChannelModeType)(modeType << 1);
  }
  if(modeType > D_CHANMODE) {
    qWarning() << "Network" << networkId() << "supplied invalid CHANMODES:" << chanmodes;
    modeType = NOT_A_CHANMODE;
  }
  return modeType;
}

QString Network::support(const QString &param) const {
  QString support_ = param.toUpper();
  if(_supports.contains(support_))
    return _supports[support_];
  else
    return QString();
}

IrcUser *Network::newIrcUser(const QString &hostmask, const QVariantMap &initData) {
  QString nick(nickFromMask(hostmask).toLower());
  if(!_ircUsers.contains(nick)) {
    IrcUser *ircuser = new IrcUser(hostmask, this);
    if(initData.isEmpty()) {
      ircuser->fromVariantMap(initData);
      ircuser->setInitialized();
    }

    if(proxy())
      proxy()->synchronize(ircuser);
    else
      qWarning() << "unable to synchronize new IrcUser" << hostmask << "forgot to call Network::setProxy(SignalProxy *)?";

    connect(ircuser, SIGNAL(nickSet(QString)), this, SLOT(ircUserNickChanged(QString)));
    connect(ircuser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
    if(!ircuser->isInitialized())
      connect(ircuser, SIGNAL(initDone()), this, SLOT(ircUserInitDone()));

    _ircUsers[nick] = ircuser;

    emit ircUserAdded(hostmask);
    emit ircUserAdded(ircuser);
    if(ircuser->isInitialized())
      emit ircUserInitDone(ircuser);
  }

  return _ircUsers[nick];
}

void Network::ircUserDestroyed() {
  IrcUser *ircUser = static_cast<IrcUser *>(sender());
  if(!ircUser)
    return;

  QHash<QString, IrcUser *>::iterator ircUserIter = _ircUsers.begin();
  while(ircUserIter != _ircUsers.end()) {
    if(ircUser == *ircUserIter) {
      ircUserIter = _ircUsers.erase(ircUserIter);
      break;
    }
    ircUserIter++;
  }
}

void Network::removeIrcUser(IrcUser *ircuser) {
  QString nick = _ircUsers.key(ircuser);
  if(nick.isNull())
    return;

  _ircUsers.remove(nick);
  disconnect(ircuser, 0, this, 0);
  emit ircUserRemoved(nick);
  emit ircUserRemoved(ircuser);
  ircuser->deleteLater();
}

void Network::removeIrcUser(const QString &nick) {
  IrcUser *ircuser;
  if((ircuser = ircUser(nick)) != 0)
    removeIrcUser(ircuser);
}

void Network::removeChansAndUsers() {
  QList<IrcUser *> users = ircUsers();
  foreach(IrcUser *user, users) {
    removeIrcUser(user);
  }
  QList<IrcChannel *> channels = ircChannels();
  foreach(IrcChannel *channel, channels) {
    removeIrcChannel(channel);
  }
}

IrcUser *Network::ircUser(QString nickname) const {
  nickname = nickname.toLower();
  if(_ircUsers.contains(nickname))
    return _ircUsers[nickname];
  else
    return 0;
}

IrcChannel *Network::newIrcChannel(const QString &channelname, const QVariantMap &initData) {
  if(!_ircChannels.contains(channelname.toLower())) {
    IrcChannel *channel = ircChannelFactory(channelname);
    if(initData.isEmpty()) {
      channel->fromVariantMap(initData);
      channel->setInitialized();
    }

    if(proxy())
      proxy()->synchronize(channel);
    else
      qWarning() << "unable to synchronize new IrcChannel" << channelname << "forgot to call Network::setProxy(SignalProxy *)?";

    connect(channel, SIGNAL(destroyed()), this, SLOT(channelDestroyed()));
    if(!channel->isInitialized())
      connect(channel, SIGNAL(initDone()), this, SLOT(ircChannelInitDone()));

    _ircChannels[channelname.toLower()] = channel;

    emit ircChannelAdded(channelname);
    emit ircChannelAdded(channel);
    if(channel->isInitialized())
      emit ircChannelInitDone(channel);
  }
  return _ircChannels[channelname.toLower()];
}

IrcChannel *Network::ircChannel(QString channelname) const {
  channelname = channelname.toLower();
  if(_ircChannels.contains(channelname))
    return _ircChannels[channelname];
  else
    return 0;
}

QByteArray Network::defaultCodecForServer() {
  if(_defaultCodecForServer)
    return _defaultCodecForServer->name();
  return QByteArray();
}

void Network::setDefaultCodecForServer(const QByteArray &name) {
  _defaultCodecForServer = QTextCodec::codecForName(name);
}

QByteArray Network::defaultCodecForEncoding() {
  if(_defaultCodecForEncoding)
    return _defaultCodecForEncoding->name();
  return QByteArray();
}

void Network::setDefaultCodecForEncoding(const QByteArray &name) {
  _defaultCodecForEncoding = QTextCodec::codecForName(name);
}

QByteArray Network::defaultCodecForDecoding() {
  if(_defaultCodecForDecoding)
    return _defaultCodecForDecoding->name();
  return QByteArray();
}

void Network::setDefaultCodecForDecoding(const QByteArray &name) {
  _defaultCodecForDecoding = QTextCodec::codecForName(name);
}

QByteArray Network::codecForServer() const {
  if(_codecForServer)
    return _codecForServer->name();
  return QByteArray();
}

void Network::setCodecForServer(const QByteArray &name) {
  setCodecForServer(QTextCodec::codecForName(name));
}

void Network::setCodecForServer(QTextCodec *codec) {
  _codecForServer = codec;
  emit codecForServerSet(codecForServer());
}

QByteArray Network::codecForEncoding() const {
  if(_codecForEncoding)
    return _codecForEncoding->name();
  return QByteArray();
}

void Network::setCodecForEncoding(const QByteArray &name) {
  setCodecForEncoding(QTextCodec::codecForName(name));
}

void Network::setCodecForEncoding(QTextCodec *codec) {
  _codecForEncoding = codec;
  emit codecForEncodingSet(codecForEncoding());
}

QByteArray Network::codecForDecoding() const {
  if(_codecForDecoding)
    return _codecForDecoding->name();
  else return QByteArray();
}

void Network::setCodecForDecoding(const QByteArray &name) {
  setCodecForDecoding(QTextCodec::codecForName(name));
}

void Network::setCodecForDecoding(QTextCodec *codec) {
  _codecForDecoding = codec;
  emit codecForDecodingSet(codecForDecoding());
}

// FIXME use server encoding if appropriate
QString Network::decodeString(const QByteArray &text) const {
  if(_codecForDecoding)
    return ::decodeString(text, _codecForDecoding);
  else return ::decodeString(text, _defaultCodecForDecoding);
}

QByteArray Network::encodeString(const QString &string) const {
  if(_codecForEncoding) {
    return _codecForEncoding->fromUnicode(string);
  }
  if(_defaultCodecForEncoding) {
    return _defaultCodecForEncoding->fromUnicode(string);
  }
  return string.toAscii();
}

QString Network::decodeServerString(const QByteArray &text) const {
  if(_codecForServer)
    return ::decodeString(text, _codecForServer);
  else
    return ::decodeString(text, _defaultCodecForServer);
}

QByteArray Network::encodeServerString(const QString &string) const {
  if(_codecForServer) {
    return _codecForServer->fromUnicode(string);
  }
  if(_defaultCodecForServer) {
    return _defaultCodecForServer->fromUnicode(string);
  }
  return string.toAscii();
}

// ====================
//  Public Slots:
// ====================
void Network::setNetworkName(const QString &networkName) {
  _networkName = networkName;
  emit networkNameSet(networkName);
}

void Network::setCurrentServer(const QString &currentServer) {
  _currentServer = currentServer;
  emit currentServerSet(currentServer);
}

void Network::setConnected(bool connected) {
  if(_connected == connected)
    return;

  _connected = connected;
  if(!connected) {
    setMyNick(QString());
    setCurrentServer(QString());
    removeChansAndUsers();
  }
  emit connectedSet(connected);
}

//void Network::setConnectionState(ConnectionState state) {
void Network::setConnectionState(int state) {
  _connectionState = (ConnectionState)state;
  //qDebug() << "netstate" << networkId() << networkName() << state;
  emit connectionStateSet(state);
  emit connectionStateSet(_connectionState);
}

void Network::setMyNick(const QString &nickname) {
  _myNick = nickname;
  if(!_myNick.isEmpty() && !ircUser(myNick())) {
    newIrcUser(myNick());
  }
  emit myNickSet(nickname);
}

void Network::setLatency(int latency) {
  if(_latency == latency)
    return;
  _latency = latency;
  emit latencySet(latency);
}

void Network::setIdentity(IdentityId id) {
  _identity = id;
  emit identitySet(id);
}

void Network::setServerList(const QVariantList &serverList) {
  _serverList = serverList;
  emit serverListSet(serverList);
}

void Network::setUseRandomServer(bool use) {
  _useRandomServer = use;
  emit useRandomServerSet(use);
}

void Network::setPerform(const QStringList &perform) {
  _perform = perform;
  emit performSet(perform);
}

void Network::setUseAutoIdentify(bool use) {
  _useAutoIdentify = use;
  emit useAutoIdentifySet(use);
}

void Network::setAutoIdentifyService(const QString &service) {
  _autoIdentifyService = service;
  emit autoIdentifyServiceSet(service);
}

void Network::setAutoIdentifyPassword(const QString &password) {
  _autoIdentifyPassword = password;
  emit autoIdentifyPasswordSet(password);
}

void Network::setUseAutoReconnect(bool use) {
  _useAutoReconnect = use;
  emit useAutoReconnectSet(use);
}

void Network::setAutoReconnectInterval(quint32 interval) {
  _autoReconnectInterval = interval;
  emit autoReconnectIntervalSet(interval);
}

void Network::setAutoReconnectRetries(quint16 retries) {
  _autoReconnectRetries = retries;
  emit autoReconnectRetriesSet(retries);
}

void Network::setUnlimitedReconnectRetries(bool unlimited) {
  _unlimitedReconnectRetries = unlimited;
  emit unlimitedReconnectRetriesSet(unlimited);
}

void Network::setRejoinChannels(bool rejoin) {
  _rejoinChannels = rejoin;
  emit rejoinChannelsSet(rejoin);
}

void Network::addSupport(const QString &param, const QString &value) {
  if(!_supports.contains(param)) {
    _supports[param] = value;
    emit supportAdded(param, value);
  }
}

void Network::removeSupport(const QString &param) {
  if(_supports.contains(param)) {
    _supports.remove(param);
    emit supportRemoved(param);
  }
}

QVariantMap Network::initSupports() const {
  QVariantMap supports;
  QHashIterator<QString, QString> iter(_supports);
  while(iter.hasNext()) {
    iter.next();
    supports[iter.key()] = iter.value();
  }
  return supports;
}

QVariantMap Network::initIrcUsersAndChannels() const {
  QVariantMap usersAndChannels;
  QVariantMap users;
  QVariantMap channels;

  QHash<QString, IrcUser *>::const_iterator userIter = _ircUsers.constBegin();
  QHash<QString, IrcUser *>::const_iterator userIterEnd = _ircUsers.constEnd();
  while(userIter != userIterEnd) {
    users[userIter.value()->hostmask()] = userIter.value()->toVariantMap();
    userIter++;
  }
  usersAndChannels["users"] = users;

  QHash<QString, IrcChannel *>::const_iterator channelIter = _ircChannels.constBegin();
  QHash<QString, IrcChannel *>::const_iterator channelIterEnd = _ircChannels.constEnd();
  while(channelIter != channelIterEnd) {
    channels[channelIter.value()->name()] = channelIter.value()->toVariantMap();
    channelIter++;
  }
  usersAndChannels["channels"] = channels;

  return usersAndChannels;
}

void Network::initSetIrcUsersAndChannels(const QVariantMap &usersAndChannels) {
  Q_ASSERT(proxy());
  if(isInitialized()) {
    qWarning() << "Network" << networkId() << "received init data for users and channels allthough there allready are known users or channels!";
    return;
  }

  QVariantMap users = usersAndChannels.value("users").toMap();
  QVariantMap::const_iterator userIter = users.constBegin();
  QVariantMap::const_iterator userIterEnd = users.constEnd();
  while(userIter != userIterEnd) {
    newIrcUser(userIter.key(), userIter.value().toMap());
    userIter++;
  }

  QVariantMap channels = usersAndChannels.value("channels").toMap();
  QVariantMap::const_iterator channelIter = channels.constBegin();
  QVariantMap::const_iterator channelIterEnd = channels.constEnd();
  while(channelIter != channelIterEnd) {
    newIrcChannel(channelIter.key(), channelIter.value().toMap());
    channelIter++;
  }

}

void Network::initSetSupports(const QVariantMap &supports) {
  QMapIterator<QString, QVariant> iter(supports);
  while(iter.hasNext()) {
    iter.next();
    addSupport(iter.key(), iter.value().toString());
  }
}

IrcUser *Network::updateNickFromMask(const QString &mask) {
  QString nick(nickFromMask(mask).toLower());
  IrcUser *ircuser;

  if(_ircUsers.contains(nick)) {
    ircuser = _ircUsers[nick];
    ircuser->updateHostmask(mask);
  } else {
    ircuser = newIrcUser(mask);
  }
  return ircuser;
}

void Network::ircUserNickChanged(QString newnick) {
  QString oldnick = _ircUsers.key(qobject_cast<IrcUser*>(sender()));

  if(oldnick.isNull())
    return;

  if(newnick.toLower() != oldnick) _ircUsers[newnick.toLower()] = _ircUsers.take(oldnick);

  if(myNick().toLower() == oldnick)
    setMyNick(newnick);
}

void Network::ircUserInitDone() {
  IrcUser *ircuser = static_cast<IrcUser *>(sender());
  Q_ASSERT(ircuser);
  connect(ircuser, SIGNAL(initDone()), this, SLOT(ircUserInitDone()));
  emit ircUserInitDone(ircuser);
}

void Network::ircChannelInitDone() {
  IrcChannel *ircChannel = static_cast<IrcChannel *>(sender());
  Q_ASSERT(ircChannel);
  disconnect(ircChannel, SIGNAL(initDone()), this, SLOT(ircChannelInitDone()));
  emit ircChannelInitDone(ircChannel);
}

void Network::removeIrcChannel(IrcChannel *channel) {
  QString chanName = _ircChannels.key(channel);
  if(chanName.isNull())
    return;

  _ircChannels.remove(chanName);
  disconnect(channel, 0, this, 0);
  emit ircChannelRemoved(chanName);
  emit ircChannelRemoved(channel);
  channel->deleteLater();
}

void Network::removeIrcChannel(const QString &channel) {
  IrcChannel *chan;
  if((chan = ircChannel(channel)) != 0)
    removeIrcChannel(chan);
}

void Network::channelDestroyed() {
  IrcChannel *channel = static_cast<IrcChannel *>(sender());
  Q_ASSERT(channel);
  _ircChannels.remove(_ircChannels.key(channel));
  emit ircChannelRemoved(channel);
}

void Network::emitConnectionError(const QString &errorMsg) {
  emit connectionError(errorMsg);
}

// ====================
//  Private:
// ====================
void Network::determinePrefixes() {
  // seems like we have to construct them first
  QString prefix = support("PREFIX");

  if(prefix.startsWith("(") && prefix.contains(")")) {
    _prefixes = prefix.section(")", 1);
    _prefixModes = prefix.mid(1).section(")", 0, 0);
  } else {
    QString defaultPrefixes("~&@%+");
    QString defaultPrefixModes("qaohv");

    if(prefix.isEmpty()) {
      _prefixes = defaultPrefixes;
      _prefixModes = defaultPrefixModes;
      return;
    }

    // we just assume that in PREFIX are only prefix chars stored
    for(int i = 0; i < defaultPrefixes.size(); i++) {
      if(prefix.contains(defaultPrefixes[i])) {
	_prefixes += defaultPrefixes[i];
	_prefixModes += defaultPrefixModes[i];
      }
    }
    // check for success
    if(!_prefixes.isNull())
      return;

    // well... our assumption was obviously wrong...
    // check if it's only prefix modes
    for(int i = 0; i < defaultPrefixes.size(); i++) {
      if(prefix.contains(defaultPrefixModes[i])) {
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

bool NetworkInfo::operator==(const NetworkInfo &other) const {
  if(networkId != other.networkId) return false;
  if(networkName != other.networkName) return false;
  if(identity != other.identity) return false;
  if(codecForServer != other.codecForServer) return false;
  if(codecForEncoding != other.codecForEncoding) return false;
  if(codecForDecoding != other.codecForDecoding) return false;
  if(serverList != other.serverList) return false;
  if(useRandomServer != other.useRandomServer) return false;
  if(perform != other.perform) return false;
  if(useAutoIdentify != other.useAutoIdentify) return false;
  if(autoIdentifyService != other.autoIdentifyService) return false;
  if(autoIdentifyPassword != other.autoIdentifyPassword) return false;
  if(useAutoReconnect != other.useAutoReconnect) return false;
  if(autoReconnectInterval != other.autoReconnectInterval) return false;
  if(autoReconnectRetries != other.autoReconnectRetries) return false;
  if(unlimitedReconnectRetries != other.unlimitedReconnectRetries) return false;
  if(rejoinChannels != other.rejoinChannels) return false;
  return true;
}

bool NetworkInfo::operator!=(const NetworkInfo &other) const {
  return !(*this == other);
}

QDataStream &operator<<(QDataStream &out, const NetworkInfo &info) {
  QVariantMap i;
  i["NetworkId"] = QVariant::fromValue<NetworkId>(info.networkId);
  i["NetworkName"] = info.networkName;
  i["Identity"] = QVariant::fromValue<IdentityId>(info.identity);
  i["CodecForServer"] = info.codecForServer;
  i["CodecForEncoding"] = info.codecForEncoding;
  i["CodecForDecoding"] = info.codecForDecoding;
  i["ServerList"] = info.serverList;
  i["UseRandomServer"] = info.useRandomServer;
  i["Perform"] = info.perform;
  i["UseAutoIdentify"] = info.useAutoIdentify;
  i["AutoIdentifyService"] = info.autoIdentifyService;
  i["AutoIdentifyPassword"] = info.autoIdentifyPassword;
  i["UseAutoReconnect"] = info.useAutoReconnect;
  i["AutoReconnectInterval"] = info.autoReconnectInterval;
  i["AutoReconnectRetries"] = info.autoReconnectRetries;
  i["UnlimitedReconnectRetries"] = info.unlimitedReconnectRetries;
  i["RejoinChannels"] = info.rejoinChannels;
  out << i;
  return out;
}

QDataStream &operator>>(QDataStream &in, NetworkInfo &info) {
  QVariantMap i;
  in >> i;
  info.networkId = i["NetworkId"].value<NetworkId>();
  info.networkName = i["NetworkName"].toString();
  info.identity = i["Identity"].value<IdentityId>();
  info.codecForServer = i["CodecForServer"].toByteArray();
  info.codecForEncoding = i["CodecForEncoding"].toByteArray();
  info.codecForDecoding = i["CodecForDecoding"].toByteArray();
  info.serverList = i["ServerList"].toList();
  info.useRandomServer = i["UseRandomServer"].toBool();
  info.perform = i["Perform"].toStringList();
  info.useAutoIdentify = i["UseAutoIdentify"].toBool();
  info.autoIdentifyService = i["AutoIdentifyService"].toString();
  info.autoIdentifyPassword = i["AutoIdentifyPassword"].toString();
  info.useAutoReconnect = i["UseAutoReconnect"].toBool();
  info.autoReconnectInterval = i["AutoReconnectInterval"].toUInt();
  info.autoReconnectRetries = i["AutoReconnectRetries"].toInt();
  info.unlimitedReconnectRetries = i["UnlimitedReconnectRetries"].toBool();
  info.rejoinChannels = i["RejoinChannels"].toBool();
  return in;
}

QDebug operator<<(QDebug dbg, const NetworkInfo &i) {
  dbg.nospace() << "(id = " << i.networkId << " name = " << i.networkName << " identity = " << i.identity
      << " codecForServer = " << i.codecForServer << " codecForEncoding = " << i.codecForEncoding << " codecForDecoding = " << i.codecForDecoding
      << " serverList = " << i.serverList << " useRandomServer = " << i.useRandomServer << " perform = " << i.perform
      << " useAutoIdentify = " << i.useAutoIdentify << " autoIdentifyService = " << i.autoIdentifyService << " autoIdentifyPassword = " << i.autoIdentifyPassword
      << " useAutoReconnect = " << i.useAutoReconnect << " autoReconnectInterval = " << i.autoReconnectInterval
      << " autoReconnectRetries = " << i.autoReconnectRetries << " unlimitedReconnectRetries = " << i.unlimitedReconnectRetries
      << " rejoinChannels = " << i.rejoinChannels << ")";
  return dbg.space();
}

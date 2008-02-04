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

#include "signalproxy.h"
#include "ircuser.h"
#include "ircchannel.h"

#include <QDebug>
#include <QTextCodec>

#include "util.h"

// ====================
//  Public:
// ====================
Network::Network(const NetworkId &networkid, QObject *parent) : SyncableObject(parent),
    _networkId(networkid),
    _identity(0),
    _myNick(QString()),
    _networkName(QString("<not initialized>")),
    _currentServer(QString()),
    _connected(false),
    _connectionState(Disconnected),
    _prefixes(QString()),
    _prefixModes(QString()),
    _proxy(0),
    _codecForEncoding(0),
    _codecForDecoding(0)
{
  setObjectName(QString::number(networkid.toInt()));
}

// I think this is unnecessary since IrcUsers have us as their daddy :)

Network::~Network() {
//  QHashIterator<QString, IrcUser *> ircuser(_ircUsers);
//  while (ircuser.hasNext()) {
//    ircuser.next();
//    delete ircuser.value();
//  }
//  qDebug() << "Destroying net" << networkName() << networkId();
}


NetworkId Network::networkId() const {
  return _networkId;
}

SignalProxy *Network::proxy() const {
  return _proxy;
}

void Network::setProxy(SignalProxy *proxy) {
  _proxy = proxy;
  //proxy->synchronize(this);  // we should to this explicitly from the outside!
}

bool Network::isMyNick(const QString &nick) const {
  return (myNick().toLower() == nick.toLower());
}

bool Network::isMe(IrcUser *ircuser) const {
  return (ircuser->nick().toLower() == myNick().toLower());
}

bool Network::isChannelName(const QString &channelname) const {
  if(channelname.isEmpty())
    return false;
  
  if(supports("CHANTYPES"))
    return support("CHANTYPES").contains(channelname[0]);
  else
    return QString("#&!+").contains(channelname[0]);
}

bool Network::isConnected() const {
  return _connected;
}

//Network::ConnectionState Network::connectionState() const {
int Network::connectionState() const {
  return _connectionState;
}

NetworkInfo Network::networkInfo() const {
  NetworkInfo info;
  info.networkName = networkName();
  info.networkId = networkId();
  info.identity = identity();
  info.codecForEncoding = codecForEncoding();
  info.codecForDecoding = codecForDecoding();
  info.serverList = serverList();
  return info;
}

void Network::setNetworkInfo(const NetworkInfo &info) {
  // we don't set our ID!
  if(!info.networkName.isEmpty() && info.networkName != networkName()) setNetworkName(info.networkName);
  if(info.identity > 0 && info.identity != identity()) setIdentity(info.identity);
  if(!info.codecForEncoding.isEmpty() && info.codecForEncoding != codecForEncoding())
    setCodecForEncoding(QTextCodec::codecForName(info.codecForEncoding));
  if(!info.codecForDecoding.isEmpty() && info.codecForDecoding != codecForDecoding())
    setCodecForDecoding(QTextCodec::codecForName(info.codecForDecoding));
  if(info.serverList.count()) setServerList(info.serverList); // FIXME compare components
}

QString Network::prefixToMode(const QString &prefix) {
  if(prefixes().contains(prefix))
    return QString(prefixModes()[prefixes().indexOf(prefix)]);
  else
    return QString();
}

QString Network::prefixToMode(const QCharRef &prefix) {
  return prefixToMode(QString(prefix));
}

QString Network::modeToPrefix(const QString &mode) {
  if(prefixModes().contains(mode))
    return QString(prefixes()[prefixModes().indexOf(mode)]);
  else
    return QString();
}

QString Network::modeToPrefix(const QCharRef &mode) {
  return modeToPrefix(QString(mode));
}
  
QString Network::networkName() const {
  return _networkName;
}

QString Network::currentServer() const {
  return _currentServer;
}

QString Network::myNick() const {
  return _myNick;
}

IdentityId Network::identity() const {
  return _identity;
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

QStringList Network::channels() const {
  return _ircChannels.keys();
}

QVariantList Network::serverList() const {
  return _serverList;
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

bool Network::supports(const QString &param) const {
  return _supports.contains(param);
}

QString Network::support(const QString &param) const {
  QString support_ = param.toUpper();
  if(_supports.contains(support_))
    return _supports[support_];
  else
    return QString();
}

IrcUser *Network::newIrcUser(const QString &hostmask) {
  QString nick(nickFromMask(hostmask).toLower());
  if(!_ircUsers.contains(nick)) {
    IrcUser *ircuser = new IrcUser(hostmask, this);
    // mark IrcUser as already initialized to keep the SignalProxy from requesting initData
    //if(isInitialized())
    //  ircuser->setInitialized();
    if(proxy())
      proxy()->synchronize(ircuser);
    else
      qWarning() << "unable to synchronize new IrcUser" << hostmask << "forgot to call Network::setProxy(SignalProxy *)?";
    
    connect(ircuser, SIGNAL(nickSet(QString)), this, SLOT(ircUserNickChanged(QString)));
    connect(ircuser, SIGNAL(initDone()), this, SLOT(ircUserInitDone()));
    _ircUsers[nick] = ircuser;
    emit ircUserAdded(hostmask);
    emit ircUserAdded(ircuser);
  }
  return _ircUsers[nick];
}

IrcUser *Network::newIrcUser(const QByteArray &hostmask) {
  return newIrcUser(decodeString(hostmask));
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

void Network::removeIrcUser(QString nick) {
  IrcUser *ircuser;
  if((ircuser = ircUser(nick)) != 0)
    removeIrcUser(ircuser);
}

IrcUser *Network::ircUser(QString nickname) const {
  nickname = nickname.toLower();
  if(_ircUsers.contains(nickname))
    return _ircUsers[nickname];
  else
    return 0;
}

IrcUser *Network::ircUser(const QByteArray &nickname) const {
  return ircUser(decodeString(nickname));
}

QList<IrcUser *> Network::ircUsers() const {
  return _ircUsers.values();
}

quint32 Network::ircUserCount() const {
  return _ircUsers.count();
}

IrcChannel *Network::newIrcChannel(const QString &channelname) {
  if(!_ircChannels.contains(channelname.toLower())) {
    IrcChannel *channel = new IrcChannel(channelname, this);
    // mark IrcUser as already initialized to keep the SignalProxy from requesting initData
    //if(isInitialized())
    //  channel->setInitialized();

    if(proxy())
      proxy()->synchronize(channel);
    else
      qWarning() << "unable to synchronize new IrcChannel" << channelname << "forgot to call Network::setProxy(SignalProxy *)?";

    connect(channel, SIGNAL(initDone()), this, SLOT(ircChannelInitDone()));
    connect(channel, SIGNAL(destroyed()), this, SLOT(channelDestroyed()));
    _ircChannels[channelname.toLower()] = channel;
    emit ircChannelAdded(channelname);
    emit ircChannelAdded(channel);
  }
  return _ircChannels[channelname.toLower()];
}

IrcChannel *Network::newIrcChannel(const QByteArray &channelname) {
  return newIrcChannel(decodeString(channelname));
}

IrcChannel *Network::ircChannel(QString channelname) const {
  channelname = channelname.toLower();
  if(_ircChannels.contains(channelname))
    return _ircChannels[channelname];
  else
    return 0;
}

IrcChannel *Network::ircChannel(const QByteArray &channelname) const {
  return ircChannel(decodeString(channelname));
}


QList<IrcChannel *> Network::ircChannels() const {
  return _ircChannels.values();
}

quint32 Network::ircChannelCount() const {
  return _ircChannels.count();
}

QByteArray Network::codecForEncoding() const {
  if(_codecForEncoding) return _codecForEncoding->name();
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
  if(_codecForDecoding) return _codecForDecoding->name();
  else return QByteArray();
}

void Network::setCodecForDecoding(const QByteArray &name) {
  setCodecForDecoding(QTextCodec::codecForName(name));
}

void Network::setCodecForDecoding(QTextCodec *codec) {
  _codecForDecoding = codec;
  emit codecForDecodingSet(codecForDecoding());
}

QString Network::decodeString(const QByteArray &text) const {
  return ::decodeString(text, _codecForDecoding);
}

QByteArray Network::encodeString(const QString string) const {
  if(_codecForEncoding) {
    return _codecForEncoding->fromUnicode(string);
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
  _connected = connected;
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
  emit myNickSet(nickname);
}

void Network::setIdentity(IdentityId id) {
  _identity = id;
  emit identitySet(id);
}

void Network::setServerList(const QVariantList &serverList) {
  _serverList = serverList;
  emit serverListSet(serverList);
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

QVariantList Network::initServerList() const {
  return serverList();
}

QStringList Network::initIrcUsers() const {
  QStringList hostmasks;
  foreach(IrcUser *ircuser, ircUsers()) {
    hostmasks << ircuser->hostmask();
  }
  return hostmasks;
}

QStringList Network::initIrcChannels() const {
  return _ircChannels.keys();
}

void Network::initSetSupports(const QVariantMap &supports) {
  QMapIterator<QString, QVariant> iter(supports);
  while(iter.hasNext()) {
    iter.next();
    addSupport(iter.key(), iter.value().toString());
  }
}

void Network::initSetServerList(const QVariantList & serverList) {
  setServerList(serverList);
}

void Network::initSetIrcUsers(const QStringList &hostmasks) {
  if(!_ircUsers.empty())
    return;
  foreach(QString hostmask, hostmasks) {
    newIrcUser(hostmask);
  }
}

void Network::initSetChannels(const QStringList &channels) {
  if(!_ircChannels.empty())
    return;
  foreach(QString channel, channels)
    newIrcChannel(channel);
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
  emit ircUserInitDone(ircuser);
}

void Network::ircChannelInitDone() {
  IrcChannel *ircchannel = static_cast<IrcChannel *>(sender());
  Q_ASSERT(ircchannel);
  emit ircChannelInitDone(ircchannel);
}

void Network::channelDestroyed() {
  IrcChannel *channel = static_cast<IrcChannel *>(sender());
  Q_ASSERT(channel);
  _ircChannels.remove(_ircChannels.key(channel));
  emit ircChannelRemoved(channel);
}

void Network::requestConnect() const {
  if(!proxy()) return;
  if(proxy()->proxyMode() == SignalProxy::Client) emit connectRequested(); // on the client this triggers calling this slot on the core
  else {
    if(connectionState() != Disconnected) {
      qWarning() << "Requesting connect while not being disconnected!";
      return;
    }
    emit connectRequested(networkId());  // and this is for CoreSession :)
  }
}

void Network::requestDisconnect() const {
  if(!proxy()) return;
  if(proxy()->proxyMode() == SignalProxy::Client) emit disconnectRequested(); // on the client this triggers calling this slot on the core
  else {
    if(connectionState() == Disconnected) {
      qWarning() << "Requesting disconnect while not being connected!";
      return;
    }
    emit disconnectRequested(networkId());  // and this is for CoreSession :)
  }
}

void Network::emitConnectionError(const QString &errorMsg) {
  emit connectionError(errorMsg);
}

// ====================
//  Private:
// ====================
void Network::determinePrefixes() {
  // seems like we have to construct them first
  QString PREFIX = support("PREFIX");
  
  if(PREFIX.startsWith("(") && PREFIX.contains(")")) {
    _prefixes = PREFIX.section(")", 1);
    _prefixModes = PREFIX.mid(1).section(")", 0, 0);
  } else {
    QString defaultPrefixes("~&@%+");
    QString defaultPrefixModes("qaohv");

    // we just assume that in PREFIX are only prefix chars stored
    for(int i = 0; i < defaultPrefixes.size(); i++) {
      if(PREFIX.contains(defaultPrefixes[i])) {
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
      if(PREFIX.contains(defaultPrefixModes[i])) {
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
  if(codecForEncoding != other.codecForEncoding) return false;
  if(codecForDecoding != other.codecForDecoding) return false;
  if(serverList != other.serverList) return false;
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
  i["CodecForEncoding"] = info.codecForEncoding;
  i["CodecForDecoding"] = info.codecForDecoding;
  i["ServerList"] = info.serverList;
  out << i;
  return out;
}

QDataStream &operator>>(QDataStream &in, NetworkInfo &info) {
  QVariantMap i;
  in >> i;
  info.networkId = i["NetworkId"].value<NetworkId>();
  info.networkName = i["NetworkName"].toString();
  info.identity = i["Identity"].value<IdentityId>();
  info.codecForEncoding = i["CodecForEncoding"].toByteArray();
  info.codecForDecoding = i["CodecForDecoding"].toByteArray();
  info.serverList = i["ServerList"].toList();
  return in;
}

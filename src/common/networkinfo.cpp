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
#include "networkinfo.h"

#include "signalproxy.h"
#include "ircuser.h"
#include "ircchannel.h"

#include <QDebug>
#include <QTextCodec>

#include "util.h"

// ====================
//  Public:
// ====================
NetworkInfo::NetworkInfo(const uint &networkid, QObject *parent)
  : SyncableObject(parent),
    _networkId(networkid),
    _initialized(false),
    _myNick(QString()),
    _networkName(QString()),
    _currentServer(QString()),
    _prefixes(QString()),
    _prefixModes(QString()),
    _proxy(0),
    _codecForEncoding(0),
    _codecForDecoding(0)
{
  setObjectName(QString::number(networkid));
}

// I think this is unnecessary since IrcUsers have us as their daddy :)
//NetworkInfo::~NetworkInfo() {
//   QHashIterator<QString, IrcUser *> ircuser(_ircUsers);
//   while (ircuser.hasNext()) {
//     ircuser.next();
//     delete ircuser.value();
//   }
//}

uint NetworkInfo::networkId() const {
  return _networkId;
}

bool NetworkInfo::initialized() const {
  return _initialized;
}

SignalProxy *NetworkInfo::proxy() const {
  return _proxy;
}

void NetworkInfo::setProxy(SignalProxy *proxy) {
  _proxy = proxy;
  proxy->synchronize(this);
}

bool NetworkInfo::isMyNick(const QString &nick) const {
  return (myNick().toLower() == nick.toLower());
}

bool NetworkInfo::isMyNick(IrcUser *ircuser) const {
  return (ircuser->nick().toLower() == myNick().toLower());
}

bool NetworkInfo::isChannelName(const QString &channelname) const {
  if(channelname.isEmpty())
    return false;
  
  if(supports("CHANTYPES"))
    return support("CHANTYPES").contains(channelname[0]);
  else
    return QString("#&!+").contains(channelname[0]);
}

QString NetworkInfo::prefixToMode(const QString &prefix) {
  if(prefixes().contains(prefix))
    return QString(prefixModes()[prefixes().indexOf(prefix)]);
  else
    return QString();
}

QString NetworkInfo::prefixToMode(const QCharRef &prefix) {
  return prefixToMode(QString(prefix));
}

QString NetworkInfo::modeToPrefix(const QString &mode) {
  if(prefixModes().contains(mode))
    return QString(prefixes()[prefixModes().indexOf(mode)]);
  else
    return QString();
}

QString NetworkInfo::modeToPrefix(const QCharRef &mode) {
  return modeToPrefix(QString(mode));
}
  
QString NetworkInfo::networkName() const {
  return _networkName;
}

QString NetworkInfo::currentServer() const {
  return _currentServer;
}

QString NetworkInfo::myNick() const {
  return _myNick;
}

QStringList NetworkInfo::nicks() const {
  // we don't use _ircUsers.keys() since the keys may be
  // not up to date after a nick change
  QStringList nicks;
  foreach(IrcUser *ircuser, _ircUsers.values()) {
    nicks << ircuser->nick();
  }
  return nicks;
}

QStringList NetworkInfo::channels() const {
  return _ircChannels.keys();
}

QString NetworkInfo::prefixes() {
  if(_prefixes.isNull())
    determinePrefixes();
  
  return _prefixes;
}

QString NetworkInfo::prefixModes() {
  if(_prefixModes.isNull())
    determinePrefixes();

  return _prefixModes;
}

bool NetworkInfo::supports(const QString &param) const {
  return _supports.contains(param);
}

QString NetworkInfo::support(const QString &param) const {
  QString support_ = param.toUpper();
  if(_supports.contains(support_))
    return _supports[support_];
  else
    return QString();
}

IrcUser *NetworkInfo::newIrcUser(const QString &hostmask) {
  QString nick(nickFromMask(hostmask).toLower());
  if(!_ircUsers.contains(nick)) {
    IrcUser *ircuser = new IrcUser(hostmask, this);
    // mark IrcUser as already initialized to keep the SignalProxy from requesting initData
    if(initialized())
      ircuser->setInitialized();
    if(proxy())
      proxy()->synchronize(ircuser);
    else
      qWarning() << "unable to synchronize new IrcUser" << hostmask << "forgot to call NetworkInfo::setProxy(SignalProxy *)?";
    
    connect(ircuser, SIGNAL(nickSet(QString)), this, SLOT(ircUserNickChanged(QString)));
    connect(ircuser, SIGNAL(initDone()), this, SIGNAL(ircUserInitDone()));
    connect(ircuser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
    _ircUsers[nick] = ircuser;
    emit ircUserAdded(hostmask);
  }
  return _ircUsers[nick];
}

IrcUser *NetworkInfo::newIrcUser(const QByteArray &hostmask) {
  return newIrcUser(decodeString(hostmask));
}

void NetworkInfo::removeIrcUser(IrcUser *ircuser) {
  QString nick = _ircUsers.key(ircuser);
  if(nick.isNull())
    return;

  _ircUsers.remove(nick);
  ircuser->deleteLater();
  emit ircUserRemoved(nick);
}

void NetworkInfo::removeIrcUser(QString nick) {
  IrcUser *ircuser;
  if((ircuser = ircUser(nick)) != 0)
    removeIrcUser(ircuser);
}

IrcUser *NetworkInfo::ircUser(QString nickname) const {
  nickname = nickname.toLower();
  if(_ircUsers.contains(nickname))
    return _ircUsers[nickname];
  else
    return 0;
}

IrcUser *NetworkInfo::ircUser(const QByteArray &nickname) const {
  return ircUser(decodeString(nickname));
}

QList<IrcUser *> NetworkInfo::ircUsers() const {
  return _ircUsers.values();
}

IrcChannel *NetworkInfo::newIrcChannel(const QString &channelname) {
  if(!_ircChannels.contains(channelname.toLower())) {
    IrcChannel *channel = new IrcChannel(channelname, this);
    // mark IrcUser as already initialized to keep the SignalProxy from requesting initData
    if(initialized())
      channel->setInitialized();

    if(proxy())
      proxy()->synchronize(channel);
    else
      qWarning() << "unable to synchronize new IrcChannel" << channelname << "forgot to call NetworkInfo::setProxy(SignalProxy *)?";

    connect(channel, SIGNAL(initDone()), this, SIGNAL(ircChannelInitDone()));
    connect(channel, SIGNAL(destroyed()), this, SLOT(channelDestroyed()));
    _ircChannels[channelname.toLower()] = channel;
    emit ircChannelAdded(channelname);
  }
  return _ircChannels[channelname.toLower()];
}

IrcChannel *NetworkInfo::newIrcChannel(const QByteArray &channelname) {
  return newIrcChannel(decodeString(channelname));
}

IrcChannel *NetworkInfo::ircChannel(QString channelname) {
  channelname = channelname.toLower();
  if(_ircChannels.contains(channelname))
    return _ircChannels[channelname];
  else
    return 0;
}

IrcChannel *NetworkInfo::ircChannel(const QByteArray &channelname) {
  return ircChannel(decodeString(channelname));
}


QList<IrcChannel *> NetworkInfo::ircChannels() const {
  return _ircChannels.values();
}

QTextCodec *NetworkInfo::codecForEncoding() const {
  return _codecForEncoding;
}

void NetworkInfo::setCodecForEncoding(const QString &name) {
  setCodecForEncoding(QTextCodec::codecForName(name.toAscii()));
}

void NetworkInfo::setCodecForEncoding(QTextCodec *codec) {
  _codecForEncoding = codec;
}

QTextCodec *NetworkInfo::codecForDecoding() const {
  return _codecForDecoding;
}

void NetworkInfo::setCodecForDecoding(const QString &name) {
  setCodecForDecoding(QTextCodec::codecForName(name.toAscii()));
}

void NetworkInfo::setCodecForDecoding(QTextCodec *codec) {
  _codecForDecoding = codec;
}

QString NetworkInfo::decodeString(const QByteArray &text) const {
  return ::decodeString(text, _codecForDecoding);
}

QByteArray NetworkInfo::encodeString(const QString string) const {
  if(_codecForEncoding) {
    return _codecForEncoding->fromUnicode(string);
  }
  return string.toAscii();
}

// ====================
//  Public Slots:
// ====================
void NetworkInfo::setNetworkName(const QString &networkName) {
  _networkName = networkName;
  emit networkNameSet(networkName);
}

void NetworkInfo::setCurrentServer(const QString &currentServer) {
  _currentServer = currentServer;
  emit currentServerSet(currentServer);
}

void NetworkInfo::setMyNick(const QString &nickname) {
  _myNick = nickname;
  emit myNickSet(nickname);
}

void NetworkInfo::addSupport(const QString &param, const QString &value) {
  if(!_supports.contains(param)) {
    _supports[param] = value;
    emit supportAdded(param, value);
  }
}

void NetworkInfo::removeSupport(const QString &param) {
  if(_supports.contains(param)) {
    _supports.remove(param);
    emit supportRemoved(param);
  }
}

QVariantMap NetworkInfo::initSupports() const {
  QVariantMap supports;
  QHashIterator<QString, QString> iter(_supports);
  while(iter.hasNext()) {
    iter.next();
    supports[iter.key()] = iter.value();
  }
  return supports;
}

QStringList NetworkInfo::initIrcUsers() const {
  QStringList hostmasks;
  foreach(IrcUser *ircuser, ircUsers()) {
    hostmasks << ircuser->hostmask();
  }
  return hostmasks;
}

QStringList NetworkInfo::initIrcChannels() const {
  return _ircChannels.keys();
}

void NetworkInfo::initSetSupports(const QVariantMap &supports) {
  QMapIterator<QString, QVariant> iter(supports);
  while(iter.hasNext()) {
    iter.next();
    addSupport(iter.key(), iter.value().toString());
  }
}

void NetworkInfo::initSetIrcUsers(const QStringList &hostmasks) {
  if(!_ircUsers.empty())
    return;
  foreach(QString hostmask, hostmasks) {
    newIrcUser(hostmask);
  }
}

void NetworkInfo::initSetChannels(const QStringList &channels) {
  if(!_ircChannels.empty())
    return;
  foreach(QString channel, channels)
    newIrcChannel(channel);
}

IrcUser *NetworkInfo::updateNickFromMask(const QString &mask) {
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

void NetworkInfo::ircUserNickChanged(QString newnick) {
  QString oldnick = _ircUsers.key(qobject_cast<IrcUser*>(sender()));

  if(oldnick.isNull())
    return;

  if(newnick.toLower() != oldnick) _ircUsers[newnick.toLower()] = _ircUsers.take(oldnick);
  
  if(myNick().toLower() == oldnick)
    setMyNick(newnick);
}

void NetworkInfo::ircUserDestroyed() {
  IrcUser *ircuser = static_cast<IrcUser *>(sender());
  Q_ASSERT(ircuser);
  removeIrcUser(ircuser);
}

void NetworkInfo::channelDestroyed() {
  IrcChannel *channel = static_cast<IrcChannel *>(sender());
  Q_ASSERT(channel);
  _ircChannels.remove(_ircChannels.key(channel));
}

void NetworkInfo::setInitialized() {
  _initialized = true;
  emit initDone();
}

// ====================
//  Private:
// ====================
void NetworkInfo::determinePrefixes() {
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


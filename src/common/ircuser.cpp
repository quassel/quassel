/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#include "ircuser.h"
#include "util.h"

#include "networkinfo.h"
#include "signalproxy.h"
#include "ircchannel.h"

#include <QTextCodec>
#include <QDebug>

IrcUser::IrcUser(const QString &hostmask, NetworkInfo *networkinfo)
  : QObject(networkinfo),
    _initialized(false),
    _nick(nickFromMask(hostmask)),
    _user(userFromMask(hostmask)),
    _host(hostFromMask(hostmask)),
    networkInfo(networkinfo),
    _codecForEncoding(0),
    _codecForDecoding(0)
{
  updateObjectName();
}

IrcUser::~IrcUser() {
  //qDebug() << nick() << "destroyed.";
}

// ====================
//  PUBLIC:
// ====================
bool IrcUser::initialized() const {
  return _initialized;
}

QString IrcUser::user() const {
  return _user;
}

QString IrcUser::host() const {
  return _host;
}

QString IrcUser::nick() const {
  return _nick;
}

QString IrcUser::hostmask() const {
  return QString("%1!%2@%3").arg(nick()).arg(user()).arg(host());
}

QString IrcUser::userModes() const {
  return _userModes;
}

QStringList IrcUser::channels() const {
  QStringList chanList;
  IrcChannel *channel;
  foreach(channel, _channels) {
    chanList << channel->name();
  }
  return chanList;
}

QTextCodec *IrcUser::codecForEncoding() const {
  return _codecForEncoding;
}

void IrcUser::setCodecForEncoding(const QString &name) {
  setCodecForEncoding(QTextCodec::codecForName(name.toAscii()));
}

void IrcUser::setCodecForEncoding(QTextCodec *codec) {
  _codecForEncoding = codec;
}

QTextCodec *IrcUser::codecForDecoding() const {
  return _codecForDecoding;
}

void IrcUser::setCodecForDecoding(const QString &name) {
  setCodecForDecoding(QTextCodec::codecForName(name.toAscii()));
}

void IrcUser::setCodecForDecoding(QTextCodec *codec) {
  _codecForDecoding = codec;
}

QString IrcUser::decodeString(const QByteArray &text) const {
  if(!codecForDecoding()) return networkInfo->decodeString(text);
  return ::decodeString(text, codecForDecoding());
}

QByteArray IrcUser::encodeString(const QString string) const {
  if(codecForEncoding()) {
    return codecForEncoding()->fromUnicode(string);
  }
  return networkInfo->encodeString(string);
}

// ====================
//  PUBLIC SLOTS:
// ====================
void IrcUser::setUser(const QString &user) {
  if(!user.isEmpty() && _user != user) {
    _user = user;
    emit userSet(user);
  }
}

void IrcUser::setHost(const QString &host) {
  if(!host.isEmpty() && _host != host) {
    _host = host;
    emit hostSet(host);
  }
}

void IrcUser::setNick(const QString &nick) {
  if(!nick.isEmpty() && nick != _nick) {
    _nick = nick;
    updateObjectName();
    emit nickSet(nick);
  }
}

void IrcUser::updateObjectName() {
  QString newName = QString::number(networkInfo->networkId()) + "/" + _nick;
  QString oldName = objectName();
  if(oldName != newName) {
    setObjectName(newName);
    emit renameObject(oldName, newName);
  }
}

void IrcUser::updateHostmask(const QString &mask) {
  if(mask == hostmask())
    return;

  QString user = userFromMask(mask);
  QString host = hostFromMask(mask);
  setUser(user);
  setHost(host);
}

void IrcUser::joinChannel(IrcChannel *channel) {
  Q_ASSERT(channel);
  if(!_channels.contains(channel)) {
    _channels.insert(channel);
    channel->join(this);
    connect(channel, SIGNAL(destroyed()), this, SLOT(channelDestroyed()));
    emit channelJoined(channel->name());
  }
}

void IrcUser::joinChannel(const QString &channelname) {
  joinChannel(networkInfo->newIrcChannel(channelname));
}

void IrcUser::partChannel(IrcChannel *channel) {
  if(_channels.contains(channel)) {
    _channels.remove(channel);
    disconnect(channel, 0, this, 0);
    channel->part(this);
    emit channelParted(channel->name());
  }
}

void IrcUser::partChannel(const QString &channelname) {
  IrcChannel *channel = networkInfo->ircChannel(channelname);
  if(channel == 0) {
    qWarning() << "IrcUser::partChannel(): received part for unknown Channel" << channelname;
  } else {
    partChannel(channel);
  }
}

void IrcUser::channelDestroyed() {
  // private slot!
  IrcChannel *channel = static_cast<IrcChannel*>(sender());
  Q_ASSERT(channel);
  if(_channels.contains(channel)) {
    _channels.remove(channel);
    disconnect(channel, 0, this, 0);
  }
}

void IrcUser::setUserModes(const QString &modes) {
  _userModes = modes;
  emit userModesSet(modes);
}

void IrcUser::addUserMode(const QString &mode) {
  if(!_userModes.contains(mode)) {
    _userModes += mode;
    emit userModeAdded(mode);
  }
}

void IrcUser::removeUserMode(const QString &mode) {
  if(_userModes.contains(mode)) {
    _userModes.remove(mode);
    emit userModeRemoved(mode);
  }
}

void IrcUser::initSetChannels(const QStringList channels) {
  foreach(QString channel, channels) {
    joinChannel(channel);
  }
}

void IrcUser::setInitialized() {
  _initialized = true;
  emit initDone();
}


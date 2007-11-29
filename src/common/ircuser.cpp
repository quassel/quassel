/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include <QDebug>

IrcUser::IrcUser(const QString &hostmask, NetworkInfo *networkinfo)
  : QObject(networkinfo),
    _initialized(false),
    _nick(nickFromMask(hostmask)),
    _user(userFromMask(hostmask)),
    _host(hostFromMask(hostmask)),
    networkInfo(networkinfo)
{
  updateObjectName();
}

IrcUser::~IrcUser() {
  qDebug() << nick() << "destroyed.";
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
  return _channels.toList();
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
    QString oldnick(_nick);
    _nick = nick;
    updateObjectName();
    emit nickSet(nick);
  }
}

void IrcUser::updateObjectName() {
  QString oldName(objectName());
  setObjectName(QString::number(networkInfo->networkId()) + "/" + _nick);
  if(!oldName.isEmpty()) {
    emit renameObject(oldName, objectName());
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

void IrcUser::joinChannel(const QString &channel) {
  if(!_channels.contains(channel)) {
    _channels.insert(channel);
    networkInfo->newIrcChannel(channel)->join(this);
    emit channelJoined(channel);
  }
}

void IrcUser::partChannel(const QString &channel) {
  if(_channels.contains(channel)) {
    _channels.remove(channel);

    Q_ASSERT(networkInfo->ircChannel(channel));
    networkInfo->ircChannel(channel)->part(this);
    
    emit channelParted(channel);
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


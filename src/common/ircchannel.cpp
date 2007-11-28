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

#include "ircchannel.h"

#include "networkinfo.h"
//#include "nicktreemodel.h"
#include "signalproxy.h"
#include "ircuser.h"

#include <QMapIterator>
#include <QHashIterator>

#include <QDebug>


IrcChannel::IrcChannel(const QString &channelname, NetworkInfo *networkinfo) 
  : QObject(networkinfo),
    _initialized(false),
    _name(channelname),
    _topic(QString()),
    networkInfo(networkinfo)
{
  setObjectName(QString::number(networkInfo->networkId()) + "/" +  channelname);
}

IrcChannel::~IrcChannel() {

}

// ====================
//  PUBLIC:
// ====================
bool IrcChannel::isKnownUser(IrcUser *ircuser) const {
  bool isknown = true;

  if(ircuser == 0) {
    qWarning() << "Channel" << name() << "received IrcUser Nullpointer!";
    isknown = false;
  }
  
  if(!_userModes.contains(ircuser)) {
    qWarning() << "Channel" << name() << "received data for unknown User" << ircuser->nick();
    isknown = false;
  }
  
  return isknown;
}

bool IrcChannel::isValidChannelUserMode(const QString &mode) const {
  bool isvalid = true;
  if(mode.size() > 1) {
    qWarning() << "Channel" << name() << "received Channel User Mode which is longer then 1 Char:" << mode;
    isvalid = false;
  }
  return isvalid;
}

bool IrcChannel::initialized() const {
  return _initialized;
}

QString IrcChannel::name() const {
  return _name;
}

QString IrcChannel::topic() const {
  return _topic;
}

QList<IrcUser *> IrcChannel::ircUsers() const {
  return _userModes.keys();
}

QString IrcChannel::userModes(IrcUser *ircuser) const {
  if(_userModes.contains(ircuser))
    return _userModes[ircuser];
  else
    return QString();
}

QString IrcChannel::userModes(const QString &nick) const {
  return userModes(networkInfo->ircUser(nick));
}

// ====================
//  PUBLIC SLOTS:
// ====================
void IrcChannel::setTopic(const QString &topic) {
  _topic = topic;
  emit topicSet(topic);
}

void IrcChannel::join(IrcUser *ircuser) {
  if(!_userModes.contains(ircuser) && ircuser) {
    _userModes[ircuser] = QString();
    ircuser->joinChannel(name());
    connect(ircuser, SIGNAL(nickSet(QString)), this, SLOT(ircUserNickSet(QString)));
    connect(ircuser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
    // if you wonder why there is no counterpart to ircUserJoined:
    // the joines are propagted by the ircuser. the signal ircUserJoined is only for convenience
    emit ircUserJoined(ircuser);
  }
}

void IrcChannel::join(const QString &nick) {
  join(networkInfo->ircUser(nick));
}

void IrcChannel::part(IrcUser *ircuser) {
  if(isKnownUser(ircuser)) {
    _userModes.remove(ircuser);
    ircuser->partChannel(name());
    // if you wonder why there is no counterpart to ircUserParted:
    // the joines are propagted by the ircuser. the signal ircUserParted is only for convenience
    emit ircUserParted(ircuser);
  }
}

void IrcChannel::part(const QString &nick) {
  part(networkInfo->ircUser(nick));
}

// SET USER MODE
void IrcChannel::setUserModes(IrcUser *ircuser, const QString &modes) {
  if(isKnownUser(ircuser)) {
    _userModes[ircuser] = modes;
    emit userModesSet(ircuser->nick(), modes);
    emit ircUserModesSet(ircuser, modes);
  }
}

void IrcChannel::setUserModes(const QString &nick, const QString &modes) {
  setUserModes(networkInfo->ircUser(nick), modes);
}

// ADD USER MODE
void IrcChannel::addUserMode(IrcUser *ircuser, const QString &mode) {
  if(!isKnownUser(ircuser) || !isValidChannelUserMode(mode))
    return;

  if(!_userModes[ircuser].contains(mode)) {
    _userModes[ircuser] += mode;
    emit userModeAdded(ircuser->nick(), mode);
    emit ircUserModeAdded(ircuser, mode);
  }

}

void IrcChannel::addUserMode(const QString &nick, const QString &mode) {
  addUserMode(networkInfo->ircUser(nick), mode);
}

// REMOVE USER MODE
void IrcChannel::removeUserMode(IrcUser *ircuser, const QString &mode) {
  if(!isKnownUser(ircuser) || !isValidChannelUserMode(mode))
    return;

  if(_userModes[ircuser].contains(mode)) {
    _userModes[ircuser].remove(mode);
    emit userModeRemoved(ircuser->nick(), mode);
    emit ircUserModeRemoved(ircuser, mode);
  }

}

void IrcChannel::removeUserMode(const QString &nick, const QString &mode) {
  removeUserMode(networkInfo->ircUser(nick), mode);
}

// INIT SET USER MODES
QVariantMap IrcChannel::initUserModes() const {
  QVariantMap usermodes;
  QHash<IrcUser *, QString>::const_iterator iter = _userModes.constBegin();
  while(iter != _userModes.constEnd()) {
    usermodes[iter.key()->nick()] = iter.value();
    iter++;
  }
  return usermodes;
}

void IrcChannel::initSetUserModes(const QVariantMap &usermodes) {
  QMapIterator<QString, QVariant> iter(usermodes);
  while(iter.hasNext()) {
    iter.next();
    setUserModes(iter.key(), iter.value().toString());
  }
}

void IrcChannel::ircUserDestroyed() {
  IrcUser *ircUser = static_cast<IrcUser *>(sender());
  Q_ASSERT(ircUser);
  _userModes.remove(ircUser);
  emit ircUserParted(ircUser);
}

void IrcChannel::ircUserNickSet(QString nick) {
  IrcUser *ircUser = qobject_cast<IrcUser *>(sender());
  Q_ASSERT(ircUser);
  emit ircUserNickSet(ircUser, nick);
}

void IrcChannel::setInitialized() {
  _initialized = true;
  emit initDone();
}


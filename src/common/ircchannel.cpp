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

#include "ircchannel.h"

#include "network.h"
//#include "nicktreemodel.h"
#include "signalproxy.h"
#include "ircuser.h"
#include "util.h"

#include <QMapIterator>
#include <QHashIterator>
#include <QTextCodec>

#include <QDebug>


IrcChannel::IrcChannel(const QString &channelname, Network *network) 
  : SyncableObject(network),
    _initialized(false),
    _name(channelname),
    _topic(QString()),
    network(network)
{
  setObjectName(QString::number(network->networkId()) + "/" +  channelname);
}

IrcChannel::~IrcChannel() {

}

// ====================
//  PUBLIC:
// ====================
bool IrcChannel::isKnownUser(IrcUser *ircuser) const {
  if(ircuser == 0) {
    qWarning() << "Channel" << name() << "received IrcUser Nullpointer!";
    return false;
  }
  
  if(!_userModes.contains(ircuser)) {
    qWarning() << "Channel" << name() << "received data for unknown User" << ircuser->nick();
    return false;
  }

  return true;
}

bool IrcChannel::isValidChannelUserMode(const QString &mode) const {
  bool isvalid = true;
  if(mode.size() > 1) {
    qWarning() << "Channel" << name() << "received Channel User Mode which is longer then 1 Char:" << mode;
    isvalid = false;
  }
  return isvalid;
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
  return userModes(network->ircUser(nick));
}

QTextCodec *IrcChannel::codecForEncoding() const {
  return _codecForEncoding;
}

void IrcChannel::setCodecForEncoding(const QString &name) {
  setCodecForEncoding(QTextCodec::codecForName(name.toAscii()));
}

void IrcChannel::setCodecForEncoding(QTextCodec *codec) {
  _codecForEncoding = codec;
}

QTextCodec *IrcChannel::codecForDecoding() const {
  return _codecForDecoding;
}

void IrcChannel::setCodecForDecoding(const QString &name) {
  setCodecForDecoding(QTextCodec::codecForName(name.toAscii()));
}

void IrcChannel::setCodecForDecoding(QTextCodec *codec) {
  _codecForDecoding = codec;
}

QString IrcChannel::decodeString(const QByteArray &text) const {
  if(!codecForDecoding()) return network->decodeString(text);
  return ::decodeString(text, _codecForDecoding);
}

QByteArray IrcChannel::encodeString(const QString string) const {
  if(codecForEncoding()) {
    return _codecForEncoding->fromUnicode(string);
  }
  return network->encodeString(string);
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
    //qDebug() << "JOIN" << name() << ircuser->nick() << ircUsers().count();
    connect(ircuser, SIGNAL(nickSet(QString)), this, SLOT(ircUserNickSet(QString)));
    connect(ircuser, SIGNAL(destroyed()), this, SLOT(ircUserDestroyed()));
    // if you wonder why there is no counterpart to ircUserJoined:
    // the joines are propagted by the ircuser. the signal ircUserJoined is only for convenience
    emit ircUserJoined(ircuser);
  }
}

void IrcChannel::join(const QString &nick) {
  join(network->ircUser(nick));
}

void IrcChannel::part(IrcUser *ircuser) {
  if(isKnownUser(ircuser)) {
    _userModes.remove(ircuser);
    ircuser->partChannel(name());
    //qDebug() << "PART" << name() << ircuser->nick() << ircUsers().count();
    // if you wonder why there is no counterpart to ircUserParted:
    // the joines are propagted by the ircuser. the signal ircUserParted is only for convenience
    emit ircUserParted(ircuser);
    if(network->isMe(ircuser))
       deleteLater();
  }
}

void IrcChannel::part(const QString &nick) {
  part(network->ircUser(nick));
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
  setUserModes(network->ircUser(nick), modes);
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
  addUserMode(network->ircUser(nick), mode);
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
  removeUserMode(network->ircUser(nick), mode);
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
  //qDebug() << "DEST" << name() << ircUsers().count();
}

void IrcChannel::ircUserNickSet(QString nick) {
  IrcUser *ircUser = qobject_cast<IrcUser *>(sender());
  Q_ASSERT(ircUser);
  emit ircUserNickSet(ircUser, nick);
}


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
#include "serverinfo.h"

ServerInfo::ServerInfo(QObject *parent)
  : QObject(parent) {
}

ServerInfo::~ServerInfo() {
}

void ServerInfo::setNetworkname(const QString &networkname) {
  networkname_ = networkname;
}

QString ServerInfo::networkname() const {
  return networkname_;
}

void ServerInfo::setCurrentServer(const QString &currentServer) {
  currentServer_ = currentServer;
}

QString ServerInfo::currentServer() const {
  return currentServer_;
}

void ServerInfo::setOwnNick(const QString &ownnick) {
  ownNick_ = ownnick;
}

QString ServerInfo::ownNick() const {
  return ownNick_;
}

QList<IrcUser *> ServerInfo::ircUsers() const {
  return ircUsers_.values();
}


IrcUser *ServerInfo::newIrcUser(const QString &hostmask) {
  IrcUser *ircuser = new IrcUser(hostmask);
  return ircUsers_[ircuser->nick()] = ircuser;
}

IrcUser *ServerInfo::ircUser(const QString &nickname) const {
  if(ircUsers_.contains(nickname))
    return ircUsers_[nickname];
  else
    throw NoSuchNickException();
}

void ServerInfo::setTopics(const QHash<QString, QString> &topics) {
  topics_ = topics;
}

QHash<QString, QString> ServerInfo::topics() const {
  return topics_;
}

void ServerInfo::updateTopic(const QString &channel, const QString &topic) {
  topics_[channel] = topic;
}

QString ServerInfo::topic(const QString &channel) const {
  if(topics_.contains(channel))
    return topics_[channel];
  else
    throw NoSuchChannelException();
}

void ServerInfo::setSupports(const QHash<QString, QString> &supports) {
  supports_ = supports;
}

QHash<QString, QString> ServerInfo::supports() const {
  return supports_;
}

QString ServerInfo::supports(const QString &feature) const {
  if(supports_.contains(feature))
    return supports_[feature];
  else
    throw UnsupportedFeatureException();
}

bool ServerInfo::isOwnNick(const QString &nick) const {
  return (ownNick_.toLower() == nick.toLower());
}

bool ServerInfo::isOwnNick(const IrcUser &ircuser) const {
  return (ircuser.nick().toLower() == ownNick_.toLower());
}



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

IrcUser::IrcUser(QObject *parent)
  : QObject(parent) {
}

IrcUser::IrcUser(const QString &hostmask, QObject *parent) 
  : QObject(parent),
    nick_(nickFromMask(hostmask)),
    user_(userFromMask(hostmask)),
    host_(hostFromMask(hostmask)) {
}

IrcUser::~IrcUser() {
}

void IrcUser::setUser(const QString &user) {
  user_ = user;
}

QString IrcUser::user() const {
  return user_;
}

void IrcUser::setHost(const QString &host) {
  host_ = host;
}

QString IrcUser::host() const {
  return host_;
}

void IrcUser::setNick(const QString &nick) {
  nick_ = nick;
}

QString IrcUser::nick() const {
  return nick_;
}

void IrcUser::setUsermodes(const QSet<QString> &usermodes) {
  usermodes_ = usermodes;
}

QSet<QString> IrcUser::usermodes() const {
  return usermodes_;
}

void IrcUser::setChannelmode(const QString &channel, const QSet<QString> &channelmode) {
  if(channelmodes_.contains(channel))
    channelmodes_[channel] |= channelmode;
  else
    channelmodes_[channel] = channelmode;
}

QSet<QString> IrcUser::channelmode(const QString &channel) const {
  if(channelmodes_.contains(channel))
    //throw NoSuchChannelException();
    Q_ASSERT(false); // FIXME: exception disabled for qtopia testing
  else
    return QSet<QString>();
}

void IrcUser::updateChannelmode(const QString &channel, const QString &channelmode, bool add) {
  if(add)
    addChannelmode(channel, channelmode);
  else
    removeChannelmode(channel, channelmode);
}

void IrcUser::addChannelmode(const QString &channel, const QString &channelmode) {
  if(!channelmodes_.contains(channel))
    channelmodes_[channel] = QSet<QString>();
  channelmodes_[channel] << channelmode;
}

void IrcUser::removeChannelmode(const QString &channel, const QString &channelmode) {
  if(channelmodes_.contains(channel))
    channelmodes_[channel].remove(channelmode);
}

QStringList IrcUser::channels() const {
  return channelmodes_.keys();
}

void IrcUser::joinChannel(const QString &channel) {
  if(!channelmodes_.contains(channel))
    channelmodes_[channel] = QSet<QString>();
}

void IrcUser::partChannel(const QString &channel) {
  channelmodes_.remove(channel);
}

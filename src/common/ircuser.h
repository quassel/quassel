/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
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

#ifndef _IRCUSER_H_
#define _IRCUSER_H_

#include <QtCore>
#include <QObject>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

#include "global.h"

class IrcUser : public QObject{
//  Q_OBJECT
  
//  Q_PROPERTY(QString user READ user WRITE setUser)
//  Q_PROPERTY(QString host READ host WRITE setHost)
//  Q_PROPERTY(QString nick READ nick WRITE setNick)
//  Q_PROPERTY(QSet<QString> usermodes READ usermodes WRITE setUsermodes)
//  Q_PROPERTY(QStringList channels READ channels)
  
public:
  IrcUser(QObject *parent = 0);
  IrcUser(const QString &hostmask, QObject *parent = 0);
  ~IrcUser();
  
  void setUser(const QString &user);
  QString user() const;
  
  void setHost(const QString &host);
  QString host() const;
  
  void setNick(const QString &nick);
  QString nick() const;
  
  void setUsermodes(const QSet<QString> &usermodes);
  QSet<QString> usermodes() const;
  
  void setChannelmode(const QString &channel, const QSet<QString> &channelmode);
  QSet<QString> channelmode(const QString &channel) const;
  
  void updateChannelmode(const QString &channel, const QString &channelmode, bool add=true);
  void addChannelmode(const QString &channel, const QString &channelmode);
  void removeChannelmode(const QString &channel, const QString &channelmode);

  QStringList channels() const;
  
  void joinChannel(const QString &channel);
  void partChannel(const QString &channel);
  
private:
  inline bool operator==(const IrcUser &ircuser2) {
    return (nick_.toLower() == ircuser2.nick().toLower());
  }

  inline bool operator==(const QString &nickname) {
    return (nick_.toLower() == nickname.toLower());
  }
  
  QString nick_;
  QString user_;
  QString host_;
  
  QHash<QString, QSet<QString> > channelmodes_; //keys: channelnames; values: Set of Channelmodes
  QSet<QString> usermodes_;
};

struct IrcUserException : public Exception {};
struct NoSuchChannelException : public IrcUserException {};
struct NoSuchNickException : public IrcUserException {};


#endif

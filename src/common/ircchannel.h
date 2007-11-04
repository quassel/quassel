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

#ifndef _IRCCHANNEL_H_
#define _IRCCHANNEL_H_

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class SignalProxy;
class NetworkInfo;
class IrcUser;

class IrcChannel : public QObject {
  Q_OBJECT
  
  Q_PROPERTY(QString name READ name STORED false)
  Q_PROPERTY(QString topic READ topic WRITE setTopic STORED false)

public:
  IrcChannel(const QString &channelname, NetworkInfo *networkInfo);

  bool isKnownUser(IrcUser *ircuser) const;
  bool isValidChannelUserMode(const QString &mode) const;

  bool initialized() const;

  QString name() const;
  QString topic() const;
  
  QList<IrcUser *> ircUsers() const;
  
  QString userMode(IrcUser *ircuser) const;
  QString userMode(const QString &nick) const;


public slots:  
  void setTopic(const QString &topic);

  void join(IrcUser *ircuser);
  void join(const QString &nick);

  void part(IrcUser *ircuser);
  void part(const QString &nick);

  void setUserModes(IrcUser *ircuser, const QString &modes);
  void setUserModes(const QString &nick, const QString &modes);
  
  void addUserMode(IrcUser *ircuser, const QString &mode);
  void addUserMode(const QString &nick, const QString &mode);
  
  void removeUserMode(IrcUser *ircuser, const QString &mode);
  void removeUserMode(const QString &nick, const QString &mode);

  // init geters
  QVariantMap initUserModes() const;

  // init seters
  void initSetUserModes(const QVariantMap &usermodes);
  
  void setInitialized();

signals:
  void topicSet(QString topic);
  void userModesSet(QString nick, QString modes);
  void userModeAdded(QString nick, QString mode);
  void userModeRemoved(QString nick, QString mode);

  void initDone();

private slots:
   void ircUserDestroyed();

private:
  bool _initialized;
  QString _name;
  QString _topic;

  QHash<IrcUser *, QString> _userModes; 
  
  NetworkInfo *networkInfo;
};

#endif

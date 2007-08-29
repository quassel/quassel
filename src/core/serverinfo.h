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

#ifndef _SERVERINFO_H_
#define _SERVERINFO_H_

#include <QString>
#include <QList>
#include <QHash>

#include "global.h"
#include "ircuser.h"

class ServerInfo : public QObject {
  Q_OBJECT
  
  Q_PROPERTY(QString networkname READ networkname WRITE setNetworkname)
  Q_PROPERTY(QString currentServer READ currentServer WRITE setCurrentServer)
  Q_PROPERTY(QString ownNick READ ownNick WRITE setOwnNick)
  Q_PROPERTY(QList<IrcUser *> ircUsers READ ircUsers)
  
public:
  ServerInfo(QObject *parent = 0);
  ~ServerInfo();

  void setNetworkname(const QString &networkname);
  QString networkname() const;
  
  void setCurrentServer(const QString &currentServer);
  QString currentServer() const;
  
  void setOwnNick(const QString &ownnick);
  QString ownNick() const;
  
  QList<IrcUser *> ircUsers() const;
  IrcUser *newIrcUser(const QString &hostmask);
  IrcUser *ircUser(const QString &nickname) const;
  
  void setTopics(const QHash<QString, QString> &topics);
  QHash<QString, QString> topics() const;
  
  void updateTopic(const QString &channel, const QString &topic);
  QString topic(const QString &channel) const;
  
  void setSupports(const QHash<QString, QString> &supports);
  QHash<QString, QString> supports() const;
  QString supports(const QString &feature) const;
  
  bool isOwnNick(const QString &nick) const;
  bool isOwnNick(const IrcUser &ircuser) const;
  
private:
  QString networkname_;
  QString currentServer_;
  QString ownNick_;
  
  //QVariantMap networkSettings;
  //QVariantMap identity;
  
  QHash<QString, IrcUser *> ircUsers_;  // stores all known nicks for the server
  QHash<QString, QString> topics_; // stores topics for each buffer
  QHash<QString, QString> supports_;  // stores results from RPL_ISUPPORT
};

struct UnsupportedFeatureException : public Exception {};

#endif

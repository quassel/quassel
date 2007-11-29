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

#ifndef _IRCSERVERHANDLER_H_
#define _IRCSERVERHANDLER_H_

#include "basichandler.h"

class IrcServerHandler : public BasicHandler {
  Q_OBJECT

public:
  IrcServerHandler(Server *parent = 0);
  ~IrcServerHandler();

  void handleServerMsg(QByteArray rawMsg);
  
public slots:
  void handleJoin(QString, QStringList);
  void handleKick(QString, QStringList);
  void handleMode(QString, QStringList);
  void handleNick(QString, QStringList);
  void handleNotice(QString, QStringList);
  void handlePart(QString, QStringList);
  void handlePing(QString, QStringList);
  void handlePrivmsg(QString, QStringList);
  void handleQuit(QString, QStringList);
  void handleTopic(QString, QStringList);

  void handle001(QString, QStringList);   // RPL_WELCOME
  void handle005(QString, QStringList);   // RPL_ISUPPORT
  void handle331(QString, QStringList);   // RPL_NOTOPIC
  void handle332(QString, QStringList);   // RPL_TOPIC
  void handle333(QString, QStringList);   // Topic set by...
  void handle353(QString, QStringList);   // RPL_NAMREPLY
  void handle432(QString, QStringList);   // ERR_ERRONEUSNICKNAME
  void handle433(QString, QStringList);   // ERR_NICKNAMEINUSE

  void defaultHandler(QString cmd, QString prefix, QStringList params);
};


#endif

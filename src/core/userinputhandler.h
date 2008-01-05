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

#ifndef _USERINPUTHANDLER_H_
#define _USERINPUTHANDLER_H_

#include "basichandler.h"

class Server;

class UserInputHandler : public BasicHandler {
  Q_OBJECT

public:
  UserInputHandler(Server *parent = 0);

  void handleUserInput(QString buffer, QString msg);
  
public slots:
  void handleAway(QString, QString);
  void handleDeop(QString, QString);
  void handleDevoice(QString, QString);
  void handleInvite(QString, QString);
  void handleJoin(QString, QString);
  void handleKick(QString, QString);
  void handleList(QString, QString);
  void handleMode(QString, QString);
  void handleMsg(QString, QString);
  void handleNick(QString, QString);
  void handleOp(QString, QString);
  void handlePart(QString, QString);
  void handleQuery(QString, QString);
  void handleQuit(QString, QString);
  void handleQuote(QString, QString);
  void handleSay(QString, QString);
  void handleTopic(QString, QString);
  void handleVoice(QString, QString);
  void handleMe(QString, QString);

  void defaultHandler(QString cmd, QString buf, QString msg);

};


#endif

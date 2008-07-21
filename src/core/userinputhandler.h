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
  UserInputHandler(NetworkConnection *parent = 0);

  void handleUserInput(const BufferInfo &bufferInfo, const QString &text);
  
public slots:
  void handleAway(const BufferInfo &bufferInfo, const QString &text);
  void handleBan(const BufferInfo &bufferInfo, const QString &text);
  void handleCtcp(const BufferInfo &bufferInfo, const QString &text);
  void handleDeop(const BufferInfo &bufferInfo, const QString &text);
  void handleDevoice(const BufferInfo &bufferInfo, const QString &text);
  void handleInvite(const BufferInfo &bufferInfo, const QString &text);
  void handleJ(const BufferInfo &bufferInfo, const QString &text);
  void handleJoin(const BufferInfo &bufferInfo, const QString &text);
  void handleKick(const BufferInfo &bufferInfo, const QString &text);
  void handleKill(const BufferInfo &bufferInfo, const QString &text);
  void handleList(const BufferInfo &bufferInfo, const QString &text);
  void handleMe(const BufferInfo &bufferInfo, const QString &text);
  void handleMode(const BufferInfo &bufferInfo, const QString &text);
  void handleMsg(const BufferInfo &bufferInfo, const QString &text);
  void handleNick(const BufferInfo &bufferInfo, const QString &text);
  void handleOper(const BufferInfo &bufferInfo, const QString &text);
  void handleOp(const BufferInfo &bufferInfo, const QString &text);
  void handlePart(const BufferInfo &bufferInfo, const QString &text);
  void handlePing(const BufferInfo &bufferInfo, const QString &text);
  void handleQuery(const BufferInfo &bufferInfo, const QString &text);
  void handleQuit(const BufferInfo &bufferInfo, const QString &text);
  void handleQuote(const BufferInfo &bufferInfo, const QString &text);
  void handleSay(const BufferInfo &bufferInfo, const QString &text);
  void handleTopic(const BufferInfo &bufferInfo, const QString &text);
  void handleVoice(const BufferInfo &bufferInfo, const QString &text);
  void handleWho(const BufferInfo &bufferInfo, const QString &text);
  void handleWhois(const BufferInfo &bufferInfo, const QString &text);
  void handleWhowas(const BufferInfo &bufferInfo, const QString &text);

  void defaultHandler(QString cmd, const BufferInfo &bufferInfo, const QString &text);
};


#endif

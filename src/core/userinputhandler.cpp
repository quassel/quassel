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
#include "userinputhandler.h"

#include "util.h"

#include "networkconnection.h"
#include "network.h"
#include "ctcphandler.h"

#include <QDebug>

UserInputHandler::UserInputHandler(NetworkConnection *parent)
  : BasicHandler(parent) {
}

void UserInputHandler::handleUserInput(QString bufname, QString msg) {
  try {
    if(msg.isEmpty())
      return;
    QString cmd;
    if(!msg.startsWith('/')) {
      cmd = QString("SAY");
    } else {
      cmd = msg.section(' ', 0, 0).remove(0, 1).toUpper();
      msg = msg.section(' ', 1);
    }
    handle(cmd, Q_ARG(QString, bufname), Q_ARG(QString, msg));
  } catch(Exception e) {
    emit displayMsg(Message::Error, "", e.msg());
  }
}

// ====================
//  Public Slots
// ====================

void UserInputHandler::handleAway(QString bufname, QString msg) {
  emit putCmd("AWAY", QStringList(msg));
}

void UserInputHandler::handleBan(QString bufname, QString msg) {
  if(!isChannelName(bufname))
    return;
  
  //TODO: find suitable default hostmask if msg gives only nickname 
  // Example: MODE &oulu +b *!*@*
  QStringList banMsg(bufname+" +b "+msg);
  emit putCmd("MODE", banMsg);
}

void UserInputHandler::handleDeop(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufname << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handleDevoice(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufname << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handleInvite(QString bufname, QString msg) {
  QStringList params;
  params << msg << bufname;
  emit putCmd("INVITE", params);
}

void UserInputHandler::handleJoin(QString bufname, QString msg) {
  emit putCmd("JOIN", msg.split(" "));
}

void UserInputHandler::handleKick(QString bufname, QString msg) {
  QStringList params;
  params << bufname << msg.split(' ', QString::SkipEmptyParts);
  emit putCmd("KICK", params);
}

void UserInputHandler::handleList(QString bufname, QString msg) {
  emit putCmd("LIST", msg.split(' ', QString::SkipEmptyParts));
}


void UserInputHandler::handleMe(QString bufname, QString msg) {
  if(bufname.isEmpty()) return; // server buffer
  server->ctcpHandler()->query(bufname, "ACTION", msg);
  emit displayMsg(Message::Action, bufname, msg, network()->myNick());
}

void UserInputHandler::handleMode(QString bufname, QString msg) {
  emit putCmd("MODE", msg.split(' ', QString::SkipEmptyParts));
}

// TODO: show privmsgs
void UserInputHandler::handleMsg(QString bufname, QString msg) {
  QString nick = msg.section(" ", 0, 0);
  msg = msg.section(" ", 1);
  if(nick.isEmpty() || msg.isEmpty()) return;
  QStringList params;
  params << nick << msg;
  emit putCmd("PRIVMSG", params);
}

void UserInputHandler::handleNick(QString bufname, QString msg) {
  QString nick = msg.section(' ', 0, 0);
  emit putCmd("NICK", QStringList(nick));
}

void UserInputHandler::handleOp(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufname << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handlePart(QString bufname, QString msg) {
  QStringList params;
  params << bufname << msg;
  emit putCmd("PART", params);
}

// TODO: implement queries
void UserInputHandler::handleQuery(QString bufname, QString msg) {
  // QString nick = msg.section(' ', 0, 0);
  
  handleMsg(bufname, msg);
  
  // TODO: usenetworkids
//   if(!nick.isEmpty())
//     emit queryRequested(network, nick);
}

void UserInputHandler::handleQuit(QString bufname, QString msg) {
  emit putCmd("QUIT", QStringList(msg));
}

void UserInputHandler::handleQuote(QString bufname, QString msg) {
  emit putRawLine(msg);
}


void UserInputHandler::handleSay(QString bufname, QString msg) {
  if(bufname.isEmpty()) return;  // server buffer
  QStringList params;
  params << bufname << msg;
  emit putCmd("PRIVMSG", params);
  if(isChannelName(bufname)) {
    emit displayMsg(Message::Plain, params[0], msg, network()->myNick(), Message::Self);
  } else {
    emit displayMsg(Message::Plain, params[0], msg, network()->myNick(), Message::Self|Message::PrivMsg);
  }
}


void UserInputHandler::handleTopic(QString bufname, QString msg) {
  if(bufname.isEmpty()) return;
  QStringList params;
  params << bufname << msg;
  emit putCmd("TOPIC", params);
}

void UserInputHandler::handleVoice(QString bufname, QString msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufname << m << nicks;
  emit putCmd("MODE", params);
}


void UserInputHandler::handleWho(QString bufname, QString msg) {
  emit putCmd("WHO", QStringList(msg));
}


void UserInputHandler::handleWhois(QString bufname, QString msg) {
  emit putCmd("WHOIS", QStringList(msg));
}


void UserInputHandler::handleWhowas(QString bufname, QString msg) {
  emit putCmd("WHOWAS", QStringList(msg));
}

void UserInputHandler::defaultHandler(QString cmd, QString bufname, QString msg) {
  emit displayMsg(Message::Error, "", QString("Error: %1 %2").arg(cmd).arg(msg));
}



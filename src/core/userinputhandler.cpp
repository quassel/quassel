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

void UserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg_) {
  try {
    if(msg_.isEmpty())
      return;
    QString cmd;
    QString msg = msg_;
    if(!msg.startsWith('/')) {
      cmd = QString("SAY");
    } else {
      cmd = msg.section(' ', 0, 0).remove(0, 1).toUpper();
      msg = msg.section(' ', 1);
    }
    handle(cmd, Q_ARG(BufferInfo, bufferInfo), Q_ARG(QString, msg));
  } catch(Exception e) {
    emit displayMsg(Message::Error, bufferInfo.type(), "", e.msg());
  }
}

// ====================
//  Public Slots
// ====================

void UserInputHandler::handleAway(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("AWAY", QStringList(msg));
}

void UserInputHandler::handleBan(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.type() != BufferInfo::ChannelBuffer)
    return;
  
  //TODO: find suitable default hostmask if msg gives only nickname 
  // Example: MODE &oulu +b *!*@*
  QStringList banMsg(bufferInfo.bufferName()+" +b "+msg);
  emit putCmd("MODE", banMsg);
}

void UserInputHandler::handleCtcp(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString nick = msg.section(' ', 0, 0);
  QString ctcpTag = msg.section(' ', 1, 1).toUpper();
  if (ctcpTag.isEmpty()) return;
  QString message = "";
  QString verboseMessage = tr("sending CTCP-%1-request").arg(ctcpTag);

  if(ctcpTag == "PING") {
    uint now = QDateTime::currentDateTime().toTime_t();
    message = QString::number(now);
  }

  server->ctcpHandler()->query(nick, ctcpTag, message);
  emit displayMsg(Message::Action, BufferInfo::StatusBuffer, "", verboseMessage, network()->myNick());
}

void UserInputHandler::handleDeop(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handleDevoice(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handleInvite(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList params;
  params << msg << bufferInfo.bufferName();
  emit putCmd("INVITE", params);
}

void UserInputHandler::handleJ(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QStringList params = msg.split(" ");
  if(params.size() > 0 && !params[0].startsWith("#")) {
    params[0] = QString("#%1").arg(params[0]);
  }
  emit putCmd("JOIN", params);
}

void UserInputHandler::handleJoin(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("JOIN", msg.split(" "));
}

void UserInputHandler::handleKick(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList params;
  params << bufferInfo.bufferName() << msg.split(' ', QString::SkipEmptyParts);
  emit putCmd("KICK", params);
}

void UserInputHandler::handleList(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("LIST", msg.split(' ', QString::SkipEmptyParts));
}


void UserInputHandler::handleMe(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return; // server buffer
  server->ctcpHandler()->query(bufferInfo.bufferName(), "ACTION", msg);
  emit displayMsg(Message::Action, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick());
}

void UserInputHandler::handleMode(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("MODE", msg.split(' ', QString::SkipEmptyParts));
}

// TODO: show privmsgs
void UserInputHandler::handleMsg(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  if(!msg.contains(' '))
    return;
      
  QStringList params;
  params << msg.section(' ', 0, 0);
  params << msg.section(' ', 1);

  emit putCmd("PRIVMSG", params);
}

void UserInputHandler::handleNick(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString nick = msg.section(' ', 0, 0);
  emit putCmd("NICK", QStringList(nick));
}

void UserInputHandler::handleOp(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handlePart(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList params;
  params << bufferInfo.bufferName() << msg;
  emit putCmd("PART", params);
}

// TODO: implement queries
void UserInputHandler::handleQuery(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString target = msg.section(' ', 0, 0);
  QString message = msg.section(' ', 1);
  if(message.isEmpty())
    emit displayMsg(Message::Server, BufferInfo::QueryBuffer, target, "Starting query with " + target, network()->myNick(), Message::Self);
  else
    emit displayMsg(Message::Plain, BufferInfo::QueryBuffer, target, message, network()->myNick(), Message::Self);
  handleMsg(bufferInfo, msg);
}

void UserInputHandler::handleQuit(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("QUIT", QStringList(msg));
}

void UserInputHandler::handleQuote(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putRawLine(msg);
}

void UserInputHandler::handleSay(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return;  // server buffer
  QStringList params;
  params << bufferInfo.bufferName() << msg;
  emit putCmd("PRIVMSG", params);
  emit displayMsg(Message::Plain, bufferInfo.type(), params[0], msg, network()->myNick(), Message::Self);
}

void UserInputHandler::handleTopic(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return;
  QStringList params;
  params << bufferInfo.bufferName() << msg;
  emit putCmd("TOPIC", params);
}

void UserInputHandler::handleVoice(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", params);
}

void UserInputHandler::handleWho(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("WHO", msg.split(' '));
}

void UserInputHandler::handleWhois(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("WHOIS", msg.split(' '));
}

void UserInputHandler::handleWhowas(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("WHOWAS", msg.split(' '));
}

void UserInputHandler::defaultHandler(QString cmd, const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: %1 %2").arg(cmd).arg(msg));
}



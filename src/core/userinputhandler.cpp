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
#include "ircuser.h"

#include <QDebug>

UserInputHandler::UserInputHandler(NetworkConnection *parent) : BasicHandler(parent) {
}

void UserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg_) {
  try {
    if(msg_.isEmpty())
      return;
    QString cmd;
    QString msg = msg_;
    // leading slashes indicate there's a command to call unless there is anothere one in the first section (like a path /proc/cpuinfo)
    int secondSlashPos = msg.indexOf('/', 1);
    int firstSpacePos = msg.indexOf(' ');
    if(!msg.startsWith('/') || (secondSlashPos != -1 && (secondSlashPos < firstSpacePos || firstSpacePos == -1))) {
      if(msg.startsWith("//"))
	msg.remove(0, 1); // //asdf is transformed to /asdf
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

  QString awayMsg = msg;
  IrcUser *me = network()->me();

  // if there is no message supplied we have to check if we are already away or not
  if(msg.isEmpty()) {
    if(me && !me->isAway())
      awayMsg = networkConnection()->identity()->awayReason();
  }
  if(me)
    me->setAwayMessage(awayMsg);
  
  putCmd("AWAY", serverEncode(awayMsg));
}

void UserInputHandler::handleBan(const BufferInfo &bufferInfo, const QString &msg) {
  QString banChannel;
  QString banUser;

  QStringList params = msg.split(" ");

  if(!params.isEmpty() && isChannelName(params[0])) {
    banChannel = params.takeFirst();
  } else if(bufferInfo.type() == BufferInfo::ChannelBuffer) {
    banChannel = bufferInfo.bufferName();
  } else {
    emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: channel unknown in command: /BAN %1").arg(msg));
    return;
  }

  if(!params.isEmpty() && !params.contains("!") && network()->ircUser(params[0])) {
    IrcUser *ircuser = network()->ircUser(params[0]);
    // generalizedHost changes <nick> to  *!ident@*.sld.tld.
    QString generalizedHost = ircuser->host();
    if(generalizedHost.isEmpty()) {
      emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: host unknown in command: /BAN %1").arg(msg));
      return;
    }

    if(generalizedHost.lastIndexOf(".") != -1 && generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".")-1) != -1) {
      int secondLastPeriodPosition = generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".")-1);
      generalizedHost.replace(0, secondLastPeriodPosition, "*");
    }
    banUser = QString("*!%1@%2").arg(ircuser->user()).arg(generalizedHost);
  } else {
    banUser = params.join(" ");
  }

  QString banMsg = QString("MODE %1 +b %2").arg(banChannel).arg(banUser);
  emit putRawLine(serverEncode(banMsg));
}

void UserInputHandler::handleCtcp(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString nick = msg.section(' ', 0, 0);
  QString ctcpTag = msg.section(' ', 1, 1).toUpper();
  if (ctcpTag.isEmpty()) return;
  QString message = "";
  QString verboseMessage = tr("sending CTCP-%1 request").arg(ctcpTag);

  if(ctcpTag == "PING") {
    uint now = QDateTime::currentDateTime().toTime_t();
    message = QString::number(now);
  }

  networkConnection()->ctcpHandler()->query(nick, ctcpTag, message);
  emit displayMsg(Message::Action, BufferInfo::StatusBuffer, "", verboseMessage, network()->myNick());
}

void UserInputHandler::handleDeop(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", serverEncode(params));
}

void UserInputHandler::handleDevoice(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "-"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", serverEncode(params));
}

void UserInputHandler::handleInvite(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList params;
  params << msg << bufferInfo.bufferName();
  emit putCmd("INVITE", serverEncode(params));
}

void UserInputHandler::handleJoin(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QStringList params = msg.trimmed().split(" ");
  QStringList chans = params[0].split(",");
  QStringList keys;
  if(params.count() > 1) keys = params[1].split(",");
  emit putCmd("JOIN", serverEncode(params)); // FIXME handle messages longer than 512 bytes!
  int i = 0;
  for(; i < keys.count(); i++) {
    if(i >= chans.count()) break;
    networkConnection()->addChannelKey(chans[i], keys[i]);
  }
  for(; i < chans.count(); i++) {
    networkConnection()->removeChannelKey(chans[i]);
  }
}

void UserInputHandler::handleKick(const BufferInfo &bufferInfo, const QString &msg) {
  QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
  QString reason = msg.section(' ', 1, -1, QString::SectionSkipEmpty).trimmed();
  if(reason.isEmpty())
    reason = networkConnection()->identity()->kickReason();

  QList<QByteArray> params;
  params << serverEncode(bufferInfo.bufferName()) << serverEncode(nick) << channelEncode(bufferInfo.bufferName(), reason);
  emit putCmd("KICK", params);
}

void UserInputHandler::handleKill(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
  QString pass = msg.section(' ', 1, -1, QString::SectionSkipEmpty);
  QList<QByteArray> params;
  params << serverEncode(nick) << serverEncode(pass);
  emit putCmd("KILL", params);
}


void UserInputHandler::handleList(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("LIST", serverEncode(msg.split(' ', QString::SkipEmptyParts)));
}

void UserInputHandler::handleMe(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return; // server buffer
  networkConnection()->ctcpHandler()->query(bufferInfo.bufferName(), "ACTION", msg);
  emit displayMsg(Message::Action, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick());
}

void UserInputHandler::handleMode(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)

  QStringList params = msg.split(' ', QString::SkipEmptyParts);
  // if the first argument is neither a channel nor us (user modes are only to oneself) the current buffer is assumed to be the target
  if(!params.isEmpty() && !network()->isChannelName(params[0]) && !network()->isMyNick(params[0]))
    params.prepend(bufferInfo.bufferName());
  
  // TODO handle correct encoding for buffer modes (channelEncode())
  emit putCmd("MODE", serverEncode(params));
}

// TODO: show privmsgs
void UserInputHandler::handleMsg(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo);
  if(!msg.contains(' '))
    return;

  QList<QByteArray> params;
  params << serverEncode(msg.section(' ', 0, 0));
  params << userEncode(params[0], msg.section(' ', 1));

  emit putCmd("PRIVMSG", params);
}

void UserInputHandler::handleNick(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString nick = msg.section(' ', 0, 0);
  emit putCmd("NICK", serverEncode(nick));
}

void UserInputHandler::handleOp(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'o';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", serverEncode(params));
}

void UserInputHandler::handleOper(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putRawLine(serverEncode(QString("OPER %1").arg(msg)));
}

void UserInputHandler::handlePart(const BufferInfo &bufferInfo, const QString &msg) {
  QList<QByteArray> params;
  QString partReason;

  // msg might contain either a channel name and/or a reaon, so we have to check if the first word is a known channel
  QString channelName = msg.section(' ', 0, 0);
  if(channelName.isEmpty() || !network()->ircChannel(channelName)) {
    channelName = bufferInfo.bufferName();
    partReason = msg;
  } else {
    partReason = msg.mid(channelName.length() + 1);
  }
  
  if(partReason.isEmpty())
    partReason = networkConnection()->identity()->partReason();

  params << serverEncode(channelName) << channelEncode(bufferInfo.bufferName(), partReason);
  emit putCmd("PART", params);
}

void UserInputHandler::handlePing(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)

  QString param = msg;
  if(param.isEmpty())
    param = QTime::currentTime().toString("hh:mm:ss.zzz");

  putCmd("PING", serverEncode(param));
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

  QString quitReason;
  if(msg.isEmpty())
    quitReason = networkConnection()->identity()->quitReason();
  else
    quitReason = msg;

  emit putCmd("QUIT", serverEncode(quitReason));
  networkConnection()->disconnectFromIrc();
}

void UserInputHandler::handleQuote(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putRawLine(serverEncode(msg));
}

void UserInputHandler::handleSay(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return;  // server buffer
  QList<QByteArray> params;
  params << serverEncode(bufferInfo.bufferName()) << channelEncode(bufferInfo.bufferName(), msg);
  emit putCmd("PRIVMSG", params);
  emit displayMsg(Message::Plain, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
}

void UserInputHandler::handleTopic(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return;
  if(!msg.isEmpty()) {
    QList<QByteArray> params;
    params << serverEncode(bufferInfo.bufferName()) << channelEncode(bufferInfo.bufferName(), msg);
    emit putCmd("TOPIC", params);
  } else {
    emit networkConnection()->putRawLine("TOPIC " + serverEncode(bufferInfo.bufferName()) + " :");
  }
}

void UserInputHandler::handleVoice(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", serverEncode(params));
}

void UserInputHandler::handleWho(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("WHO", serverEncode(msg.split(' ')));
}

void UserInputHandler::handleWhois(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("WHOIS", serverEncode(msg.split(' ')));
}

void UserInputHandler::handleWhowas(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putCmd("WHOWAS", serverEncode(msg.split(' ')));
}

void UserInputHandler::defaultHandler(QString cmd, const BufferInfo &bufferInfo, const QString &msg) {
  for(int i = 0; i < coreSession()->aliasManager().count(); i++) {
    if(coreSession()->aliasManager()[i].name.toLower() == cmd.toLower()) {
      expand(coreSession()->aliasManager()[i].expansion, bufferInfo, msg);
      return;
    }
  }
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: %1 %2").arg(cmd).arg(msg));
}

void UserInputHandler::expand(const QString &alias, const BufferInfo &bufferInfo, const QString &msg) {
  QStringList commands = alias.split(QRegExp("; ?"));
  QStringList params = msg.split(' ');
  for(int i = 0; i < commands.count(); i++) {
    QString command = commands[i];
    for(int j = params.count(); j > 0; j--) {
      command = command.replace(QString("$%1").arg(j), params[j - 1]);
    }
    command = command.replace("$0", msg);
    command = command.replace("$channelname", bufferInfo.bufferName());
    command = command.replace("$currentnick", network()->myNick());
    handleUserInput(bufferInfo, command);
  }
}




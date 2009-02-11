/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "ctcphandler.h"
#include "coreidentity.h"
#include "ircuser.h"

#include <QDebug>
#include <QRegExp>

UserInputHandler::UserInputHandler(CoreNetwork *parent)
  : BasicHandler(parent)
{
}

void UserInputHandler::handleUserInput(const BufferInfo &bufferInfo, const QString &msg_) {
  if(msg_.isEmpty())
    return;
  QString cmd;
  QString msg = msg_;
  // leading slashes indicate there's a command to call unless there is another one in the first section (like a path /proc/cpuinfo)
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
      awayMsg = network()->identityPtr()->awayReason();
  }
  if(me)
    me->setAwayMessage(awayMsg);

  putCmd("AWAY", serverEncode(awayMsg));
}

void UserInputHandler::handleBan(const BufferInfo &bufferInfo, const QString &msg) {
  banOrUnban(bufferInfo, msg, true);
}

void UserInputHandler::handleUnban(const BufferInfo &bufferInfo, const QString &msg) {
  banOrUnban(bufferInfo, msg, false);
}

void UserInputHandler::banOrUnban(const BufferInfo &bufferInfo, const QString &msg, bool ban) {
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

    static QRegExp ipAddress("\\d+\\.\\d+\\.\\d+\\.\\d+");
    if(ipAddress.exactMatch(generalizedHost))    {
        int lastDotPos = generalizedHost.lastIndexOf('.') + 1;
        generalizedHost.replace(lastDotPos, generalizedHost.length() - lastDotPos, '*');
    } else if(generalizedHost.lastIndexOf(".") != -1 && generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".")-1) != -1) {
      int secondLastPeriodPosition = generalizedHost.lastIndexOf(".", generalizedHost.lastIndexOf(".")-1);
      generalizedHost.replace(0, secondLastPeriodPosition, "*");
    }
    banUser = QString("*!%1@%2").arg(ircuser->user(), generalizedHost);
  } else {
    banUser = params.join(" ");
  }

  QString banMode = ban ? "+b" : "-b";
  QString banMsg = QString("MODE %1 %2 %3").arg(banChannel, banMode, banUser);
  emit putRawLine(serverEncode(banMsg));
}

void UserInputHandler::handleCtcp(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)

  QString nick = msg.section(' ', 0, 0);
  QString ctcpTag = msg.section(' ', 1, 1).toUpper();
  if(ctcpTag.isEmpty())
    return;

  QString message = "";
  QString verboseMessage = tr("sending CTCP-%1 request").arg(ctcpTag);

  if(ctcpTag == "PING") {
    uint now = QDateTime::currentDateTime().toTime_t();
    message = QString::number(now);
  }

  network()->ctcpHandler()->query(nick, ctcpTag, message);
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
  Q_UNUSED(bufferInfo);

  // trim spaces before chans or keys
  QString sane_msg = msg;
  sane_msg.replace(QRegExp(", +"), ",");
  QStringList params = sane_msg.trimmed().split(" ");
  QStringList chans = params[0].split(",");
  QStringList keys;
  int i;
  for(i = 0; i < chans.count(); i++) {
    if(!network()->isChannelName(chans[i]))
      chans[i].prepend('#');
  }
  params[0] = chans.join(",");
  if(params.count() > 1) keys = params[1].split(",");
  emit putCmd("JOIN", serverEncode(params)); // FIXME handle messages longer than 512 bytes!
  i = 0;
  for(; i < keys.count(); i++) {
    if(i >= chans.count()) break;
    network()->addChannelKey(chans[i], keys[i]);
  }
  for(; i < chans.count(); i++) {
    network()->removeChannelKey(chans[i]);
  }
}

void UserInputHandler::handleKick(const BufferInfo &bufferInfo, const QString &msg) {
  QString nick = msg.section(' ', 0, 0, QString::SectionSkipEmpty);
  QString reason = msg.section(' ', 1, -1, QString::SectionSkipEmpty).trimmed();
  if(reason.isEmpty())
    reason = network()->identityPtr()->kickReason();

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
  network()->ctcpHandler()->query(bufferInfo.bufferName(), "ACTION", msg);
  emit displayMsg(Message::Action, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
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

  QByteArray target = serverEncode(msg.section(' ', 0, 0));
  putPrivmsg(target, userEncode(target, msg.section(' ', 1)));
}

void UserInputHandler::handleNick(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  QString nick = msg.section(' ', 0, 0);
  emit putCmd("NICK", serverEncode(nick));
}

void UserInputHandler::handleNotice(const BufferInfo &bufferInfo, const QString &msg) {
  QString bufferName = msg.section(' ', 0, 0);
  QString payload = msg.section(' ', 1);
  QList<QByteArray> params;
  params << serverEncode(bufferName) << channelEncode(bufferInfo.bufferName(), payload);
  emit putCmd("NOTICE", params);
  emit displayMsg(Message::Notice, bufferName, payload, network()->myNick(), Message::Self);
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
    partReason = network()->identityPtr()->partReason();

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
  network()->disconnectFromIrc(true, msg);
}

void UserInputHandler::issueQuit(const QString &reason) {
  emit putCmd("QUIT", serverEncode(reason));
}

void UserInputHandler::handleQuote(const BufferInfo &bufferInfo, const QString &msg) {
  Q_UNUSED(bufferInfo)
  emit putRawLine(serverEncode(msg));
}

void UserInputHandler::handleSay(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty())
    return;  // server buffer
  putPrivmsg(serverEncode(bufferInfo.bufferName()), channelEncode(bufferInfo.bufferName(), msg));
  emit displayMsg(Message::Plain, bufferInfo.type(), bufferInfo.bufferName(), msg, network()->myNick(), Message::Self);
}

void UserInputHandler::handleTopic(const BufferInfo &bufferInfo, const QString &msg) {
  if(bufferInfo.bufferName().isEmpty()) return;
  QList<QByteArray> params;
  params << serverEncode(bufferInfo.bufferName());
  if(!msg.isEmpty())
    params << channelEncode(bufferInfo.bufferName(), msg);
  emit putCmd("TOPIC", params);
}

void UserInputHandler::handleVoice(const BufferInfo &bufferInfo, const QString &msg) {
  QStringList nicks = msg.split(' ', QString::SkipEmptyParts);
  QString m = "+"; for(int i = 0; i < nicks.count(); i++) m += 'v';
  QStringList params;
  params << bufferInfo.bufferName() << m << nicks;
  emit putCmd("MODE", serverEncode(params));
}

void UserInputHandler::handleWait(const BufferInfo &bufferInfo, const QString &msg) {
  int splitPos = msg.indexOf(';');
  if(splitPos <= 0)
    return;

  bool ok;
  int delay = msg.left(splitPos).trimmed().toInt(&ok);
  if(!ok)
    return;

  delay *= 1000;

  QString command = msg.mid(splitPos + 1).trimmed();
  if(command.isEmpty())
    return;

  _delayedCommands[startTimer(delay)] = Command(bufferInfo, command);
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
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", QString("Error: %1 %2").arg(cmd, msg));
}

void UserInputHandler::expand(const QString &alias, const BufferInfo &bufferInfo, const QString &msg) {
  QRegExp paramRangeR("\\$(\\d+)\\.\\.(\\d*)");
  QStringList commands = alias.split(QRegExp("; ?"));
  QStringList params = msg.split(' ');
  QStringList expandedCommands;
  for(int i = 0; i < commands.count(); i++) {
    QString command = commands[i];

    // replace ranges like $1..3
    if(!params.isEmpty()) {
      int pos;
      while((pos = paramRangeR.indexIn(command)) != -1) {
	int start = paramRangeR.cap(1).toInt();
	bool ok;
	int end = paramRangeR.cap(2).toInt(&ok);
	if(!ok) {
	  end = params.count();
	}
	if(end < start)
	  command = command.replace(pos, paramRangeR.matchedLength(), QString());
	else {
	  command = command.replace(pos, paramRangeR.matchedLength(), QStringList(params.mid(start - 1, end - start + 1)).join(" "));
	}
      }
    }

    for(int j = params.count(); j > 0; j--) {
      IrcUser *ircUser = network()->ircUser(params[j - 1]);
      command = command.replace(QString("$%1:hostname").arg(j), ircUser ? ircUser->host() : QString("*"));
      command = command.replace(QString("$%1").arg(j), params[j - 1]);
    }
    command = command.replace("$0", msg);
    command = command.replace("$channelname", bufferInfo.bufferName());
    command = command.replace("$currentnick", network()->myNick());
    expandedCommands << command;
  }

  while(!expandedCommands.isEmpty()) {
    QString command;
    if(expandedCommands[0].trimmed().toLower().startsWith("/wait")) {
      command = expandedCommands.join("; ");
      expandedCommands.clear();
    } else {
      command = expandedCommands.takeFirst();
    }
    handleUserInput(bufferInfo, command);
  }
}


void UserInputHandler::putPrivmsg(const QByteArray &target, const QByteArray &message) {
  static const char *cmd = "PRIVMSG";
  int overrun = lastParamOverrun(cmd, QList<QByteArray>() << message);
  if(overrun) {
    static const char *splitter = " .,-";
    int maxSplitPos = message.count() - overrun;
    int splitPos = -1;
    for(const char *splitChar = splitter; *splitChar != 0; splitChar++) {
      splitPos = qMax(splitPos, message.lastIndexOf(*splitChar, maxSplitPos));
    }
    if(splitPos <= 0) {
      splitPos = maxSplitPos;
    }
    putCmd(cmd, QList<QByteArray>() << target << message.left(splitPos));
    putPrivmsg(target, message.mid(splitPos));
    return;
  } else {
    putCmd(cmd, QList<QByteArray>() << target << message);
  }
}

// returns 0 if the message will not be chopped by the irc server or number of chopped bytes if message is too long
int UserInputHandler::lastParamOverrun(const QString &cmd, const QList<QByteArray> &params) {
  // the server will pass our message trunkated to 512 bytes including CRLF with the following format:
  // ":prefix COMMAND param0 param1 :lastparam"
  // where prefix = "nickname!user@host"
  // that means that the last message can be as long as:
  // 512 - nicklen - userlen - hostlen - commandlen - sum(param[0]..param[n-1])) - 2 (for CRLF) - 4 (":!@" + 1space between prefix and command) - max(paramcount - 1, 0) (space for simple params) - 2 (space and colon for last param)
  IrcUser *me = network()->me();
  int maxLen = 480 - cmd.toAscii().count(); // educated guess in case we don't know us (yet?)

  if(me)
    maxLen = 512 - serverEncode(me->nick()).count() - serverEncode(me->user()).count() - serverEncode(me->host()).count() - cmd.toAscii().count() - 6;

  if(!params.isEmpty()) {
    for(int i = 0; i < params.count() - 1; i++) {
      maxLen -= (params[i].count() + 1);
    }
    maxLen -= 2; // " :" last param separator;

    if(params.last().count() > maxLen) {
      return params.last().count() - maxLen;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}


void UserInputHandler::timerEvent(QTimerEvent *event) {
  if(!_delayedCommands.contains(event->timerId())) {
    QObject::timerEvent(event);
    return;
  }
  BufferInfo bufferInfo = _delayedCommands[event->timerId()].bufferInfo;
  QString rawCommand = _delayedCommands[event->timerId()].command;
  _delayedCommands.remove(event->timerId());
  event->accept();

  // the stored command might be the result of an alias expansion, so we need to split it up again
  QStringList commands = rawCommand.split(QRegExp("; ?"));
  foreach(QString command, commands) {
    handleUserInput(bufferInfo, command);
  }
}

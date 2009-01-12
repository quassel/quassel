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
#include "ctcphandler.h"

#include "message.h"
#include "network.h"
#include "quassel.h"
#include "util.h"

CtcpHandler::CtcpHandler(CoreNetwork *parent)
  : BasicHandler(parent),
    XDELIM("\001")
{

  QByteArray MQUOTE = QByteArray("\020");
  ctcpMDequoteHash[MQUOTE + '0'] = QByteArray(1, '\000');
  ctcpMDequoteHash[MQUOTE + 'n'] = QByteArray(1, '\n');
  ctcpMDequoteHash[MQUOTE + 'r'] = QByteArray(1, '\r');
  ctcpMDequoteHash[MQUOTE + MQUOTE] = MQUOTE;

  QByteArray XQUOTE = QByteArray("\134");
  ctcpXDelimDequoteHash[XQUOTE + XQUOTE] = XQUOTE;
  ctcpXDelimDequoteHash[XQUOTE + QByteArray("a")] = XDELIM;
}

QByteArray CtcpHandler::lowLevelQuote(const QByteArray &message) {
  QByteArray quotedMessage = message;

  QHash<QByteArray, QByteArray> quoteHash = ctcpMDequoteHash;
  QByteArray MQUOTE = QByteArray("\020");
  quoteHash.remove(MQUOTE + MQUOTE);
  quotedMessage.replace(MQUOTE, MQUOTE + MQUOTE);

  QHash<QByteArray, QByteArray>::const_iterator quoteIter = quoteHash.constBegin();
  while(quoteIter != quoteHash.constEnd()) {
    quotedMessage.replace(quoteIter.value(), quoteIter.key());
    quoteIter++;
  }
  return quotedMessage;
}

QByteArray CtcpHandler::lowLevelDequote(const QByteArray &message) {
  QByteArray dequotedMessage;
  QByteArray messagepart;
  QHash<QByteArray, QByteArray>::iterator ctcpquote;

  // copy dequote Message
  for(int i = 0; i < message.size(); i++) {
    messagepart = message.mid(i,1);
    if(i+1 < message.size()) {
      for(ctcpquote = ctcpMDequoteHash.begin(); ctcpquote != ctcpMDequoteHash.end(); ++ctcpquote) {
        if(message.mid(i,2) == ctcpquote.key()) {
          messagepart = ctcpquote.value();
          i++;
          break;
        }
      }
    }
    dequotedMessage += messagepart;
  }
  return dequotedMessage;
}

QByteArray CtcpHandler::xdelimQuote(const QByteArray &message) {
  QByteArray quotedMessage = message;
  QHash<QByteArray, QByteArray>::const_iterator quoteIter = ctcpXDelimDequoteHash.constBegin();
  while(quoteIter != ctcpXDelimDequoteHash.constEnd()) {
    quotedMessage.replace(quoteIter.value(), quoteIter.key());
    quoteIter++;
  }
  return quotedMessage;
}

QByteArray CtcpHandler::xdelimDequote(const QByteArray &message) {
  QByteArray dequotedMessage;
  QByteArray messagepart;
  QHash<QByteArray, QByteArray>::iterator xdelimquote;

  for(int i = 0; i < message.size(); i++) {
    messagepart = message.mid(i,1);
    if(i+1 < message.size()) {
      for(xdelimquote = ctcpXDelimDequoteHash.begin(); xdelimquote != ctcpXDelimDequoteHash.end(); ++xdelimquote) {
        if(message.mid(i,2) == xdelimquote.key()) {
          messagepart = xdelimquote.value();
          i++;
          break;
        }
      }
    }
    dequotedMessage += messagepart;
  }
  return dequotedMessage;
}

void CtcpHandler::parse(Message::Type messageType, const QString &prefix, const QString &target, const QByteArray &message) {
  QByteArray ctcp;

  //lowlevel message dequote
  QByteArray dequotedMessage = lowLevelDequote(message);

  CtcpType ctcptype = messageType == Message::Notice
    ? CtcpReply
    : CtcpQuery;

  Message::Flags flags = (messageType == Message::Notice && !network()->isChannelName(target))
    ? Message::Redirected
    : Message::None;

  // extract tagged / extended data
  int xdelimPos = -1;
  int xdelimEndPos = -1;
  int spacePos = -1;
  while((xdelimPos = dequotedMessage.indexOf(XDELIM)) != -1) {
    if(xdelimPos > 0)
      displayMsg(messageType, target, userDecode(target, dequotedMessage.left(xdelimPos)), prefix, flags);

    xdelimEndPos = dequotedMessage.indexOf(XDELIM, xdelimPos + 1);
    if(xdelimEndPos == -1) {
      // no matching end delimiter found... treat rest of the message as ctcp
      xdelimEndPos = dequotedMessage.count();
    }
    ctcp = xdelimDequote(dequotedMessage.mid(xdelimPos + 1, xdelimEndPos - xdelimPos - 1));
    dequotedMessage = dequotedMessage.mid(xdelimEndPos + 1);

    //dispatch the ctcp command
    QString ctcpcmd = userDecode(target, ctcp.left(spacePos));
    QString ctcpparam = userDecode(target, ctcp.mid(spacePos + 1));

    spacePos = ctcp.indexOf(' ');
    if(spacePos != -1) {
      ctcpcmd = userDecode(target, ctcp.left(spacePos));
      ctcpparam = userDecode(target, ctcp.mid(spacePos + 1));
    } else {
      ctcpcmd = userDecode(target, ctcp);
      ctcpparam = QString();
    }

    handle(ctcpcmd, Q_ARG(CtcpType, ctcptype), Q_ARG(QString, prefix), Q_ARG(QString, target), Q_ARG(QString, ctcpparam));
  }

  if(!dequotedMessage.isEmpty())
    displayMsg(messageType, target, userDecode(target, dequotedMessage), prefix, flags);
}

QByteArray CtcpHandler::pack(const QByteArray &ctcpTag, const QByteArray &message) {
  return XDELIM + ctcpTag + ' ' + xdelimQuote(message) + XDELIM;
}

void CtcpHandler::query(const QString &bufname, const QString &ctcpTag, const QString &message) {
  QList<QByteArray> params;
  params << serverEncode(bufname) << lowLevelQuote(pack(serverEncode(ctcpTag), userEncode(bufname, message)));
  emit putCmd("PRIVMSG", params);
}

void CtcpHandler::reply(const QString &bufname, const QString &ctcpTag, const QString &message) {
  QList<QByteArray> params;
  params << serverEncode(bufname) << lowLevelQuote(pack(serverEncode(ctcpTag), userEncode(bufname, message)));
  emit putCmd("NOTICE", params);
}

//******************************/
// CTCP HANDLER
//******************************/
void CtcpHandler::handleAction(CtcpType ctcptype, const QString &prefix, const QString &target, const QString &param) {
  Q_UNUSED(ctcptype)
  emit displayMsg(Message::Action, typeByTarget(target), target, param, prefix);
}

void CtcpHandler::handlePing(CtcpType ctcptype, const QString &prefix, const QString &target, const QString &param) {
  Q_UNUSED(target)
  if(ctcptype == CtcpQuery) {
    reply(nickFromMask(prefix), "PING", param);
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Received CTCP PING request from %1").arg(prefix));
  } else {
    // display ping answer
    uint now = QDateTime::currentDateTime().toTime_t();
    uint then = QDateTime().fromTime_t(param.toInt()).toTime_t();
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Received CTCP PING answer from %1 with %2 seconds round trip time").arg(prefix).arg(now-then));
  }
}

void CtcpHandler::handleVersion(CtcpType ctcptype, const QString &prefix, const QString &target, const QString &param) {
  Q_UNUSED(target)
  if(ctcptype == CtcpQuery) {
    reply(nickFromMask(prefix), "VERSION", QString("Quassel IRC %1 (built on %2) -- http://www.quassel-irc.org")
        .arg(Quassel::buildInfo().plainVersionString)
        .arg(Quassel::buildInfo().buildDate));
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Received CTCP VERSION request by %1").arg(prefix));
  } else {
    // display Version answer
    emit displayMsg(Message::Server, BufferInfo::StatusBuffer, "", tr("Received CTCP VERSION answer from %1: %2").arg(prefix).arg(param));
  }
}

void CtcpHandler::defaultHandler(const QString &cmd, CtcpType ctcptype, const QString &prefix, const QString &target, const QString &param) {
  Q_UNUSED(ctcptype);
  Q_UNUSED(target);
  Q_UNUSED(param);
  emit displayMsg(Message::Error, BufferInfo::StatusBuffer, "", tr("Received unknown CTCP %1 by %2").arg(cmd).arg(prefix));
}



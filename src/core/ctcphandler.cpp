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
#include "ctcphandler.h"

#include "util.h"
#include "message.h"

CtcpHandler::CtcpHandler(NetworkConnection *parent)
  : BasicHandler(parent) {

  QString MQUOTE = QString('\020');
  ctcpMDequoteHash[MQUOTE + '0'] = QString('\000');
  ctcpMDequoteHash[MQUOTE + 'n'] = QString('\n');
  ctcpMDequoteHash[MQUOTE + 'r'] = QString('\r');
  ctcpMDequoteHash[MQUOTE + MQUOTE] = MQUOTE;

  XDELIM = QString('\001');
  QString XQUOTE = QString('\134');
  ctcpXDelimDequoteHash[XQUOTE + XQUOTE] = XQUOTE;
  ctcpXDelimDequoteHash[XQUOTE + QString('a')] = XDELIM;
}

QString CtcpHandler::dequote(QString message) {
  QString dequotedMessage;
  QString messagepart;
  QHash<QString, QString>::iterator ctcpquote;
  
  // copy dequote Message
  for(int i = 0; i < message.size(); i++) {
    messagepart = message[i];
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


QString CtcpHandler::XdelimDequote(QString message) {
  QString dequotedMessage;
  QString messagepart;
  QHash<QString, QString>::iterator xdelimquote;

  for(int i = 0; i < message.size(); i++) {
    messagepart = message[i];
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

QStringList CtcpHandler::parse(CtcpType ctcptype, QString prefix, QString target, QString message) {
  QStringList messages;
  QString ctcp;
  
  //lowlevel message dequote
  QString dequotedMessage = dequote(message);
  
  // extract tagged / extended data
  while(dequotedMessage.contains(XDELIM)) {
    if(dequotedMessage.indexOf(XDELIM) > 0) 
      messages << dequotedMessage.section(XDELIM,0,0);
    ctcp = XdelimDequote(dequotedMessage.section(XDELIM,1,1));
    dequotedMessage = dequotedMessage.section(XDELIM,2,2);
    
    //dispatch the ctcp command
    QString ctcpcmd = ctcp.section(' ', 0, 0);
    QString ctcpparam = ctcp.section(' ', 1);

    handle(ctcpcmd, Q_ARG(CtcpType, ctcptype), Q_ARG(QString, prefix), Q_ARG(QString, target), Q_ARG(QString, ctcpparam));
  }
  if(!dequotedMessage.isEmpty()) {
    messages << dequotedMessage;
  }
  return messages;
}

QString CtcpHandler::pack(QString ctcpTag, QString message) {
  return XDELIM + ctcpTag + ' ' + message + XDELIM;
}

void CtcpHandler::query(QString bufname, QString ctcpTag, QString message) {
  QStringList params;
  params << bufname << pack(ctcpTag, message);
  emit putCmd("PRIVMSG", params); 
}

void CtcpHandler::reply(QString bufname, QString ctcpTag, QString message) {
  QStringList params;
  params << bufname << pack(ctcpTag, message);
  emit putCmd("NOTICE", params);
}

//******************************/
// CTCP HANDLER
//******************************/
void CtcpHandler::handleAction(CtcpType ctcptype, QString prefix, QString target, QString param) {
  Q_UNUSED(ctcptype)
  emit displayMsg(Message::Action, target, param, prefix);
}

void CtcpHandler::handlePing(CtcpType ctcptype, QString prefix, QString target, QString param) {
  Q_UNUSED(target)
  if(ctcptype == CtcpQuery) {
    reply(nickFromMask(prefix), "PING", param);
    emit displayMsg(Message::Server, "", tr("Received CTCP PING request from %1").arg(prefix));
  } else {
    // display ping answer
  }
}

void CtcpHandler::handleVersion(CtcpType ctcptype, QString prefix, QString target, QString param) {
  Q_UNUSED(target)
  if(ctcptype == CtcpQuery) {
    // FIXME use real Info about quassel :)
    reply(nickFromMask(prefix), "VERSION", QString("Quassel IRC (Pre-Release) - http://www.quassel-irc.org"));
    emit displayMsg(Message::Server, "", tr("Received CTCP VERSION request by %1").arg(prefix));
  } else {
    // TODO display Version answer
  }
}

void CtcpHandler::defaultHandler(QString cmd, CtcpType ctcptype, QString prefix, QString target, QString param) {
  Q_UNUSED(ctcptype);
  Q_UNUSED(target);
  Q_UNUSED(param);
  emit displayMsg(Message::Error, "", tr("Received unknown CTCP %1 by %2").arg(cmd).arg(prefix));
}



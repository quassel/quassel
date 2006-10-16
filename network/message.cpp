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

#include "message.h"
#include <QtDebug>

extern BuiltinCmd builtins[];

recvHandlerType Message::defaultRecvHandler;
sendHandlerType Message::defaultSendHandler;
QHash<QString, CmdType> Message::cmdTypes;

Message::Message(Server *srv, Buffer *buf, QString _cmd, QString _prefix, QStringList _params)
  : server(srv), buffer(buf), cmd(_cmd), prefix(_prefix), params(_params) {

  // Check if it's a registered cmd (or a numeric reply with a custom entry)
  if(cmdTypes.contains(cmd)) {
    CmdType c = cmdTypes[cmd];
    recvHandler = ( c.recvHandler ? c.recvHandler : defaultRecvHandler);
    sendHandler = ( c.sendHandler ? c.sendHandler : defaultSendHandler);
    type = - c.type;
  } else {
    int t = cmd.toInt();
    if(t) {
      type = t;
      recvHandler = defaultRecvHandler;
      sendHandler = defaultSendHandler;
    } else {
      // Unknown cmd!
      qWarning() << "Unknown command: " << cmd;
      type = 0;
    }
  }
}

void Message::init(recvHandlerType _r, sendHandlerType _s) {
  defaultRecvHandler = _r;
  defaultSendHandler = _s;

  // Register builtin commands
  for(int i = 0; ; i++) {
    if(builtins[i].cmd.isEmpty()) break;
    CmdType c;
    c.cmd = builtins[i].cmd.toUpper();
    c.cmdDescr = builtins[i].cmdDescr;
    c.args = builtins[i].args;
    c.argsDescr = builtins[i].argsDescr;
    c.recvHandler = ( builtins[i].recvHandler ? builtins[i].recvHandler : defaultRecvHandler);
    c.sendHandler = ( builtins[i].sendHandler ? builtins[i].sendHandler : defaultSendHandler);
    cmdTypes.insert(c.cmd, c);
  }

}

recvHandlerType Message::getRecvHandler() {
  CmdType c = cmdTypes[cmd];
  if(c.recvHandler) return c.recvHandler;
  qDebug() << "No recvHandler defined for " << cmd << "!";
  return 0;
}

sendHandlerType Message::getSendHandler() {
  CmdType c = cmdTypes[cmd];
  if(c.sendHandler) return c.sendHandler;
  qDebug() << "No sendHandler defined for " << cmd << "!";
  return 0;
}

/** This parses a raw string as sent by the server and constructs a message object which is then returned.
 */
Message * Message::createFromServerString(Server *srv, QString msg) {
  if(msg.isEmpty()) {
    qWarning() << "Received empty message from server!";
    return 0;
  }
  QString prefix;
  QString cmd;
  QStringList params;
  if(msg[0] == ':') {
    msg.remove(0,1);
    prefix = msg.section(' ', 0, 0);
    msg = msg.section(' ', 1);
  }
  cmd = msg.section(' ', 0, 0).toUpper();
  msg = msg.section(' ', 1);
  QString left = msg.section(':', 0, 0);
  QString trailing = msg.section(':', 1);
  if(!left.isEmpty()) {
    params << left.split(' ', QString::SkipEmptyParts);
  }
  if(!trailing.isEmpty()) {
    params << trailing;
  }
  return new Message(srv, 0, cmd, prefix, params);
  //qDebug() << "prefix: " << prefix << " cmd: " << cmd << " params: " << params;
}

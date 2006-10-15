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

#include "messages.h"
#include <QtDebug>

extern BuiltinCmd builtins[];

QHash<QString, CmdType> Message::cmdTypes;

Message::Message(QString _cmd, QStringList args) {
  cmd = _cmd; qDebug() << "cmd: " << cmd;
  params = args;
}

void Message::init() {
  // Register builtin commands
  for(int i = 0; ; i++) {
    if(!builtins[i].handler) break;
    CmdType c;
    c.cmd = builtins[i].cmd.toLower();
    c.cmdDescr = builtins[i].cmdDescr;
    c.args = builtins[i].args;
    c.argsDescr = builtins[i].argsDescr;
    c.handler = builtins[i].handler;
    cmdTypes.insert(c.cmd, c);
  }

}

cmdhandler Message::getCmdHandler() {
  CmdType c = cmdTypes[cmd];
  if(c.handler) return c.handler;
  qDebug() << "No handler defined for " << cmd << "!";
  return 0;
}

/*
void Message::parseParams(QString str) {
  QString left = str.section(':', 0, 0);
  QString trailing = str.section(':', 1);
  if(left.size()) {
    params << left.split(' ', QString::SkipEmptyParts);
  }
  if(trailing.size()) {
    params << trailing;
  }
  qDebug() << params;

}
*/
void Message::test1(QStringList foo) { qDebug() << "Test 1! " << cmd; };

void Message::test2(QStringList bar) { qDebug() << "Test 2!"; };

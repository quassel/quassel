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

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <QHash>
#include <QString>
#include <QStringList>

class Message;

typedef void (Message::* cmdhandler)(QStringList);

/**
 * This contains information that depends (solely) on the message type, such as the handler function and help texts.
 * Most of these are defined at compile time, but more may be added at runtime.
 */
struct CmdType {
  QString cmd;
  QString cmdDescr;
  QString args;
  QString argsDescr;
  cmdhandler handler;

};

/**
 *
*/
class Message {
  public:
    uint type;
    QString prefix;
    QString cmd;
    QStringList params;

    Message(QString cmd, QStringList args = QStringList());

    virtual ~Message() {};

    static void init();
    //static registerCmd();
    //static unregisterCmd();

    cmdhandler getCmdHandler();

    void test1(QStringList);
    void test2(QStringList);

  protected:
    static QHash<QString, CmdType> cmdTypes;
};


/** This is only used to have a nice way for defining builtin commands.
 *  We create an array of these in builtin_cmds.cpp and read this to fill our
 *  command hash.
 */
struct BuiltinCmd {
  QString cmd;
  QString cmdDescr;
  QString args;
  QString argsDescr;
  cmdhandler handler;
};

#endif

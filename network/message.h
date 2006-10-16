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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <QHash>
#include <QString>
#include <QStringList>

class Server;
class Buffer;
class Message;

typedef void (*sendHandlerType)(Message *);        // handler for commands sent by the user
typedef void (*recvHandlerType)(Message *);              // handler for incoming messages

/**
 * This contains information that depends (solely) on the message type, such as the handler function and help texts.
 * Most of these are defined at compile time, but more may be added at runtime.
 */
struct CmdType {
  int type;
  QString cmd;
  QString cmdDescr;
  QString args;
  QString argsDescr;
  recvHandlerType recvHandler;
  sendHandlerType sendHandler;

};

/**
 *
*/
class Message {
  public:
    Message(Server *server, Buffer *buffer, QString cmd, QString prefix, QStringList args = QStringList());

    virtual ~Message() {};

    static void init(recvHandlerType defaultRevcHandler, sendHandlerType defaultSendHandler);
    static Message * createFromServerString(Server *server, QString srvMsg);
    //static Message * createFromUserString(Server *server, Buffer *buffer, QString usrMsg);
    //static registerCmd();
    //static unregisterCmd();

    inline Server * getServer() { return server; }
    inline Buffer * getBuffer() { return buffer; }
    inline int getType() { return type; }
    inline QString getPrefix() { return prefix; }
    inline QString getCmd() { return cmd; }
    inline QStringList getParams() { return params; }
    inline recvHandlerType getRecvHandler();
    inline sendHandlerType getSendHandler();

  protected:
    Server *server;
    Buffer *buffer;
    int type;
    QString prefix;
    QString cmd;
    QStringList params;
    recvHandlerType recvHandler;
    sendHandlerType sendHandler;
    static recvHandlerType defaultRecvHandler;
    static sendHandlerType defaultSendHandler;

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
  recvHandlerType recvHandler;
  sendHandlerType sendHandler;
};

#endif

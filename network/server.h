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

#ifndef _SERVER_H_
#define _SERVER_H_

#include <QtCore>
#include <QTimer>
#include <QtNetwork>

#include "global.h"
#include "buffer.h"
#include "message.h"

#define DEFAULT_PORT 6667


/*!
 * This is a server object, managing a single connection to an IRC server, handling the associated channels and so on.
 * We have this running in its own thread mainly to not block other server objects or the core if something goes wrong,
 * e.g. if some scripts starts running wild...
 */

class Server : public QThread {
  Q_OBJECT

  public:
    Server(QString network);
    ~Server();

    // serverState state();
    bool isConnected() { return socket.state() == QAbstractSocket::ConnectedState; }
    QString getNetwork() { return network; }

  public slots:
    // void setServerOptions();
    void connectToIrc(QString net);
    void disconnectFromIrc(QString net);
    void userInput(QString net, QString buffer, QString msg);

    void putRawLine(QString input);
    void putCmd(QString cmd, QStringList params, QString prefix = 0);

    //void exitThread();

  signals:
    void recvRawServerMsg(QString);
    void sendStatusMsg(QString);
    void sendMessage(QString buffer, Message msg);
    void disconnected();

    void setTopic(QString network, QString buffer, QString topic);
    void setNicks(QString network, QString buffer, QStringList nicks);


  private slots:
    void run();
    void socketHasData();
    void socketError(QAbstractSocket::SocketError);
    void socketConnected();
    void socketDisconnected();
    void socketStateChanged(QAbstractSocket::SocketState);

    /* Message Handlers */

    /* handleUser(QString, Buffer *) */
    void handleUserJoin(QString, Buffer *);
    void handleUserQuote(QString, Buffer *);
    void handleUserSay(QString, Buffer *);


    /* handleServer(QString, QStringList); */
    void handleServerJoin(QString, QStringList);
    void handleServerNotice(QString, QStringList);
    void handleServerPing(QString, QStringList);
    void handleServerPrivmsg(QString, QStringList);

    void handleServer001(QString, QStringList);   // RPL_WELCOME
    void handleServer331(QString, QStringList);   // RPL_NOTOPIC
    void handleServer332(QString, QStringList);   // RPL_TOPIC

    void defaultServerHandler(QString cmd, QString prefix, QStringList params);
    void defaultUserHandler(QString cmd, QString msg, Buffer *buf);

  private:
    QString network;
    QTcpSocket socket;
    QHash<QString, Buffer*> buffers;

    QString currentNick;
    QString currentServer;

    void handleServerMsg(QString rawMsg);
    void handleUserMsg(QString buffer, QString usrMsg);

    class ParseError : public Exception {
      public:
        ParseError(QString cmd, QString prefix, QStringList params);
    };

    class UnknownCmdError : public Exception {
      public:
        UnknownCmdError(QString cmd, QString prefix, QStringList params);
    };
};

#endif

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

#include "quassel.h"

#define DEFAULT_PORT 6667

/*! \file */

/*! \class Server
 * This is a server object, managing a single connection to an IRC server, handling the associated channels and so on.
 * We have this run in its own thread mainly to not block other server objects or the core if something goes wrong,
 * e.g. if some scripts starts running wild...
 */

class Server : public QThread {
  Q_OBJECT

  public:
    Server();
    ~Server();
    static void init();

    void run();

    // serverState state();
    bool isConnected() { return socket.state() == QAbstractSocket::ConnectedState; }

  public slots:
    // void setServerOptions();
    void connectToIrc(const QString &host, quint16 port = DEFAULT_PORT);
    void disconnectFromIrc();

    void putRawLine(QString input);
    void putCmd(QString cmd, QStringList params, QString prefix = 0);

  signals:
    //void outputLine(const QString & /*, Buffer *target = 0 */);

    void recvRawServerMsg(QString);
    void recvLine(QString); // temp, should send a message to the GUI

  private slots:
    void socketHasData();
    void socketError(QAbstractSocket::SocketError);
    void socketConnected();
    void socketDisconnected();
    void socketStateChanged(QAbstractSocket::SocketState);

    /* Message Handlers */
    /* handleXxxxFromServer(QString prefix, QStringList params); */
    void handleNoticeFromServer(QString, QStringList);
    void handlePingFromServer(QString, QStringList);

    void defaultHandlerForServer(QString cmd, QString prefix, QStringList params);

  private:
    QTcpSocket socket;
    QTextStream stream;

    void handleServerMsg(QString rawMsg);
    void handleUserMsg(QString usrMsg);
    //static inline void dispatchServerMsg(Message *msg) { msg->getServer()->handleServerMsg(msg); }
    //static inline void dispatchUserMsg(Message *msg)   { msg->getServer()->handleUserMsg(msg); }

    class ParseError : public Exception {
      public:
        ParseError(QString cmd, QString prefix, QStringList params);
    };

    class UnknownCmdError : public Exception {
      public:
        UnknownCmdError(QString cmd, QString prefix, QStringList params);
    };
};

class Buffer {};


#endif

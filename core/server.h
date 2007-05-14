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
    QStringList providesUserHandlers();

  public slots:
    // void setServerOptions();
    void sendState();
    void connectToIrc(QString net);
    void disconnectFromIrc(QString net);
    void userInput(QString net, QString buffer, QString msg);

    void putRawLine(QString input);
    void putCmd(QString cmd, QStringList params, QString prefix = 0);

    //void exitThread();

  signals:
    void serverState(QString net, VarMap data);
    void recvRawServerMsg(QString);
    void displayStatusMsg(QString);
    void displayMsg(Message msg);
    void connected(QString network);
    void disconnected(QString network);

    void nickAdded(QString network, QString nick, VarMap props);
    void nickRenamed(QString network, QString oldnick, QString newnick);
    void nickRemoved(QString network, QString nick);
    void nickUpdated(QString network, QString nick, VarMap props);
    void modeSet(QString network, QString target, QString mode);
    void topicSet(QString network, QString buffer, QString topic);
    void setNicks(QString network, QString buffer, QStringList nicks);
    void ownNickSet(QString network, QString newNick);
    void queryRequested(QString network, QString nick);


  private slots:
    void run();
    void socketHasData();
    void socketError(QAbstractSocket::SocketError);
    void socketConnected();
    void socketDisconnected();
    void socketStateChanged(QAbstractSocket::SocketState);

    /* Message Handlers */

    /* void handleUser(QString, QString); */
    void handleUserAway(QString, QString);
    void handleUserDeop(QString, QString);
    void handleUserDevoice(QString, QString);
    void handleUserInvite(QString, QString);
    void handleUserJoin(QString, QString);
    void handleUserKick(QString, QString);
    void handleUserList(QString, QString);
    void handleUserMode(QString, QString);
    void handleUserMsg(QString, QString);
    void handleUserNick(QString, QString);
    void handleUserOp(QString, QString);
    void handleUserPart(QString, QString);
    void handleUserQuery(QString, QString);
    void handleUserQuit(QString, QString);
    void handleUserQuote(QString, QString);
    void handleUserSay(QString, QString);
    void handleUserTopic(QString, QString);
    void handleUserVoice(QString, QString);

    /* void handleServer(QString, QStringList); */
    void handleServerJoin(QString, QStringList);
    void handleServerKick(QString, QStringList);
    void handleServerMode(QString, QStringList);
    void handleServerNick(QString, QStringList);
    void handleServerNotice(QString, QStringList);
    void handleServerPart(QString, QStringList);
    void handleServerPing(QString, QStringList);
    void handleServerPrivmsg(QString, QStringList);
    void handleServerQuit(QString, QStringList);
    void handleServerTopic(QString, QStringList);

    void handleServer001(QString, QStringList);   // RPL_WELCOME
    void handleServer005(QString, QStringList);   // RPL_ISUPPORT
    void handleServer331(QString, QStringList);   // RPL_NOTOPIC
    void handleServer332(QString, QStringList);   // RPL_TOPIC
    void handleServer333(QString, QStringList);   // Topic set by...
    void handleServer353(QString, QStringList);   // RPL_NAMREPLY
    void handleServer433(QString, QStringList);  // RPL_NICKNAMEINUSER

    void defaultServerHandler(QString cmd, QString prefix, QStringList params);
    void defaultUserHandler(QString buf, QString cmd, QString msg);

  private:
    QString network;
    QTcpSocket socket;
    //QHash<QString, Buffer*> buffers;

    QString ownNick;
    QString currentServer;
    VarMap networkSettings;
    VarMap identity;
    QHash<QString, VarMap> nicks;  // stores all known nicks for the server
    QHash<QString, QString> topics; // stores topics for each buffer
    VarMap serverSupports;  // stores results from RPL_ISUPPORT

    void handleServerMsg(QString rawMsg);
    void handleUserInput(QString buffer, QString usrMsg);

    QString updateNickFromMask(QString mask);

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

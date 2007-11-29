/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

#ifndef _SERVER_H_
#define _SERVER_H_

#include <QAbstractSocket>
#include <QString>
#include <QStringList>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

#include "message.h"
#include "signalproxy.h"

class NetworkInfo;

class IrcServerHandler;
class UserInputHandler;
class CtcpHandler;
class CoreSession;

/*!
 * This is a server object, managing a single connection to an IRC server, handling the associated channels and so on.
 * We have this running in its own thread mainly to not block other server objects or the core if something goes wrong,
 * e.g. if some scripts starts running wild...
 */

class Server : public QThread {
  Q_OBJECT

public:
  Server(UserId uid, uint networkId, QString network);
  ~Server();

  UserId userId() const { return _userId; } 

  // serverState state();
  bool isConnected() const { return socket.state() == QAbstractSocket::ConnectedState; }

  uint networkId() const;
  QString networkName();  // hasbeen getNetwork()

  NetworkInfo *networkInfo() { return _networkInfo; }
  IrcServerHandler *ircServerHandler() {return _ircServerHandler; }
  UserInputHandler *userInputHandler() {return _userInputHandler; }
  CtcpHandler *ctcpHandler() {return _ctcpHandler; }
  
public slots:
  // void setServerOptions();
  void connectToIrc(QString net);
  void disconnectFromIrc(QString net);
  void userInput(uint netid, QString buffer, QString msg);

  void putRawLine(QString input);
  void putCmd(QString cmd, QStringList params, QString prefix = 0);


private slots:
  void threadFinished();

signals:
  void serverState(QString net, QVariantMap data);
  void recvRawServerMsg(QString);
  void displayStatusMsg(QString);
  //void displayMsg(Message msg);
  void displayMsg(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void connected(uint networkId);
  void disconnected(uint networkId);

  void synchronizeClients();
  
  void queryRequested(QString network, QString nick);


private slots:
  void run();
  void socketHasData();
  void socketError(QAbstractSocket::SocketError);
  void socketConnected();
  void socketStateChanged(QAbstractSocket::SocketState);

private:
  UserId _userId;
  uint _networkId;

  QTcpSocket socket;

  IrcServerHandler *_ircServerHandler;
  UserInputHandler *_userInputHandler;
  CtcpHandler *_ctcpHandler;

  NetworkInfo *_networkInfo;

  QVariantMap networkSettings;
  QVariantMap identity;

  CoreSession *coreSession() const;
  
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

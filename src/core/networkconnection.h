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

#ifndef _NETWORKCONNECTION_H_
#define _NETWORKCONNECTION_H_

#include <QAbstractSocket>
#include <QString>
#include <QStringList>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

#include "message.h"
#include "signalproxy.h"

class Network;

class IrcServerHandler;
class UserInputHandler;
class CtcpHandler;
class CoreSession;

/*!
 * This is a server object, managing a single connection to an IRC server, handling the associated channels and so on.
 * We have this running in its own thread mainly to not block other server objects or the core if something goes wrong,
 * e.g. if some scripts starts running wild...
 */

class NetworkConnection : public QThread {
  Q_OBJECT

public:
  NetworkConnection(UserId uid, NetworkId networkId, QString network, const QVariant &previousState = QVariant());
  ~NetworkConnection();

  UserId userId() const { return _userId; } 

  // networkState state();
  bool isConnected() const { return socket.state() == QAbstractSocket::ConnectedState; }

  NetworkId networkId() const;
  QString networkName() const;  // hasbeen getNetwork()

  Network *network() const { return _network; }
  IrcServerHandler *ircServerHandler() const { return _ircServerHandler; }
  UserInputHandler *userInputHandler() const { return _userInputHandler; }
  CtcpHandler *ctcpHandler() const { return _ctcpHandler; }

  QVariant state(); ///< Return data necessary to restore the server's state upon core restart

  //! Decode a string using the server (network) decoding.
  QString serverDecode(const QByteArray &string) const;

  //! Decode a string using a buffer-specific encoding if one is set (and use the server encoding else).
  QString bufferDecode(const QString &bufferName, const QByteArray &string) const;

  //! Decode a string using a IrcUser specific encoding, if one exists (using the server encoding else).
  QString userDecode(const QString &userNick, const QByteArray &string) const;

  //! Encode a string using the server (network) encoding.
  QByteArray serverEncode(const QString &string) const;

  //! Encode a string using the buffer-specific encoding, if set, and use the server encoding else.
  QByteArray bufferEncode(const QString &bufferName, const QString &string) const;

  //! Encode a string using the user-specific encoding, if set, and use the server encoding else.
  QByteArray userEncode(const QString &userNick, const QString &string) const;

public slots:
  // void setServerOptions();
  void connectToIrc(QString net);
  void disconnectFromIrc(QString net);
  void userInput(uint netid, QString buffer, QString msg);

  void putRawLine(QString input);
  void putCmd(QString cmd, QStringList params, QString prefix = 0);


private slots:
  void threadFinished();
  void sendPerform();

signals:
  void networkState(QString net, QVariantMap data);
  void recvRawServerMsg(QString);
  void displayStatusMsg(QString);
  //void displayMsg(Message msg);
  void displayMsg(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void connected(uint networkId);
  void disconnected(uint networkId);

  void connectionInitialized(); ///< Emitted after receipt of 001 to indicate that we can now send data to the IRC server

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
  NetworkId _networkId;

  QTcpSocket socket;

  IrcServerHandler *_ircServerHandler;
  UserInputHandler *_userInputHandler;
  CtcpHandler *_ctcpHandler;

  Network *_network;

  QVariantMap networkSettings;
  QVariantMap identity;

  QVariant _previousState;

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

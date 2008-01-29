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
#include <QTimer>

#include "message.h"
#include "signalproxy.h"

class CoreSession;
class Network;

class IrcServerHandler;
class UserInputHandler;
class CtcpHandler;

class NetworkConnection : public QObject {
  Q_OBJECT

public:
  NetworkConnection(Network *network, CoreSession *session, const QVariant &previousState = QVariant());
  ~NetworkConnection();

  NetworkId networkId() const;
  QString networkName() const;
  Network *network() const;
  CoreSession *coreSession() const;

  bool isConnected() const;

  IrcServerHandler *ircServerHandler() const;
  UserInputHandler *userInputHandler() const;
  CtcpHandler *ctcpHandler() const;

  //! Return data necessary to restore the connection state upon core restart
  QVariant state() const;

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
  void connectToIrc();
  void disconnectFromIrc();
  void userInput(QString buffer, QString msg);

  void putRawLine(QString input);
  void putCmd(QString cmd, QStringList params, QString prefix = 0);


private slots:
  void sendPerform();

signals:
  // #void networkState(QString net, QVariantMap data);
  void recvRawServerMsg(QString);
  void displayStatusMsg(QString);
  //void displayMsg(Message msg);
  void displayMsg(Message::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void connected(NetworkId networkId);
  void disconnected(NetworkId networkId);

  void connectionInitialized(); ///< Emitted after receipt of 001 to indicate that we can now send data to the IRC server

  //void queryRequested(QString network, QString nick);


private slots:
  void socketHasData();
  void socketError(QAbstractSocket::SocketError);
  void socketConnected();
  void socketDisconnected();
  void socketStateChanged(QAbstractSocket::SocketState);

private:
  QTcpSocket socket;

  Network *_network;
  CoreSession *_coreSession;

  IrcServerHandler *_ircServerHandler;
  UserInputHandler *_userInputHandler;
  CtcpHandler *_ctcpHandler;

  QVariantMap networkSettings;
  QVariantMap identity;

  QVariant _previousState;

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

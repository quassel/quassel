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

#include "identity.h"
#include "message.h"
#include "network.h"
#include "signalproxy.h"

class CoreSession;
class Network;

class IrcServerHandler;
class UserInputHandler;
class CtcpHandler;

class NetworkConnection : public QObject {
  Q_OBJECT

public:
  NetworkConnection(Network *network, CoreSession *session);
  ~NetworkConnection();

  NetworkId networkId() const;
  QString networkName() const;
  Network *network() const;
  Identity *identity() const;
  CoreSession *coreSession() const;

  bool isConnected() const;
  Network::ConnectionState connectionState() const;

  IrcServerHandler *ircServerHandler() const;
  UserInputHandler *userInputHandler() const;
  CtcpHandler *ctcpHandler() const;

  //! Decode a string using the server (network) decoding.
  QString serverDecode(const QByteArray &string) const;

  //! Decode a string using a channel-specific encoding if one is set (and use the standard encoding else).
  QString channelDecode(const QString &channelName, const QByteArray &string) const;

  //! Decode a string using an IrcUser-specific encoding, if one exists (using the standaed encoding else).
  QString userDecode(const QString &userNick, const QByteArray &string) const;

  //! Encode a string using the server (network) encoding.
  QByteArray serverEncode(const QString &string) const;

  //! Encode a string using the channel-specific encoding, if set, and use the standard encoding else.
  QByteArray channelEncode(const QString &channelName, const QString &string) const;

  //! Encode a string using the user-specific encoding, if set, and use the standard encoding else.
  QByteArray userEncode(const QString &userNick, const QString &string) const;

  inline QString channelKey(const QString &channel) const { return _channelKeys.value(channel, QString()); }

public slots:
  // void setServerOptions();
  void connectToIrc(bool reconnecting = false);
  void disconnectFromIrc();
  void userInput(BufferInfo bufferInfo, QString msg);

  void putRawLine(QByteArray input);
  void putCmd(const QString &cmd, const QVariantList &params, const QByteArray &prefix = QByteArray());

  void addChannelKey(const QString &channel, const QString &key);
  void removeChannelKey(const QString &channel);

private slots:
  void sendPerform();
  void autoReconnectSettingsChanged();
  void doAutoReconnect();
  void nickChanged(const QString &newNick, const QString &oldNick); // this signal is inteded to rename query buffers in the storage backend

signals:
  // #void networkState(QString net, QVariantMap data);
  void recvRawServerMsg(QString);
  void displayStatusMsg(QString);
  //void displayMsg(Message msg);
  void displayMsg(Message::Type, BufferInfo::Type, QString target, QString text, QString sender = "", quint8 flags = Message::None);
  void connected(NetworkId networkId);   ///< Emitted after receipt of 001 to indicate that we can now send data to the IRC server
  void disconnected(NetworkId networkId);
  void connectionStateChanged(Network::ConnectionState);
  void connectionInitialized(); ///< Emitted after receipt of 001 to indicate that we can now send data to the IRC server
  void connectionError(const QString &errorMsg);

  void quitRequested(NetworkId networkId);

  //void queryRequested(QString network, QString nick);
  void nickChanged(const NetworkId &networkId, const QString &newNick, const QString &oldNick); // this signal is inteded to rename query buffers in the storage backend

private slots:
  void socketHasData();
  void socketError(QAbstractSocket::SocketError);
  void socketConnected();
  void socketDisconnected();
  void socketStateChanged(QAbstractSocket::SocketState);
  void setConnectionState(Network::ConnectionState);
  void networkInitialized(const QString &currentServer);

private:
  QTcpSocket socket;
  Network::ConnectionState _connectionState;

  Network *_network;
  CoreSession *_coreSession;

  IrcServerHandler *_ircServerHandler;
  UserInputHandler *_userInputHandler;
  CtcpHandler *_ctcpHandler;

  QHash<QString, QString> _channelKeys;
  QTimer _autoReconnectTimer;
  int _autoReconnectCount;

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

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
#include <QTimer>

#ifdef HAVE_SSL
# include <QSslSocket>
# include <QSslError>
#else
# include <QTcpSocket>
#endif

#include "coresession.h"
#include "identity.h"
#include "message.h"
#include "network.h"
#include "signalproxy.h"

class IrcServerHandler;
class UserInputHandler;
class CtcpHandler;

class NetworkConnection : public QObject {
  Q_OBJECT

public:
  NetworkConnection(Network *network, CoreSession *session);
  ~NetworkConnection();

  inline NetworkId networkId() const { return network()->networkId(); }
  inline QString networkName() const { return network()->networkName(); }
  inline Network *network() const { return _network; }
  inline Identity *identity() const { return coreSession()->identity(network()->identity()); }
  inline CoreSession *coreSession() const { return _coreSession; }

  inline bool isConnected() const { return connectionState() == Network::Initialized; }
  inline Network::ConnectionState connectionState() const { return _connectionState; }

  inline IrcServerHandler *ircServerHandler() const { return _ircServerHandler; }
  inline UserInputHandler *userInputHandler() const { return _userInputHandler; }
  inline CtcpHandler *ctcpHandler() const { return _ctcpHandler; }

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

  inline QString channelKey(const QString &channel) const { return _channelKeys.value(channel.toLower(), QString()); }
  inline QStringList persistentChannels() const { return _channelKeys.keys(); }

  inline bool isAutoWhoInProgress(const QString &channel) const { return _autoWhoInProgress.value(channel.toLower(), 0); }

public slots:
  // void setServerOptions();
  void connectToIrc(bool reconnecting = false);
  void disconnectFromIrc(bool requested = true);
  void userInput(BufferInfo bufferInfo, QString msg);

  void putRawLine(QByteArray input);
  int lastParamOverrun(const QString &cmd, const QList<QByteArray> &params);
  void putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray());

  void setChannelJoined(const QString &channel);
  void setChannelParted(const QString &channel);
  void addChannelKey(const QString &channel, const QString &key);
  void removeChannelKey(const QString &channel);

  bool setAutoWhoDone(const QString &channel);

signals:
  // #void networkState(QString net, QVariantMap data);
  void recvRawServerMsg(QString);
  void displayStatusMsg(QString);
  //void displayMsg(Message msg);
  void displayMsg(Message::Type, BufferInfo::Type, QString target, QString text, QString sender = "", Message::Flags flags = Message::None);
  void connected(NetworkId networkId);   ///< Emitted after receipt of 001 to indicate that we can now send data to the IRC server
  void disconnected(NetworkId networkId);
  void connectionStateChanged(Network::ConnectionState);
  void connectionInitialized(); ///< Emitted after receipt of 001 to indicate that we can now send data to the IRC server
  void connectionError(const QString &errorMsg);

  void quitRequested(NetworkId networkId);

  //void queryRequested(QString network, QString nick);
  void nickChanged(const NetworkId &networkId, const QString &newNick, const QString &oldNick); // this signal is inteded to rename query buffers in the storage backend
  void channelJoined(NetworkId, const QString &channel, const QString &key = QString());
  void channelParted(NetworkId, const QString &channel);

  void sslErrors(const QVariant &errorData);

private slots:
  void socketHasData();
  void socketError(QAbstractSocket::SocketError);
  void socketConnected();
  void socketInitialized();
  void socketCloseTimeout();
  void socketDisconnected();
  void socketStateChanged(QAbstractSocket::SocketState);
  void setConnectionState(Network::ConnectionState);
  void networkInitialized(const QString &currentServer);

  void sendPerform();
  void autoReconnectSettingsChanged();
  void doAutoReconnect();
  void sendPing();
  void sendAutoWho();
  void startAutoWhoCycle();
  void nickChanged(const QString &newNick, const QString &oldNick); // this signal is inteded to rename query buffers in the storage backend

#ifdef HAVE_SSL
  void socketEncrypted();
  void sslErrors(const QList<QSslError> &errors);
#endif

  void fillBucketAndProcessQueue();

private:
#ifdef HAVE_SSL
  QSslSocket socket;
#else
  QTcpSocket socket;
#endif

  Network::ConnectionState _connectionState;

  Network *_network;
  CoreSession *_coreSession;
  BufferInfo _statusBufferInfo;

  IrcServerHandler *_ircServerHandler;
  UserInputHandler *_userInputHandler;
  CtcpHandler *_ctcpHandler;

  QHash<QString, QString> _channelKeys;  // stores persistent channels and their passwords, if any

  QTimer _autoReconnectTimer;
  
  int _autoReconnectCount;

  QTimer _socketCloseTimer;

  /* this flag triggers quitRequested() once the socket is closed
   * it is needed to determine whether or not the connection needs to be
   *in the automatic session restore. */
  bool _quitRequested;

  bool _previousConnectionAttemptFailed;
  int _lastUsedServerlistIndex;

  QTimer _pingTimer;
  
  bool _autoWhoEnabled;
  QStringList _autoWhoQueue;
  QHash<QString, int> _autoWhoInProgress;
  int _autoWhoInterval;
  int _autoWhoNickLimit;
  int _autoWhoDelay;
  QTimer _autoWhoTimer, _autoWhoCycleTimer;

  QTimer _tokenBucketTimer;
  int _messagesPerSecond;   // token refill speed
  int _burstSize;   // size of the token bucket
  int _tokenBucket; // the virtual bucket that holds the tokens
  QList<QByteArray> _msgQueue;

  void writeToSocket(QByteArray s);

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

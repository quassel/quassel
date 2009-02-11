/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef CORENETWORK_H
#define CORENETWORK_H

#include "network.h"
#include "coreircchannel.h"

#include <QTimer>

#ifdef HAVE_SSL
# include <QSslSocket>
# include <QSslError>
#else
# include <QTcpSocket>
#endif

#include "coresession.h"

class CoreIdentity;
class IrcServerHandler;
class UserInputHandler;
class CtcpHandler;

class CoreNetwork : public Network {
  Q_OBJECT

public:
  CoreNetwork(const NetworkId &networkid, CoreSession *session);
  ~CoreNetwork();
  inline virtual const QMetaObject *syncMetaObject() const { return &Network::staticMetaObject; }

  inline CoreIdentity *identityPtr() const { return coreSession()->identity(identity()); }
  inline CoreSession *coreSession() const { return _coreSession; }

  inline IrcServerHandler *ircServerHandler() const { return _ircServerHandler; }
  inline UserInputHandler *userInputHandler() const { return _userInputHandler; }
  inline CtcpHandler *ctcpHandler() const { return _ctcpHandler; }

  //! Decode a string using the server (network) decoding.
  inline QString serverDecode(const QByteArray &string) const { return decodeServerString(string); }

  //! Decode a string using a channel-specific encoding if one is set (and use the standard encoding else).
  QString channelDecode(const QString &channelName, const QByteArray &string) const;

  //! Decode a string using an IrcUser-specific encoding, if one exists (using the standaed encoding else).
  QString userDecode(const QString &userNick, const QByteArray &string) const;

  //! Encode a string using the server (network) encoding.
  inline QByteArray serverEncode(const QString &string) const { return encodeServerString(string); }

  //! Encode a string using the channel-specific encoding, if set, and use the standard encoding else.
  QByteArray channelEncode(const QString &channelName, const QString &string) const;

  //! Encode a string using the user-specific encoding, if set, and use the standard encoding else.
  QByteArray userEncode(const QString &userNick, const QString &string) const;

  inline QString channelKey(const QString &channel) const { return _channelKeys.value(channel.toLower(), QString()); }

  inline bool isAutoWhoInProgress(const QString &channel) const { return _autoWhoInProgress.value(channel.toLower(), 0); }

  inline UserId userId() const { return _coreSession->user(); }

public slots:
  virtual void setMyNick(const QString &mynick);

  virtual void requestConnect() const;
  virtual void requestDisconnect() const;
  virtual void requestSetNetworkInfo(const NetworkInfo &info);

  virtual void setUseAutoReconnect(bool);
  virtual void setAutoReconnectInterval(quint32);
  virtual void setAutoReconnectRetries(quint16);

  void connectToIrc(bool reconnecting = false);
  void disconnectFromIrc(bool requested = true, const QString &reason = QString(), bool withReconnect = false);

  void userInput(BufferInfo bufferInfo, QString msg);
  void putRawLine(QByteArray input);
  void putCmd(const QString &cmd, const QList<QByteArray> &params, const QByteArray &prefix = QByteArray());

  void setChannelJoined(const QString &channel);
  void setChannelParted(const QString &channel);
  void addChannelKey(const QString &channel, const QString &key);
  void removeChannelKey(const QString &channel);

  bool setAutoWhoDone(const QString &channel);

  Server usedServer() const;

  inline void resetPong() { _gotPong = true; }
  inline bool gotPong() { return _gotPong; }

signals:
  void recvRawServerMsg(QString);
  void displayStatusMsg(QString);
  void displayMsg(Message::Type, BufferInfo::Type, QString target, QString text, QString sender = "", Message::Flags flags = Message::None);
  void disconnected(NetworkId networkId);
  void connectionError(const QString &errorMsg);

  void quitRequested(NetworkId networkId);
  void sslErrors(const QVariant &errorData);

protected:
  inline virtual IrcChannel *ircChannelFactory(const QString &channelname) { return new CoreIrcChannel(channelname, this); }

private slots:
  void socketHasData();
  void socketError(QAbstractSocket::SocketError);
  void socketInitialized();
  inline void socketCloseTimeout() { socket.disconnectFromHost(); }
  void socketDisconnected();
  void socketStateChanged(QAbstractSocket::SocketState);
  void networkInitialized();

  void sendPerform();
  void restoreUserModes();
  void doAutoReconnect();
  void sendPing();
  void sendAutoWho();
  void startAutoWhoCycle();

#ifdef HAVE_SSL
  void sslErrors(const QList<QSslError> &errors);
#endif

  void fillBucketAndProcessQueue();

  void writeToSocket(const QByteArray &data);

private:
  CoreSession *_coreSession;

#ifdef HAVE_SSL
  QSslSocket socket;
#else
  QTcpSocket socket;
#endif

  IrcServerHandler *_ircServerHandler;
  UserInputHandler *_userInputHandler;
  CtcpHandler *_ctcpHandler;

  QHash<QString, QString> _channelKeys;  // stores persistent channels and their passwords, if any

  QTimer _autoReconnectTimer;
  int _autoReconnectCount;

  QTimer _socketCloseTimer;

  /* this flag triggers quitRequested() once the socket is closed
   * it is needed to determine whether or not the connection needs to be
   * in the automatic session restore. */
  bool _quitRequested;
  QString _quitReason;

  bool _previousConnectionAttemptFailed;
  int _lastUsedServerIndex;

  QTimer _pingTimer;
  bool _gotPong;

  bool _autoWhoEnabled;
  QStringList _autoWhoQueue;
  QHash<QString, int> _autoWhoInProgress;
  int _autoWhoInterval;
  int _autoWhoNickLimit;
  int _autoWhoDelay;
  QTimer _autoWhoTimer, _autoWhoCycleTimer;

  QTimer _tokenBucketTimer;
  int _messagesPerSecond;   // token refill speed
  int _burstSize;           // size of the token bucket
  int _tokenBucket;         // the virtual bucket that holds the tokens
  QList<QByteArray> _msgQueue;
};

#endif //CORENETWORK_H

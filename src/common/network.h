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

#ifndef NETWORK_H_
#define NETWORK_H_

#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QVariantMap>
#include <QPointer>
#include <QMutex>

#include "types.h"
#include "syncableobject.h"

#include "signalproxy.h"
#include "ircuser.h"
#include "ircchannel.h"

// defined below!
struct NetworkInfo;

// TODO: ConnectionInfo to propagate and sync the current state of NetworkConnection, encodings etcpp

class Network : public SyncableObject {
  Q_OBJECT
  Q_ENUMS(ConnectionState Network::ConnectionState)

  Q_PROPERTY(QString networkName READ networkName WRITE setNetworkName STORED false)
  Q_PROPERTY(QString currentServer READ currentServer WRITE setCurrentServer STORED false)
  Q_PROPERTY(QString myNick READ myNick WRITE setMyNick STORED false)
  Q_PROPERTY(int latency READ latency WRITE setLatency STORED false)
  Q_PROPERTY(QByteArray codecForServer READ codecForServer WRITE setCodecForServer STORED false)
  Q_PROPERTY(QByteArray codecForEncoding READ codecForEncoding WRITE setCodecForEncoding STORED false)
  Q_PROPERTY(QByteArray codecForDecoding READ codecForDecoding WRITE setCodecForDecoding STORED false)
  Q_PROPERTY(IdentityId identityId READ identity WRITE setIdentity STORED false)
  Q_PROPERTY(bool isConnected READ isConnected WRITE setConnected STORED false)
  //Q_PROPERTY(Network::ConnectionState connectionState READ connectionState WRITE setConnectionState STORED false)
  Q_PROPERTY(int connectionState READ connectionState WRITE setConnectionState STORED false)
  Q_PROPERTY(bool useRandomServer READ useRandomServer WRITE setUseRandomServer STORED false)
  Q_PROPERTY(QStringList perform READ perform WRITE setPerform STORED false)
  Q_PROPERTY(bool useAutoIdentify READ useAutoIdentify WRITE setUseAutoIdentify STORED false)
  Q_PROPERTY(QString autoIdentifyService READ autoIdentifyService WRITE setAutoIdentifyService STORED false)
  Q_PROPERTY(QString autoIdentifyPassword READ autoIdentifyPassword WRITE setAutoIdentifyPassword STORED false)
  Q_PROPERTY(bool useAutoReconnect READ useAutoReconnect WRITE setUseAutoReconnect STORED false)
  Q_PROPERTY(quint32 autoReconnectInterval READ autoReconnectInterval WRITE setAutoReconnectInterval STORED false)
  Q_PROPERTY(quint16 autoReconnectRetries READ autoReconnectRetries WRITE setAutoReconnectRetries STORED false)
  Q_PROPERTY(bool unlimitedReconnectRetries READ unlimitedReconnectRetries WRITE setUnlimitedReconnectRetries STORED false)
  Q_PROPERTY(bool rejoinChannels READ rejoinChannels WRITE setRejoinChannels STORED false)

public:
  enum ConnectionState {
    Disconnected,
    Connecting,
    Initializing,
    Initialized,
    Reconnecting,
    Disconnecting
  };

  // see:
  //  http://www.irc.org/tech_docs/005.html
  //  http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt
  enum ChannelModeType {
    NOT_A_CHANMODE = 0x00,
    A_CHANMODE = 0x01,
    B_CHANMODE = 0x02,
    C_CHANMODE = 0x04,
    D_CHANMODE = 0x08
  };

  
  Network(const NetworkId &networkid, QObject *parent = 0);
  ~Network();

  inline NetworkId networkId() const { return _networkId; }

  inline SignalProxy *proxy() const { return _proxy; }
  inline void setProxy(SignalProxy *proxy) { _proxy = proxy; }

  inline bool isMyNick(const QString &nick) const { return (myNick().toLower() == nick.toLower()); }
  inline bool isMe(IrcUser *ircuser) const { return (ircuser->nick().toLower() == myNick().toLower()); }

  bool isChannelName(const QString &channelname) const;

  inline bool isConnected() const { return _connected; }
  //Network::ConnectionState connectionState() const;
  inline int connectionState() const { return _connectionState; }

  QString prefixToMode(const QString &prefix);
  inline QString prefixToMode(const QCharRef &prefix) { return prefixToMode(QString(prefix)); }
  QString modeToPrefix(const QString &mode);
  inline QString modeToPrefix(const QCharRef &mode) { return modeToPrefix(QString(mode)); }

  ChannelModeType channelModeType(const QString &mode);
  inline ChannelModeType channelModeType(const QCharRef &mode) { return channelModeType(QString(mode)); }
  
  inline const QString &networkName() const { return _networkName; }
  inline const QString &currentServer() const { return _currentServer; }
  inline const QString &myNick() const { return _myNick; }
  inline int latency() const { return _latency; }
  inline IrcUser *me() const { return ircUser(myNick()); }
  inline IdentityId identity() const { return _identity; }
  QStringList nicks() const;
  inline QStringList channels() const { return _ircChannels.keys(); }
  inline const QVariantList &serverList() const { return _serverList; }
  inline bool useRandomServer() const { return _useRandomServer; }
  inline const QStringList &perform() const { return _perform; }
  inline bool useAutoIdentify() const { return _useAutoIdentify; }
  inline const QString &autoIdentifyService() const { return _autoIdentifyService; }
  inline const QString &autoIdentifyPassword() const { return _autoIdentifyPassword; }
  inline bool useAutoReconnect() const { return _useAutoReconnect; }
  inline quint32 autoReconnectInterval() const { return _autoReconnectInterval; }
  inline quint16 autoReconnectRetries() const { return _autoReconnectRetries; }
  inline bool unlimitedReconnectRetries() const { return _unlimitedReconnectRetries; }
  inline bool rejoinChannels() const { return _rejoinChannels; }

  NetworkInfo networkInfo() const;
  void setNetworkInfo(const NetworkInfo &);

  QString prefixes();
  QString prefixModes();

  bool supports(const QString &param) const { return _supports.contains(param); }
  QString support(const QString &param) const;

  IrcUser *newIrcUser(const QString &hostmask);
  inline IrcUser *newIrcUser(const QByteArray &hostmask) { return newIrcUser(decodeServerString(hostmask)); }
  IrcUser *ircUser(QString nickname) const;
  inline IrcUser *ircUser(const QByteArray &nickname) const { return ircUser(decodeServerString(nickname)); }
  inline QList<IrcUser *> ircUsers() const { return _ircUsers.values(); }
  inline quint32 ircUserCount() const { return _ircUsers.count(); }

  IrcChannel *newIrcChannel(const QString &channelname);
  inline IrcChannel *newIrcChannel(const QByteArray &channelname) { return newIrcChannel(decodeServerString(channelname)); }
  IrcChannel *ircChannel(QString channelname) const;
  inline IrcChannel *ircChannel(const QByteArray &channelname) const { return ircChannel(decodeServerString(channelname)); }
  inline QList<IrcChannel *> ircChannels() const { return _ircChannels.values(); }
  inline quint32 ircChannelCount() const { return _ircChannels.count(); }

  QByteArray codecForServer() const;
  QByteArray codecForEncoding() const;
  QByteArray codecForDecoding() const;
  void setCodecForServer(QTextCodec *codec);
  void setCodecForEncoding(QTextCodec *codec);
  void setCodecForDecoding(QTextCodec *codec);

  QString decodeString(const QByteArray &text) const;
  QByteArray encodeString(const QString &string) const;
  QString decodeServerString(const QByteArray &text) const;
  QByteArray encodeServerString(const QString &string) const;

  static QByteArray defaultCodecForServer();
  static QByteArray defaultCodecForEncoding();
  static QByteArray defaultCodecForDecoding();
  static void setDefaultCodecForServer(const QByteArray &name);
  static void setDefaultCodecForEncoding(const QByteArray &name);
  static void setDefaultCodecForDecoding(const QByteArray &name);

public slots:
  void setNetworkName(const QString &networkName);
  void setCurrentServer(const QString &currentServer);
  void setConnected(bool isConnected);
  //void setConnectionState(Network::ConnectionState state);
  void setConnectionState(int state);
  void setMyNick(const QString &mynick);
  void setLatency(int latency);
  void setIdentity(IdentityId);

  void setServerList(const QVariantList &serverList);
  void setUseRandomServer(bool);
  void setPerform(const QStringList &);
  void setUseAutoIdentify(bool);
  void setAutoIdentifyService(const QString &);
  void setAutoIdentifyPassword(const QString &);
  void setUseAutoReconnect(bool);
  void setAutoReconnectInterval(quint32);
  void setAutoReconnectRetries(quint16);
  void setUnlimitedReconnectRetries(bool);
  void setRejoinChannels(bool);

  void setCodecForServer(const QByteArray &codecName);
  void setCodecForEncoding(const QByteArray &codecName);
  void setCodecForDecoding(const QByteArray &codecName);

  void addSupport(const QString &param, const QString &value = QString());
  void removeSupport(const QString &param);

  inline void addIrcUser(const QString &hostmask) { newIrcUser(hostmask); }
  inline void addIrcChannel(const QString &channel) { newIrcChannel(channel); }
  void removeIrcUser(const QString &nick);
  void removeIrcChannel(const QString &channel);

  //init geters
  QVariantMap initSupports() const;
  inline QVariantList initServerList() const { return serverList(); }
  virtual QVariantMap initIrcUsersAndChannels() const;
//   QStringList initIrcUsers() const;
//   QStringList initIrcChannels() const;
  
  //init seters
  void initSetSupports(const QVariantMap &supports);
  inline void initSetServerList(const QVariantList &serverList) { setServerList(serverList); }
  virtual void initSetIrcUsersAndChannels(const QVariantMap &usersAndChannels);
//   void initSetIrcUsers(const QStringList &hostmasks);
//   void initSetIrcChannels(const QStringList &channels);
  
  IrcUser *updateNickFromMask(const QString &mask);

  // these slots are to keep the hashlists of all users and the
  // channel lists up to date
  void ircUserNickChanged(QString newnick);

  virtual inline void requestConnect() const { emit connectRequested(); }
  virtual inline void requestDisconnect() const { emit disconnectRequested(); }

  void emitConnectionError(const QString &);

private slots:
  void ircUserDestroyed();
  void channelDestroyed();
  void removeIrcUser(IrcUser *ircuser);
  void removeIrcChannel(IrcChannel *ircChannel);
  void removeChansAndUsers();
  void ircUserInitDone();
  void ircChannelInitDone();

signals:
  void aboutToBeDestroyed();
  void networkNameSet(const QString &networkName);
  void currentServerSet(const QString &currentServer);
  void connectedSet(bool isConnected);
  void connectionStateSet(Network::ConnectionState);
  void connectionStateSet(int);
  void connectionError(const QString &errorMsg);
  void myNickSet(const QString &mynick);
  void latencySet(int latency);
  void identitySet(IdentityId);

  void serverListSet(QVariantList serverList);
  void useRandomServerSet(bool);
  void performSet(const QStringList &);
  void useAutoIdentifySet(bool);
  void autoIdentifyServiceSet(const QString &);
  void autoIdentifyPasswordSet(const QString &);
  void useAutoReconnectSet(bool);
  void autoReconnectIntervalSet(quint32);
  void autoReconnectRetriesSet(quint16);
  void unlimitedReconnectRetriesSet(bool);
  void rejoinChannelsSet(bool);

  void codecForServerSet(const QByteArray &codecName);
  void codecForEncodingSet(const QByteArray &codecName);
  void codecForDecodingSet(const QByteArray &codecName);

  void supportAdded(const QString &param, const QString &value);
  void supportRemoved(const QString &param);

  void ircUserAdded(const QString &hostmask);
  void ircUserAdded(IrcUser *);
  void ircChannelAdded(const QString &channelname);
  void ircChannelAdded(IrcChannel *);

  void ircUserRemoved(const QString &nick);
  void ircChannelRemoved(const QString &channel);

  // needed for client sync progress
  void ircUserRemoved(QObject *);
  void ircChannelRemoved(QObject *);

  void ircUserInitDone(IrcUser *);
  void ircChannelInitDone(IrcChannel *);

  void connectRequested(NetworkId id = 0) const;
  void disconnectRequested(NetworkId id = 0) const;

private:
  QPointer<SignalProxy> _proxy;

  NetworkId _networkId;
  IdentityId _identity;

  QString _myNick;
  int _latency;
  QString _networkName;
  QString _currentServer;
  bool _connected;
  ConnectionState _connectionState;

  QString _prefixes;
  QString _prefixModes;

  QHash<QString, IrcUser *> _ircUsers;  // stores all known nicks for the server
  QHash<QString, IrcChannel *> _ircChannels; // stores all known channels
  QHash<QString, QString> _supports;  // stores results from RPL_ISUPPORT

  QVariantList _serverList;
  bool _useRandomServer;
  QStringList _perform;

  bool _useAutoIdentify;
  QString _autoIdentifyService;
  QString _autoIdentifyPassword;

  bool _useAutoReconnect;
  quint32 _autoReconnectInterval;
  quint16 _autoReconnectRetries;
  bool _unlimitedReconnectRetries;
  bool _rejoinChannels;

  void determinePrefixes();

  QTextCodec *_codecForServer;
  QTextCodec *_codecForEncoding;
  QTextCodec *_codecForDecoding;

  static QTextCodec *_defaultCodecForServer;
  static QTextCodec *_defaultCodecForEncoding;
  static QTextCodec *_defaultCodecForDecoding;
};

//! Stores all editable information about a network (as opposed to runtime state).
struct NetworkInfo {
  NetworkId networkId;
  QString networkName;
  IdentityId identity;

  bool useCustomEncodings; // not used!
  QByteArray codecForServer;
  QByteArray codecForEncoding;
  QByteArray codecForDecoding;

  // Server entry: QString "Host", uint "Port", QString "Password", bool "UseSSL"
  QVariantList serverList;
  bool useRandomServer;

  QStringList perform;

  bool useAutoIdentify;
  QString autoIdentifyService;
  QString autoIdentifyPassword;

  bool useAutoReconnect;
  quint32 autoReconnectInterval;
  quint16 autoReconnectRetries;
  bool unlimitedReconnectRetries;
  bool rejoinChannels;

  bool operator==(const NetworkInfo &other) const;
  bool operator!=(const NetworkInfo &other) const;
};

QDataStream &operator<<(QDataStream &out, const NetworkInfo &info);
QDataStream &operator>>(QDataStream &in, NetworkInfo &info);
QDebug operator<<(QDebug dbg, const NetworkInfo &i);

Q_DECLARE_METATYPE(NetworkInfo);

#endif

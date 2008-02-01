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

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QVariantMap>
#include <QPointer>
#include <QMutex>

#include "types.h"
#include "syncableobject.h"

class SignalProxy;
class IrcUser;
class IrcChannel;

// defined below!
struct NetworkInfo;

// TODO: ConnectionInfo to propagate and sync the current state of NetworkConnection, encodings etcpp

class Network : public SyncableObject {
  Q_OBJECT
  Q_ENUMS(ConnectionState Network::ConnectionState)

  Q_PROPERTY(QString networkName READ networkName WRITE setNetworkName STORED false)
  Q_PROPERTY(QString currentServer READ currentServer WRITE setCurrentServer STORED false)
  Q_PROPERTY(QString myNick READ myNick WRITE setMyNick STORED false)
  Q_PROPERTY(QByteArray codecForEncoding READ codecForEncoding WRITE setCodecForEncoding STORED false)
  Q_PROPERTY(QByteArray codecForDecoding READ codecForDecoding WRITE setCodecForDecoding STORED false)
  Q_PROPERTY(IdentityId identityId READ identity WRITE setIdentity STORED false)
  Q_PROPERTY(bool isConnected READ isConnected WRITE setConnected STORED false)
  Q_PROPERTY(Network::ConnectionState connectionState READ connectionState WRITE setConnectionState STORED false)

public:
  enum ConnectionState { Disconnected, Connecting, Initializing, Initialized, Disconnecting };

  Network(const NetworkId &networkid, QObject *parent = 0);
  ~Network();

  NetworkId networkId() const;

  SignalProxy *proxy() const;
  void setProxy(SignalProxy *proxy);

  bool isMyNick(const QString &nick) const;
  bool isMe(IrcUser *ircuser) const;

  bool isChannelName(const QString &channelname) const;

  bool isConnected() const;
  Network::ConnectionState connectionState() const;

  QString prefixToMode(const QString &prefix);
  QString prefixToMode(const QCharRef &prefix);
  QString modeToPrefix(const QString &mode);
  QString modeToPrefix(const QCharRef &mode);

  QString networkName() const;
  QString currentServer() const;
  QString myNick() const;
  IdentityId identity() const;
  QStringList nicks() const;
  QStringList channels() const;
  QVariantList serverList() const;

  NetworkInfo networkInfo() const;
  void setNetworkInfo(const NetworkInfo &);

  QString prefixes();
  QString prefixModes();

  bool supports(const QString &param) const;
  QString support(const QString &param) const;

  IrcUser *newIrcUser(const QString &hostmask);
  IrcUser *newIrcUser(const QByteArray &hostmask);
  IrcUser *ircUser(QString nickname) const;
  IrcUser *ircUser(const QByteArray &nickname) const;
  QList<IrcUser *> ircUsers() const;
  quint32 ircUserCount() const;

  IrcChannel *newIrcChannel(const QString &channelname);
  IrcChannel *newIrcChannel(const QByteArray &channelname);
  IrcChannel *ircChannel(QString channelname) const;
  IrcChannel *ircChannel(const QByteArray &channelname) const;
  QList<IrcChannel *> ircChannels() const;
  quint32 ircChannelCount() const;

  QByteArray codecForEncoding() const;
  QByteArray codecForDecoding() const;
  void setCodecForEncoding(QTextCodec *codec);
  void setCodecForDecoding(QTextCodec *codec);

  QString decodeString(const QByteArray &text) const;
  QByteArray encodeString(const QString string) const;

public slots:
  void setNetworkName(const QString &networkName);
  void setCurrentServer(const QString &currentServer);
  void setConnected(bool isConnected);
  void setConnectionState(Network::ConnectionState state);
  void setMyNick(const QString &mynick);
  void setIdentity(IdentityId);

  void setServerList(const QVariantList &serverList);

  void setCodecForEncoding(const QByteArray &codecName);
  void setCodecForDecoding(const QByteArray &codecName);

  void addSupport(const QString &param, const QString &value = QString());
  void removeSupport(const QString &param);

  inline void addIrcUser(const QString &hostmask) { newIrcUser(hostmask); }
  void removeIrcUser(QString nick);
  
  //init geters
  QVariantMap initSupports() const;
  QVariantList initServerList() const;
  QStringList initIrcUsers() const;
  QStringList initIrcChannels() const;
  
  //init seters
  void initSetSupports(const QVariantMap &supports);
  void initSetServerList(const QVariantList &serverList);
  void initSetIrcUsers(const QStringList &hostmasks);
  void initSetChannels(const QStringList &channels);
  
  IrcUser *updateNickFromMask(const QString &mask);

  // these slots are to keep the hashlists of all users and the
  // channel lists up to date
  void ircUserNickChanged(QString newnick);

  void requestConnect() const;
  void requestDisconnect() const;

  void emitConnectionError(const QString &);

private slots:
  void ircUserDestroyed();
  void channelDestroyed();
  void removeIrcUser(IrcUser *ircuser);
  void ircUserInitDone();
  void ircChannelInitDone();

signals:
  void networkNameSet(const QString &networkName);
  void currentServerSet(const QString &currentServer);
  void connectedSet(bool isConnected);
  void connectionStateSet(Network::ConnectionState);
  void connectionError(const QString &errorMsg);
  void myNickSet(const QString &mynick);
  void identitySet(IdentityId);

  void serverListSet(QVariantList serverList);

  void codecForEncodingSet(const QString &codecName);
  void codecForDecodingSet(const QString &codecName);

  void supportAdded(const QString &param, const QString &value);
  void supportRemoved(const QString &param);

  void ircUserAdded(const QString &hostmask);
  void ircUserAdded(IrcUser *);
  void ircChannelAdded(const QString &channelname);
  void ircChannelAdded(IrcChannel *);

  void ircUserRemoved(const QString &nick);

  // needed for client sync progress
  void ircUserRemoved(QObject *);
  void ircChannelRemoved(QObject *);

  void ircUserInitDone(IrcUser *);
  void ircChannelInitDone(IrcChannel *);

  void connectRequested(NetworkId id = 0) const;
  void disconnectRequested(NetworkId id = 0) const;

private:
  NetworkId _networkId;
  IdentityId _identity;

  QString _myNick;
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
  QStringList _perform;
  //QVariantMap networkSettings;

  QPointer<SignalProxy> _proxy;
  void determinePrefixes();

  QTextCodec *_codecForEncoding;
  QTextCodec *_codecForDecoding;

};

//! Stores all editable information about a network (as opposed to runtime state).
struct NetworkInfo {
  NetworkId networkId;
  QString networkName;
  IdentityId identity;
  QByteArray codecForEncoding;
  QByteArray codecForDecoding;
  QVariantList serverList;

  bool operator==(const NetworkInfo &other) const;
  bool operator!=(const NetworkInfo &other) const;
};

QDataStream &operator<<(QDataStream &out, const NetworkInfo &info);
QDataStream &operator>>(QDataStream &in, NetworkInfo &info);

Q_DECLARE_METATYPE(NetworkInfo);

#endif

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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <QAbstractSocket>
#include <QTcpSocket>
#include <QList>
#include <QPointer>

#include "buffer.h" // needed for activity lvl

class BufferInfo;
class Message;
class MessageModel;
class AbstractMessageProcessor;

class Identity;
class Network;

class AbstractUi;
class AbstractUiMsg;
class NetworkModel;
class BufferModel;
class BufferSyncer;
class ClientBacklogManager;
class ClientIrcListHelper;
class BufferViewManager;
class IrcUser;
class IrcChannel;
class SignalProxy;
struct NetworkInfo;

class Client : public QObject {
  Q_OBJECT

public:
  enum ClientMode {
    LocalCore,
    RemoteCore
  };

  static Client *instance();
  static void destroy();
  static void init(AbstractUi *);

  static QList<BufferInfo> allBufferInfos();
  static QList<Buffer *> buffers();
  // static Buffer *buffer(BufferId bufferUid);
  static Buffer *buffer(BufferInfo);

  static QList<NetworkId> networkIds();
  static const Network * network(NetworkId);

  static QList<IdentityId> identityIds();
  static const Identity * identity(IdentityId);

  //! Request creation of an identity with the given data.
  /** The request will be sent to the core, and will be propagated back to all the clients
   *  with a new valid IdentityId.
   *  \param identity The identity template for the new identity. It does not need to have a valid ID.
   */
  static void createIdentity(const Identity &identity);

  //! Request update of an identity with the given data.
  /** The request will be sent to the core, and will be propagated back to all the clients.
   *  \param id The identity to be updated.
   *  \param serializedData The identity's content (cf. SyncableObject::toVariantMap())
   */
  static void updateIdentity(IdentityId id, const QVariantMap &serializedData);

  //! Request removal of the identity with the given ID from the core (and all the clients, of course).
  /** \param id The ID of the identity to be removed.
   */
  static void removeIdentity(IdentityId id);

  static void createNetwork(const NetworkInfo &info);
  static void updateNetwork(const NetworkInfo &info);
  static void removeNetwork(NetworkId id);

  static inline NetworkModel *networkModel() { return instance()->_networkModel; }
  static inline BufferModel *bufferModel() { return instance()->_bufferModel; }
  static inline MessageModel *messageModel() { return instance()->_messageModel; }
  static inline AbstractMessageProcessor *messageProcessor() { return instance()->_messageProcessor; }
  static inline SignalProxy *signalProxy() { return instance()->_signalProxy; }

  static inline ClientBacklogManager *backlogManager() { return instance()->_backlogManager; }
  static inline ClientIrcListHelper *ircListHelper() { return instance()->_ircListHelper; }
  static inline BufferViewManager *bufferViewManager() { return instance()->_bufferViewManager; }

  static AccountId currentCoreAccount();

  static bool isConnected();
  static bool isSynced();

  static void userInput(BufferInfo bufferInfo, QString message);

  static void setBufferLastSeenMsg(BufferId id, const MsgId &msgId); // this is synced to core and other clients
  static void removeBuffer(BufferId id);

signals:
  void sendInput(BufferInfo, QString message);
  void showBuffer(Buffer *);
  void bufferUpdated(BufferInfo bufferInfo);
  void backlogReceived(Buffer *, QList<Message>);
  void requestBacklog(BufferInfo, QVariant, QVariant);
  void requestNetworkStates();

  void showConfigWizard(const QVariantMap &coredata);

  void connected();
  void securedConnection();
  void disconnected();
  void coreConnectionStateChanged(bool);

  //! The identity with the given ID has been newly created in core and client.
  /** \param id The ID of the newly created identity.
   */
  void identityCreated(IdentityId id);

  //! The identity with the given ID has been removed.
  /** Upon emitting this signal, the identity is already gone from the core, and it will
   *  be deleted from the client immediately afterwards, so connected slots need to clean
   *  up their stuff.
   *  \param id The ID of the identity about to be removed.
   */
  void identityRemoved(IdentityId id);

  //! Sent to the core when an identity shall be created. Should not be used elsewhere.
  void requestCreateIdentity(const Identity &);
  //! Sent to the core when an identity shall be removed. Should not be used elsewhere.
  void requestRemoveIdentity(IdentityId);

  void networkCreated(NetworkId id);
  void networkRemoved(NetworkId id);

  void requestCreateNetwork(const NetworkInfo &info);
  void requestRemoveNetwork(NetworkId);

public slots:
  //void selectBuffer(Buffer *);

  void disconnectFromCore();

  void setCoreConfiguration(const QVariantMap &settings);

  void bufferRemoved(BufferId bufferId);
  void bufferRenamed(BufferId bufferId, const QString &newName);

private slots:
  //void coreSocketError(QAbstractSocket::SocketError);

  //void networkConnected(NetworkId);
  //void networkDisconnected(NetworkId);

  void recvMessage(const Message &message);
  void recvStatusMsg(QString network, QString message);
  void receiveBacklog(BufferId bufferId, const QVariantList &msgs);
  void updateBufferInfo(BufferInfo);

  void bufferDestroyed();
  void networkDestroyed();
  void coreIdentityCreated(const Identity &);
  void coreIdentityRemoved(IdentityId);
  void coreNetworkCreated(NetworkId);
  void coreNetworkRemoved(NetworkId);

  void setConnectedToCore(QIODevice *socket, AccountId id);
  void setSyncedToCore();
  void setSecuredConnection();


private:
  Client(QObject *parent = 0);
  virtual ~Client();
  void init();

  static void addNetwork(Network *);
  static void setCurrentCoreAccount(AccountId);
  static inline BufferSyncer *bufferSyncer() { return instance()->_bufferSyncer; }

  Buffer *statusBuffer(const NetworkId &networkid) const;

  static QPointer<Client> instanceptr;

  QPointer<QIODevice> socket;

  SignalProxy * _signalProxy;
  AbstractUi * mainUi;
  NetworkModel * _networkModel;
  BufferModel * _bufferModel;
  BufferSyncer * _bufferSyncer;
  ClientBacklogManager *_backlogManager;
  BufferViewManager *_bufferViewManager;
  ClientIrcListHelper *_ircListHelper;

  MessageModel *_messageModel;
  AbstractMessageProcessor *_messageProcessor;

  ClientMode clientMode;

  bool _connectedToCore, _syncedToCore;

  QHash<BufferId, Buffer *> _buffers;
  QHash<NetworkId, Buffer *> _statusBuffers; // fast lookup
  QHash<NetworkId, Network *> _networks;
  QHash<IdentityId, Identity *> _identities;

  static AccountId _currentCoreAccount;

  friend class ClientSyncer;
};

#endif

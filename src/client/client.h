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

class Identity;
class Network;


class AbstractUi;
class AbstractUiMsg;
class NetworkModel;
class BufferModel;
class IrcUser;
class IrcChannel;
class NickModel;
class SignalProxy;

class QTimer;


class Client : public QObject {
  Q_OBJECT

public:
  static Client *instance();
  static void destroy();
  static void init(AbstractUi *);

  static QList<BufferInfo> allBufferInfos();
  static QList<Buffer *> buffers();
  static Buffer *buffer(BufferId bufferUid);
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
   *  \param identity The identity to be updated.
   */
  static void updateIdentity(const Identity &identity);

  //! Request removal of the identity with the given ID from the core (and all the clients, of course).
  /** \param id The ID of the identity to be removed.
   */
  static void removeIdentity(IdentityId id);

  static void addNetwork(NetworkId id);
  static void addNetwork(Network *);


  static NetworkModel *networkModel();
  static BufferModel *bufferModel();
  static NickModel *nickModel();
  static SignalProxy *signalProxy();

  static AbstractUiMsg *layoutMsg(const Message &);

  static bool isConnected();
  static bool isSynced();

  static void fakeInput(BufferId bufferUid, QString message);
  static void fakeInput(BufferInfo bufferInfo, QString message);

  static void storeSessionData(const QString &key, const QVariant &data);
  static QVariant retrieveSessionData(const QString &key, const QVariant &def = QVariant());
  static QStringList sessionDataKeys();

  enum ClientMode { LocalCore, RemoteCore };

signals:
  void sendInput(BufferInfo, QString message);
  void showBuffer(Buffer *);
  void bufferUpdated(BufferInfo bufferInfo);
  void backlogReceived(Buffer *, QList<Message>);
  void requestBacklog(BufferInfo, QVariant, QVariant);
  void requestNetworkStates();

  void showConfigWizard(const QVariantMap &coredata);

  void connected();
  void disconnected();
  void coreConnectionStateChanged(bool);

  void sessionDataChanged(const QString &key);
  void sessionDataChanged(const QString &key, const QVariant &data);
  void sendSessionData(const QString &key, const QVariant &data);

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
  //! Sent to the core when an identity shall be updated. Should not be used elsewhere.
  void requestUpdateIdentity(const Identity &);
  //! Sent to the core when an identity shall be removed. Should not be used elsewhere.
  void requestRemoveIdentity(IdentityId);

  void networkAdded(NetworkId id);

public slots:
  //void selectBuffer(Buffer *);

  void setConnectedToCore(QIODevice *socket);
  void setSyncedToCore();
  void disconnectFromCore();

  void setCoreConfiguration(const QVariantMap &settings);


private slots:
  void recvSessionData(const QString &key, const QVariant &data);

  //void coreSocketError(QAbstractSocket::SocketError);

  void userInput(BufferInfo, QString);

  //void networkConnected(NetworkId);
  //void networkDisconnected(NetworkId);

  void recvMessage(const Message &message);
  void recvStatusMsg(QString network, QString message);
  void recvBacklogData(BufferInfo, QVariantList, bool);
  void updateBufferInfo(BufferInfo);

  void layoutMsg();

  void bufferDestroyed();
  void networkDestroyed();
  void coreIdentityCreated(const Identity &);
  void coreIdentityRemoved(IdentityId);

private:
  Client(QObject *parent = 0);
  virtual ~Client();
  void init();

  void syncToCore(const QVariantMap &sessionState);

  static QPointer<Client> instanceptr;

  QPointer<QIODevice> socket;
  QPointer<SignalProxy> _signalProxy;
  QPointer<AbstractUi> mainUi;
  QPointer<NetworkModel> _networkModel;
  QPointer<BufferModel> _bufferModel;
  QPointer<NickModel> _nickModel;

  ClientMode clientMode;

  bool _connectedToCore, _syncedToCore;

  QHash<BufferId, Buffer *> _buffers;
  QHash<NetworkId, Network *> _networks;
  QHash<IdentityId, Identity *> _identities;

  QTimer *layoutTimer;
  QList<Buffer *> layoutQueue;

  QVariantMap sessionData;

  friend class ClientSyncer;
};

#endif

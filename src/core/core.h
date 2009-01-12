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

#ifndef CORE_H
#define CORE_H

#include <QDateTime>
#include <QString>
#include <QVariant>
#include <QTimer>

#ifdef HAVE_SSL
#  include <QSslSocket>
#  include "sslserver.h"
#else
#  include <QTcpSocket>
#  include <QTcpServer>
#endif

#include "storage.h"
#include "bufferinfo.h"
#include "message.h"
#include "sessionthread.h"
#include "types.h"

class CoreSession;
class SessionThread;
class SignalProxy;
struct NetworkInfo;

class Core : public QObject {
  Q_OBJECT

  public:
  static Core * instance();
  static void destroy();

  static void saveState();
  static void restoreState();

  /*** Storage access ***/
  // These methods are threadsafe.

  //! Store a user setting persistently
  /**
   * \param userId       The users Id
   * \param settingName  The Name of the Setting
   * \param data         The Value
   */
  static inline void setUserSetting(UserId userId, const QString &settingName, const QVariant &data) {
    instance()->storage->setUserSetting(userId, settingName, data);
  }

  //! Retrieve a persistent user setting
  /**
   * \param userId       The users Id
   * \param settingName  The Name of the Setting
   * \param default      Value to return in case it's unset.
   * \return the Value of the Setting or the default value if it is unset.
   */
  static inline QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &data = QVariant()) {
    return instance()->storage->getUserSetting(userId, settingName, data);
  }

  /* Identity handling */
  static inline IdentityId createIdentity(UserId user, CoreIdentity &identity) {
    return instance()->storage->createIdentity(user, identity);
  }
  static bool updateIdentity(UserId user, const CoreIdentity &identity) {
    return instance()->storage->updateIdentity(user, identity);
  }
  static void removeIdentity(UserId user, IdentityId identityId) {
    instance()->storage->removeIdentity(user, identityId);
  }
  static QList<CoreIdentity> identities(UserId user) {
    return instance()->storage->identities(user);
  }

  //! Create a Network in the Storage and store it's Id in the given NetworkInfo
  /** \note This method is thredsafe.
   *
   *  \param user        The core user
   *  \param networkInfo a NetworkInfo definition to store the newly created ID in
   *  \return true if successfull.
   */
  static bool createNetwork(UserId user, NetworkInfo &info);

  //! Apply the changes to NetworkInfo info to the storage engine
  /** \note This method is thredsafe.
   *
   *  \param user        The core user
   *  \param networkInfo The Updated NetworkInfo
   *  \return true if successfull.
   */
  static inline bool updateNetwork(UserId user, const NetworkInfo &info) {
    return instance()->storage->updateNetwork(user, info);
  }

  //! Permanently remove a Network and all the data associated with it.
  /** \note This method is thredsafe.
   *
   *  \param user        The core user
   *  \param networkId   The network to delete
   *  \return true if successfull.
   */
  static inline bool removeNetwork(UserId user, const NetworkId &networkId) {
    return instance()->storage->removeNetwork(user, networkId);
  }

  //! Returns a list of all NetworkInfos for the given UserId user
  /** \note This method is thredsafe.
   *
   *  \param user        The core user
   *  \return QList<NetworkInfo>.
   */
  static inline QList<NetworkInfo> networks(UserId user) {
    return instance()->storage->networks(user);
  }

  //! Get the NetworkId for a network name.
  /** \note This method is threadsafe.
   *
   *  \param user    The core user
   *  \param network The name of the network
   *  \return The NetworkId corresponding to the given network.
   */
  static inline NetworkId networkId(UserId user, const QString &network) {
    return instance()->storage->getNetworkId(user, network);
  }

  //! Get a list of Networks to restore
  /** Return a list of networks the user was connected at the time of core shutdown
   *  \note This method is threadsafe.
   *
   *  \param user  The User Id in question
   */
  static inline QList<NetworkId> connectedNetworks(UserId user) {
    return instance()->storage->connectedNetworks(user);
  }

  //! Update the connected state of a network
  /** \note This method is threadsafe
   *
   *  \param user        The Id of the networks owner
   *  \param networkId   The Id of the network
   *  \param isConnected whether the network is connected or not
   */
  static inline void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected) {
    return instance()->storage->setNetworkConnected(user, networkId, isConnected);
  }

  //! Get a hash of channels with their channel keys for a given network
  /** The keys are channel names and values are passwords (possibly empty)
   *  \note This method is threadsafe
   *
   *  \param user       The id of the networks owner
   *  \param networkId  The Id of the network
   */
  static inline QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId) {
    return instance()->storage->persistentChannels(user, networkId);
  }

  //! Update the connected state of a channel
  /** \note This method is threadsafe
   *
   *  \param user       The Id of the networks owner
   *  \param networkId  The Id of the network
   *  \param channel    The name of the channel
   *  \param isJoined   whether the channel is connected or not
   */
  static inline void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined) {
    return instance()->storage->setChannelPersistent(user, networkId, channel, isJoined);
  }

  //! Update the key of a channel
  /** \note This method is threadsafe
   *
   *  \param user       The Id of the networks owner
   *  \param networkId  The Id of the network
   *  \param channel    The name of the channel
   *  \param key        The key of the channel (possibly empty)
   */
  static inline void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key) {
    return instance()->storage->setPersistentChannelKey(user, networkId, channel, key);
  }

  //! Get the unique BufferInfo for the given combination of network and buffername for a user.
  /** \note This method is threadsafe.
   *
   *  \param user      The core user who owns this buffername
   *  \param networkId The network id
   *  \param type      The type of the buffer (StatusBuffer, Channel, etc.)
   *  \param buffer    The buffer name (if empty, the net's status buffer is returned)
   *  \param create    Whether or not the buffer should be created if it doesnt exist
   *  \return The BufferInfo corresponding to the given network and buffer name, or 0 if not found
   */
  static inline BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "", bool create = true) {
    return instance()->storage->bufferInfo(user, networkId, type, buffer, create);
  }

  //! Get the unique BufferInfo for a bufferId
  /** \note This method is threadsafe
   *  \param user      The core user who owns this buffername
   *  \param bufferId  The id of the buffer
   *  \return The BufferInfo corresponding to the given buffer id, or an invalid BufferInfo if not found.
   */
  static inline BufferInfo getBufferInfo(UserId user, const BufferId &bufferId) {
    return instance()->storage->getBufferInfo(user, bufferId);
  }

  //! Store a Message in the backlog.
  /** \note This method is threadsafe.
   *
   *  \param msg  The message object to be stored
   *  \return The globally unique id for the stored message
   */
  static inline MsgId storeMessage(const Message &message) {
    return instance()->storage->logMessage(message);
  }

  //! Request a certain number messages stored in a given buffer.
  /** \param buffer   The buffer we request messages from
   *  \param first    if != -1 return only messages with a MsgId >= first
   *  \param last     if != -1 return only messages with a MsgId < last
   *  \param limit    if != -1 limit the returned list to a max of \limit entries
   *  \return The requested list of messages
   */
  static inline QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1) {
    return instance()->storage->requestMsgs(user, bufferId, first, last, limit);
  }

  //! Request a certain number of messages across all buffers
  /** \param first    if != -1 return only messages with a MsgId >= first
   *  \param last     if != -1 return only messages with a MsgId < last
   *  \param limit    Max amount of messages
   *  \return The requested list of messages
   */
  static inline QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1) {
    return instance()->storage->requestAllMsgs(user, first, last, limit);
  }

  //! Request a list of all buffers known to a user.
  /** This method is used to get a list of all buffers we have stored a backlog from.
   *  \note This method is threadsafe.
   *
   *  \param user  The user whose buffers we request
   *  \return A list of the BufferInfos for all buffers as requested
   */
  static inline QList<BufferInfo> requestBuffers(UserId user) {
    return instance()->storage->requestBuffers(user);
  }

  //! Request a list of BufferIds for a given NetworkId
  /** \note This method is threadsafe.
   *
   *  \param user  The user whose buffers we request
   *  \param networkId  The NetworkId of the network in question
   *  \return List of BufferIds belonging to the Network
   */
  static inline QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId) {
    return instance()->storage->requestBufferIdsForNetwork(user, networkId);
  }

  //! Remove permanently a buffer and it's content from the storage backend
  /** This call cannot be reverted!
   *  \note This method is threadsafe.
   *
   *  \param user      The user who is the owner of the buffer
   *  \param bufferId  The bufferId
   *  \return true if successfull
   */
  static inline bool removeBuffer(const UserId &user, const BufferId &bufferId) {
    return instance()->storage->removeBuffer(user, bufferId);
  }

  //! Rename a Buffer
  /** \note This method is threadsafe.
   *  \param user      The id of the buffer owner
   *  \param bufferId  The bufferId
   *  \param newName   The new name of the buffer
   *  \return true if successfull
   */
  static inline bool renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName) {
    return instance()->storage->renameBuffer(user, bufferId, newName);
  }

  //! Merge the content of two Buffers permanently. This cannot be reversed!
  /** \note This method is threadsafe.
   *  \param user      The id of the buffer owner
   *  \param bufferId1 The bufferId of the remaining buffer
   *  \param bufferId2 The buffer that is about to be removed
   *  \return true if successfulln
   */
  static inline bool mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2) {
    return instance()->storage->mergeBuffersPermanently(user, bufferId1, bufferId2);
  }

  //! Update the LastSeenDate for a Buffer
  /** This Method is used to make the LastSeenDate of a Buffer persistent
   *  \note This method is threadsafe.
   *
   * \param user      The Owner of that Buffer
   * \param bufferId  The buffer id
   * \param MsgId     The Message id of the message that has been just seen
   */
  static inline void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId) {
    return instance()->storage->setBufferLastSeenMsg(user, bufferId, msgId);
  }

  //! Get a Hash of all last seen message ids
  /** This Method is called when the Quassel Core is started to restore the lastSeenMsgIds
   *  \note This method is threadsafe.
   *
   * \param user      The Owner of the buffers
   */
  static inline QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user) {
    return instance()->storage->bufferLastSeenMsgIds(user);
  }

  const QDateTime &startTime() const { return _startTime; }

  static inline QTimer &syncTimer() { return instance()->_storageSyncTimer; }

public slots:
  //! Make storage data persistent
  /** \note This method is threadsafe.
   */
  void syncStorage();
  void setupInternalClientSession(SignalProxy *proxy);
signals:
  //! Sent when a BufferInfo is updated in storage.
  void bufferInfoUpdated(UserId user, const BufferInfo &info);

  //! Relay From CoreSession::sessionState(const QVariant &). Used for internal connection only
  void sessionState(const QVariant &);

private slots:
  bool startListening();
  void stopListening(const QString &msg = QString());
  void incomingConnection();
  void clientHasData();
  void clientDisconnected();

  bool initStorage(QVariantMap dbSettings, bool setup = false);

#ifdef HAVE_SSL
  void sslErrors(const QList<QSslError> &errors);
#endif
  void socketError(QAbstractSocket::SocketError);

private:
  Core();
  ~Core();
  void init();
  static Core *instanceptr;

  SessionThread *createSession(UserId userId, bool restoreState = false);
  void setupClientSession(QTcpSocket *socket, UserId uid);
  void processClientMessage(QTcpSocket *socket, const QVariantMap &msg);
  //void processCoreSetup(QTcpSocket *socket, QVariantMap &msg);
  QString setupCoreForInternalUsage();
  QString setupCore(QVariantMap setupData);

  bool registerStorageBackend(Storage *);
  void unregisterStorageBackend(Storage *);

  QHash<UserId, SessionThread *> sessions;
  Storage *storage;
  QTimer _storageSyncTimer;

#ifdef HAVE_SSL
  SslServer _server, _v6server;
#else
  QTcpServer _server, _v6server;
#endif

  QHash<QTcpSocket *, quint32> blocksizes;
  QHash<QTcpSocket *, QVariantMap> clientInfo;

  QHash<QString, Storage *> _storageBackends;

  QDateTime _startTime;

  bool configured;
};

#endif

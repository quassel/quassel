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

#ifndef _CORE_H_
#define _CORE_H_

#include <QDateTime>
#include <QMutex>
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

#include "bufferinfo.h"
#include "message.h"
#include "global.h"
#include "sessionthread.h"
#include "types.h"

class CoreSession;
class SessionThread;
class Storage;
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
    static void setUserSetting(UserId userId, const QString &settingName, const QVariant &data);
  
    //! Retrieve a persistent user setting
    /**
     * \param userId       The users Id
     * \param settingName  The Name of the Setting
     * \param default      Value to return in case it's unset.
     * \return the Value of the Setting or the default value if it is unset.
     */
    static QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &data = QVariant());

  
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
    static bool updateNetwork(UserId user, const NetworkInfo &info);

    //! Permanently remove a Network and all the data associated with it.
    /** \note This method is thredsafe.
     *
     *  \param user        The core user
     *  \param networkId   The network to delete
     *  \return true if successfull.
     */
    static bool removeNetwork(UserId user, const NetworkId &networkId);
  
    //! Returns a list of all NetworkInfos for the given UserId user
    /** \note This method is thredsafe.
     *
     *  \param user        The core user
     *  \return QList<NetworkInfo>.
     */
    static QList<NetworkInfo> networks(UserId user);

    //! Get the NetworkId for a network name.
    /** \note This method is threadsafe.
     *
     *  \param user    The core user
     *  \param network The name of the network
     *  \return The NetworkId corresponding to the given network.
     */
    static NetworkId networkId(UserId user, const QString &network);

    //! Get a list of Networks to restore
    /** Return a list of networks the user was connected at the time of core shutdown
     *  \note This method is threadsafe.
     *
     *  \param user  The User Id in question
     */
    static QList<NetworkId> connectedNetworks(UserId user);

    //! Update the connected state of a network
    /** \note This method is threadsafe
     *
     *  \param user        The Id of the networks owner
     *  \param networkId   The Id of the network
     *  \param isConnected whether the network is connected or not
     */
    static void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected);

    //! Get a hash of channels with their channel keys for a given network
    /** The keys are channel names and values are passwords (possibly empty)
     *  \note This method is threadsafe
     *
     *  \param user       The id of the networks owner
     *  \param networkId  The Id of the network
     */
    static QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId);

    //! Update the connected state of a channel
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param channel    The name of the channel
     *  \param isJoined   whether the channel is connected or not
     */
    static void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined);

    //! Update the key of a channel
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param channel    The name of the channel
     *  \param key        The key of the channel (possibly empty)
     */
    static void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key);

    //! Get the unique BufferInfo for the given combination of network and buffername for a user.
    /** \note This method is threadsafe.
     *
     *  \param user      The core user who owns this buffername
     *  \param networkId The network id
     *  \param type      The type of the buffer (StatusBuffer, Channel, etc.)
     *  \param buffer    The buffer name (if empty, the net's status buffer is returned)
     *  \return The BufferInfo corresponding to the given network and buffer name, or 0 if not found
     */
    static BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type, const QString &buffer = "");

    //! Get the unique BufferInfo for a bufferId
    /** \note This method is threadsafe
     *  \param user      The core user who owns this buffername
     *  \param bufferId  The id of the buffer
     *  \return The BufferInfo corresponding to the given buffer id, or an invalid BufferInfo if not found.
     */
    static BufferInfo getBufferInfo(UserId user, const BufferId &bufferId);

  
    //! Store a Message in the backlog.
    /** \note This method is threadsafe.
     *
     *  \param msg  The message object to be stored
     *  \return The globally unique id for the stored message
     */
    static MsgId storeMessage(const Message &message);

    //! Request a certain number (or all) messages stored in a given buffer.
    /** \note This method is threadsafe.
     *
     *  \param buffer   The buffer we request messages from
     *  \param lastmsgs The number of messages we would like to receive, or -1 if we'd like all messages from that buffername
     *  \param offset   Do not return (but DO count) messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    static QList<Message> requestMsgs(UserId user, BufferId buffer, int lastmsgs = -1, int offset = -1);

    //! Request messages stored in a given buffer since a certain point in time.
    /** \note This method is threadsafe.
     *
     *  \param buffer   The buffer we request messages from
     *  \param since    Only return messages newer than this point in time
     *  \param offset   Do not return messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    static QList<Message> requestMsgs(UserId user, BufferId buffer, QDateTime since, int offset = -1);

    //! Request a range of messages stored in a given buffer.
    /** \note This method is threadsafe.
     *
     *  \param buffer   The buffer we request messages from
     *  \param first    Return messages with first <= MsgId <= last
     *  \param last     Return messages with first <= MsgId <= last
     *  \return The requested list of messages
     */
    static QList<Message> requestMsgRange(UserId user, BufferId buffer, int first, int last);

    //! Request a list of all buffers known to a user.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  \note This method is threadsafe.
     *
     *  \param user  The user whose buffers we request
     *  \return A list of the BufferInfos for all buffers as requested
     */
    static QList<BufferInfo> requestBuffers(UserId user);


    //! Request a list of BufferIds for a given NetworkId
    /** \note This method is threadsafe.
    *
    *  \param user  The user whose buffers we request
    *  \param networkId  The NetworkId of the network in question
    *  \return List of BufferIds belonging to the Network
    */
    static QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId);

    //! Remove permanently a buffer and it's content from the storage backend
    /** This call cannot be reverted!
     *  \note This method is threadsafe.
     *
     *  \param user      The user who is the owner of the buffer
     *  \param bufferId  The bufferId
     *  \return true if successfull
     */
    static bool removeBuffer(const UserId &user, const BufferId &bufferId);

    //! Rename a Buffer
    /** \note This method is threadsafe.
     *  \param user      The id of the buffer owner
     *  \param networkId The id of the network the buffer belongs to
     *  \param newName   The new name of the buffer
     *  \param oldName   The previous name of the buffer
     *  \return the BufferId of the affected buffer or an invalid BufferId if not successfull
     */
    static BufferId renameBuffer(const UserId &user, const NetworkId &networkId, const QString &newName, const QString &oldName);

    //! Update the LastSeenDate for a Buffer
    /** This Method is used to make the LastSeenDate of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id of the message that has been just seen
     */
    static void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId);

    //! Get a Hash of all last seen message ids
    /** This Method is called when the Quassel Core is started to restore the lastSeenMsgIds
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    static QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user);

  const QDateTime &startTime() const { return _startTime; }

  public slots:
    //! Make storage data persistent
    /** \note This method is threadsafe.
     */
    void syncStorage();
  
  signals:
    //! Sent when a BufferInfo is updated in storage.
    void bufferInfoUpdated(UserId user, const BufferInfo &info);

  private slots:
    bool startListening(uint port = Global::parser.value("port").toUInt());
    void stopListening();
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
    QString setupCore(const QVariant &setupData);

    bool registerStorageBackend(Storage *);
    void unregisterStorageBackend(Storage *);

    QHash<UserId, SessionThread *> sessions;
    Storage *storage;
    QTimer _storageSyncTimer;

#ifdef HAVE_SSL  
    SslServer server;
#else
    QTcpServer server;
#endif  

    QHash<QTcpSocket *, quint32> blocksizes;
    QHash<QTcpSocket *, QVariantMap> clientInfo;

    QHash<QString, Storage *> _storageBackends;

    QDateTime _startTime;

    bool configured;

    static QMutex mutex;
};

#endif

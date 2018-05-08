/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#pragma once

#include <memory>
#include <vector>

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

#include "authenticator.h"
#include "bufferinfo.h"
#include "deferredptr.h"
#include "message.h"
#include "oidentdconfiggenerator.h"
#include "sessionthread.h"
#include "storage.h"
#include "types.h"

class CoreAuthHandler;
class CoreSession;
struct NetworkInfo;
class SessionThread;
class SignalProxy;

class AbstractSqlMigrationReader;
class AbstractSqlMigrationWriter;

class Core : public QObject
{
    Q_OBJECT

public:
    static Core *instance();
    static void destroy();

    static void saveState();
    static void restoreState();

    /*** Storage access ***/
    // These methods are threadsafe.

    //! Validate user
    /**
     * \param userName The user's login name
     * \param password The user's uncrypted password
     * \return The user's ID if valid; 0 otherwise
     */
    static inline UserId validateUser(const QString &userName, const QString &password) {
        return instance()->_storage->validateUser(userName, password);
    }

    //! Authenticate user against auth backend
    /**
     * \param userName The user's login name
     * \param password The user's uncrypted password
     * \return The user's ID if valid; 0 otherwise
     */
    static inline UserId authenticateUser(const QString &userName, const QString &password) {
        return instance()->_authenticator->validateUser(userName, password);
    }

    //! Add a new user, exposed so auth providers can call this without being the storage.
    /**
     * \param userName The user's login name
     * \param password The user's uncrypted password
     * \param authenticator The name of the auth provider service used to log the user in, defaults to "Database".
     * \return The user's ID if valid; 0 otherwise
     */
    static inline UserId addUser(const QString &userName, const QString &password, const QString &authenticator = "Database") {
        return instance()->_storage->addUser(userName, password, authenticator);
    }

    //! Does a comparison test against the authenticator in the database and the authenticator currently in use for a UserID.
    /**
     * \param userid The user's ID (note: not login name).
     * \param authenticator The name of the auth provider service used to log the user in, defaults to "Database".
     * \return True if the userid was configured with the passed authenticator, false otherwise.
     */
    static inline bool checkAuthProvider(const UserId userid, const QString &authenticator) {
        return instance()->_storage->getUserAuthenticator(userid) == authenticator;
    }

    //! Change a user's password
    /**
     * \param userId     The user's ID
     * \param password   The user's unencrypted new password
     * \return true, if the password change was successful
     */
    static bool changeUserPassword(UserId userId, const QString &password);

    //! Check if we can change a user password.
    /**
     * \param userID     The user's ID
     * \return true, if we can change their password, false otherwise
     */
    static bool canChangeUserPassword(UserId userId);

    //! Store a user setting persistently
    /**
     * \param userId       The users Id
     * \param settingName  The Name of the Setting
     * \param data         The Value
     */
    static inline void setUserSetting(UserId userId, const QString &settingName, const QVariant &data)
    {
        instance()->_storage->setUserSetting(userId, settingName, data);
    }


    //! Retrieve a persistent user setting
    /**
     * \param userId       The users Id
     * \param settingName  The Name of the Setting
     * \param defaultValue Value to return in case it's unset.
     * \return the Value of the Setting or the default value if it is unset.
     */
    static inline QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &defaultValue = QVariant())
    {
        return instance()->_storage->getUserSetting(userId, settingName, defaultValue);
    }


    /* Identity handling */
    static inline IdentityId createIdentity(UserId user, CoreIdentity &identity)
    {
        return instance()->_storage->createIdentity(user, identity);
    }


    static bool updateIdentity(UserId user, const CoreIdentity &identity)
    {
        return instance()->_storage->updateIdentity(user, identity);
    }


    static void removeIdentity(UserId user, IdentityId identityId)
    {
        instance()->_storage->removeIdentity(user, identityId);
    }


    static QList<CoreIdentity> identities(UserId user)
    {
        return instance()->_storage->identities(user);
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
    static inline bool updateNetwork(UserId user, const NetworkInfo &info)
    {
        return instance()->_storage->updateNetwork(user, info);
    }


    //! Permanently remove a Network and all the data associated with it.
    /** \note This method is thredsafe.
     *
     *  \param user        The core user
     *  \param networkId   The network to delete
     *  \return true if successfull.
     */
    static inline bool removeNetwork(UserId user, const NetworkId &networkId)
    {
        return instance()->_storage->removeNetwork(user, networkId);
    }


    //! Returns a list of all NetworkInfos for the given UserId user
    /** \note This method is thredsafe.
     *
     *  \param user        The core user
     *  \return QList<NetworkInfo>.
     */
    static inline QList<NetworkInfo> networks(UserId user)
    {
        return instance()->_storage->networks(user);
    }


    //! Get a list of Networks to restore
    /** Return a list of networks the user was connected at the time of core shutdown
     *  \note This method is threadsafe.
     *
     *  \param user  The User Id in question
     */
    static inline QList<NetworkId> connectedNetworks(UserId user)
    {
        return instance()->_storage->connectedNetworks(user);
    }


    //! Update the connected state of a network
    /** \note This method is threadsafe
     *
     *  \param user        The Id of the networks owner
     *  \param networkId   The Id of the network
     *  \param isConnected whether the network is connected or not
     */
    static inline void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected)
    {
        return instance()->_storage->setNetworkConnected(user, networkId, isConnected);
    }


    //! Get a hash of channels with their channel keys for a given network
    /** The keys are channel names and values are passwords (possibly empty)
     *  \note This method is threadsafe
     *
     *  \param user       The id of the networks owner
     *  \param networkId  The Id of the network
     */
    static inline QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId)
    {
        return instance()->_storage->persistentChannels(user, networkId);
    }


    //! Update the connected state of a channel
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param channel    The name of the channel
     *  \param isJoined   whether the channel is connected or not
     */
    static inline void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined)
    {
        return instance()->_storage->setChannelPersistent(user, networkId, channel, isJoined);
    }


    //! Get a hash of buffers with their ciphers for a given network
    /** The keys are channel names and values are ciphers (possibly empty)
     *  \note This method is threadsafe
     *
     *  \param user       The id of the networks owner
     *  \param networkId  The Id of the network
     */
    static inline QHash<QString, QByteArray> bufferCiphers(UserId user, const NetworkId &networkId)
    {
        return instance()->_storage->bufferCiphers(user, networkId);
    }


    //! Update the cipher of a buffer
    /** \note This method is threadsafe
     *
     *  \param user        The Id of the networks owner
     *  \param networkId   The Id of the network
     *  \param bufferName The Cname of the buffer
     *  \param cipher      The cipher for the buffer
     */
    static inline void setBufferCipher(UserId user, const NetworkId &networkId, const QString &bufferName, const QByteArray &cipher)
    {
        return instance()->_storage->setBufferCipher(user, networkId, bufferName, cipher);
    }


    //! Update the key of a channel
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param channel    The name of the channel
     *  \param key        The key of the channel (possibly empty)
     */
    static inline void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key)
    {
        return instance()->_storage->setPersistentChannelKey(user, networkId, channel, key);
    }


    //! retrieve last known away message for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     */
    static inline QString awayMessage(UserId user, NetworkId networkId)
    {
        return instance()->_storage->awayMessage(user, networkId);
    }


    //! Make away message persistent for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param awayMsg    The current away message of own user
     */
    static inline void setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg)
    {
        return instance()->_storage->setAwayMessage(user, networkId, awayMsg);
    }


    //! retrieve last known user mode for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     */
    static inline QString userModes(UserId user, NetworkId networkId)
    {
        return instance()->_storage->userModes(user, networkId);
    }


    //! Make our user modes persistent for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param userModes  The current user modes of own user
     */
    static inline void setUserModes(UserId user, NetworkId networkId, const QString &userModes)
    {
        return instance()->_storage->setUserModes(user, networkId, userModes);
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
    static inline BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "", bool create = true)
    {
        return instance()->_storage->bufferInfo(user, networkId, type, buffer, create);
    }


    //! Get the unique BufferInfo for a bufferId
    /** \note This method is threadsafe
     *  \param user      The core user who owns this buffername
     *  \param bufferId  The id of the buffer
     *  \return The BufferInfo corresponding to the given buffer id, or an invalid BufferInfo if not found.
     */
    static inline BufferInfo getBufferInfo(UserId user, const BufferId &bufferId)
    {
        return instance()->_storage->getBufferInfo(user, bufferId);
    }


    //! Store a Message in the storage backend and set it's unique Id.
    /** \note This method is threadsafe.
     *
     *  \param message The message object to be stored
     *  \return true on success
     */
    static inline bool storeMessage(Message &message)
    {
        return instance()->_storage->logMessage(message);
    }


    //! Store a list of Messages in the storage backend and set their unique Id.
    /** \note This method is threadsafe.
     *
     *  \param messages The list message objects to be stored
     *  \return true on success
     */
    static inline bool storeMessages(MessageList &messages)
    {
        return instance()->_storage->logMessages(messages);
    }


    //! Request a certain number messages stored in a given buffer.
    /** \param buffer   The buffer we request messages from
     *  \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    if != -1 limit the returned list to a max of \limit entries
     *  \return The requested list of messages
     */
    static inline QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1)
    {
        return instance()->_storage->requestMsgs(user, bufferId, first, last, limit);
    }


    //! Request a certain number messages stored in a given buffer, matching certain filters
    /** \param buffer   The buffer we request messages from
     *  \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    if != -1 limit the returned list to a max of \limit entries
     *  \param type     The Message::Types that should be returned
     *  \return The requested list of messages
     */
    static inline QList<Message> requestMsgsFiltered(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1,
                                                     int limit = -1, Message::Types type = Message::Types{-1},
                                                     Message::Flags flags = Message::Flags{-1})
    {
        return instance()->_storage->requestMsgsFiltered(user, bufferId, first, last, limit, type, flags);
    }


    //! Request a certain number of messages across all buffers
    /** \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    Max amount of messages
     *  \return The requested list of messages
     */
    static inline QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1)
    {
        return instance()->_storage->requestAllMsgs(user, first, last, limit);
    }


    //! Request a certain number of messages across all buffers, matching certain filters
    /** \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    Max amount of messages
     *  \param type     The Message::Types that should be returned
     *  \return The requested list of messages
     */
    static inline QList<Message> requestAllMsgsFiltered(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1,
                                                        Message::Types type = Message::Types{-1},
                                                        Message::Flags flags = Message::Flags{-1})
    {
        return instance()->_storage->requestAllMsgsFiltered(user, first, last, limit, type, flags);
    }


    //! Request a list of all buffers known to a user.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  \note This method is threadsafe.
     *
     *  \param user  The user whose buffers we request
     *  \return A list of the BufferInfos for all buffers as requested
     */
    static inline QList<BufferInfo> requestBuffers(UserId user)
    {
        return instance()->_storage->requestBuffers(user);
    }


    //! Request a list of BufferIds for a given NetworkId
    /** \note This method is threadsafe.
     *
     *  \param user  The user whose buffers we request
     *  \param networkId  The NetworkId of the network in question
     *  \return List of BufferIds belonging to the Network
     */
    static inline QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId)
    {
        return instance()->_storage->requestBufferIdsForNetwork(user, networkId);
    }


    //! Remove permanently a buffer and it's content from the storage backend
    /** This call cannot be reverted!
     *  \note This method is threadsafe.
     *
     *  \param user      The user who is the owner of the buffer
     *  \param bufferId  The bufferId
     *  \return true if successfull
     */
    static inline bool removeBuffer(const UserId &user, const BufferId &bufferId)
    {
        return instance()->_storage->removeBuffer(user, bufferId);
    }


    //! Rename a Buffer
    /** \note This method is threadsafe.
     *  \param user      The id of the buffer owner
     *  \param bufferId  The bufferId
     *  \param newName   The new name of the buffer
     *  \return true if successfull
     */
    static inline bool renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName)
    {
        return instance()->_storage->renameBuffer(user, bufferId, newName);
    }


    //! Merge the content of two Buffers permanently. This cannot be reversed!
    /** \note This method is threadsafe.
     *  \param user      The id of the buffer owner
     *  \param bufferId1 The bufferId of the remaining buffer
     *  \param bufferId2 The buffer that is about to be removed
     *  \return true if successfulln
     */
    static inline bool mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2)
    {
        return instance()->_storage->mergeBuffersPermanently(user, bufferId1, bufferId2);
    }


    //! Update the LastSeenDate for a Buffer
    /** This Method is used to make the LastSeenDate of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id of the message that has been just seen
     */
    static inline void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId)
    {
        return instance()->_storage->setBufferLastSeenMsg(user, bufferId, msgId);
    }

    //! Get the auth username associated with a userId
    /** \param user  The user to retrieve the username for
     *  \return      The username for the user
     */
    static inline QString getAuthUserName(UserId user) {
        return instance()->_storage->getAuthUserName(user);
    }

    //! Get a usable sysident for the given user in oidentd-strict mode
    /** \param user    The user to retrieve the sysident for
     *  \return The authusername
     */
    QString strictSysIdent(UserId user) const;


    //! Get a Hash of all last seen message ids
    /** This Method is called when the Quassel Core is started to restore the lastSeenMsgIds
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    static inline QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user)
    {
        return instance()->_storage->bufferLastSeenMsgIds(user);
    }


    //! Update the MarkerLineMsgId for a Buffer
    /** This Method is used to make the marker line position of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id where the marker line should be placed
     */
    static inline void setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId)
    {
        return instance()->_storage->setBufferMarkerLineMsg(user, bufferId, msgId);
    }


    //! Get a Hash of all marker line message ids
    /** This Method is called when the Quassel Core is started to restore the MarkerLineMsgIds
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    static inline QHash<BufferId, MsgId> bufferMarkerLineMsgIds(UserId user)
    {
        return instance()->_storage->bufferMarkerLineMsgIds(user);
    }

    //! Update the BufferActivity for a Buffer
    /** This Method is used to make the activity state of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id where the marker line should be placed
     */
    static inline void setBufferActivity(UserId user, BufferId bufferId, Message::Types activity) {
        return instance()->_storage->setBufferActivity(user, bufferId, activity);
    }


    //! Get a Hash of all buffer activity states
    /** This Method is called when the Quassel Core is started to restore the BufferActivity
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    static inline QHash<BufferId, Message::Types> bufferActivities(UserId user) {
        return instance()->_storage->bufferActivities(user);
    }

    //! Get the bitset of buffer activity states for a buffer
    /** This method is used to load the activity state of a buffer when its last seen message changes.
     *  \note This method is threadsafe.
     *
     * \param bufferId The buffer
     * \param lastSeenMsgId     The last seen message
     */
    static inline Message::Types bufferActivity(BufferId bufferId, MsgId lastSeenMsgId) {
        return instance()->_storage->bufferActivity(bufferId, lastSeenMsgId);
    }

    //! Update the highlight count for a Buffer
    /** This Method is used to make the highlight count state of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id where the marker line should be placed
     */
    static inline void setHighlightCount(UserId user, BufferId bufferId, int highlightCount) {
        return instance()->_storage->setHighlightCount(user, bufferId, highlightCount);
    }


    //! Get a Hash of all highlight count states
    /** This Method is called when the Quassel Core is started to restore the highlight count
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    static inline QHash<BufferId, int> highlightCounts(UserId user) {
        return instance()->_storage->highlightCounts(user);
    }
    //! Get the highlight count states for a buffer
    /** This method is used to load the highlight count of a buffer when its last seen message changes.
     *  \note This method is threadsafe.
     *
     * \param bufferId The buffer
     * \param lastSeenMsgId     The last seen message
     */
    static inline int highlightCount(BufferId bufferId, MsgId lastSeenMsgId) {
        return instance()->_storage->highlightCount(bufferId, lastSeenMsgId);
    }

    static inline QDateTime startTime() { return instance()->_startTime; }
    static inline bool isConfigured() { return instance()->_configured; }
    static bool sslSupported();

    /**
     * Reloads SSL certificates used for connection with clients
     *
     * @return True if certificates reloaded successfully, otherwise false.
     */
    static bool reloadCerts();

    static void cacheSysIdent();

    static QVariantList backendInfo();
    static QVariantList authenticatorInfo();

    static QString setup(const QString &adminUser, const QString &adminPassword, const QString &backend, const QVariantMap &setupData, const QString &authenticator, const QVariantMap &authSetupMap);

    static inline QTimer &syncTimer() { return instance()->_storageSyncTimer; }

    inline OidentdConfigGenerator *oidentdConfigGenerator() const { return _oidentdConfigGenerator; }

    static const int AddClientEventId;

public slots:
    //! Make storage data persistent
    /** \note This method is threadsafe.
     */
    void syncStorage();
    void setupInternalClientSession(InternalPeer *clientConnection);
    QString setupCore(const QString &adminUser, const QString &adminPassword, const QString &backend, const QVariantMap &setupData, const QString &authenticator, const QVariantMap &authSetupMap);

signals:
    //! Sent when a BufferInfo is updated in storage.
    void bufferInfoUpdated(UserId user, const BufferInfo &info);

    //! Relay from CoreSession::sessionState(). Used for internal connection only
    void sessionState(const Protocol::SessionState &sessionState);

protected:
    virtual void customEvent(QEvent *event);

private slots:
    bool startListening();
    void stopListening(const QString &msg = QString());
    void incomingConnection();
    void clientDisconnected();

    bool initStorage(const QString &backend, const QVariantMap &settings, bool setup = false);
    bool initAuthenticator(const QString &backend, const QVariantMap &settings, bool setup = false);

    void socketError(QAbstractSocket::SocketError err, const QString &errorString);
    void setupClientSession(RemotePeer *, UserId);

    bool changeUserPass(const QString &username);

private:
    Core();
    ~Core();
    void init();
    static Core *instanceptr;

    SessionThread *sessionForUser(UserId userId, bool restoreState = false);
    void addClientHelper(RemotePeer *peer, UserId uid);
    //void processCoreSetup(QTcpSocket *socket, QVariantMap &msg);
    QString setupCoreForInternalUsage();

    bool createUser();

    template<typename Storage>
    void registerStorageBackend();

    template<typename Authenticator>
    void registerAuthenticator();

    void registerStorageBackends();
    void registerAuthenticators();

    DeferredSharedPtr<Storage>       storageBackend(const QString& backendId) const;
    DeferredSharedPtr<Authenticator> authenticator(const QString& authenticatorId) const;

    bool selectBackend(const QString &backend);
    bool selectAuthenticator(const QString &backend);

    bool saveBackendSettings(const QString &backend, const QVariantMap &settings);
    void saveAuthenticatorSettings(const QString &backend, const QVariantMap &settings);

    template<typename Backend>
    QVariantMap promptForSettings(const Backend *backend);

private:
    QSet<CoreAuthHandler *> _connectingClients;
    QHash<UserId, SessionThread *> _sessions;
    DeferredSharedPtr<Storage>       _storage;        ///< Active storage backend
    DeferredSharedPtr<Authenticator> _authenticator;  ///< Active authenticator
    QTimer _storageSyncTimer;
    QMap<UserId, QString> _authUserNames;

#ifdef HAVE_SSL
    SslServer _server, _v6server;
#else
    QTcpServer _server, _v6server;
#endif

    OidentdConfigGenerator *_oidentdConfigGenerator {nullptr};

    std::vector<DeferredSharedPtr<Storage>>       _registeredStorageBackends;
    std::vector<DeferredSharedPtr<Authenticator>> _registeredAuthenticators;

    QDateTime _startTime;

    bool _configured;

    static std::unique_ptr<AbstractSqlMigrationReader> getMigrationReader(Storage *storage);
    static std::unique_ptr<AbstractSqlMigrationWriter> getMigrationWriter(Storage *storage);
    static void stdInEcho(bool on);
    static inline void enableStdInEcho() { stdInEcho(true); }
    static inline void disableStdInEcho() { stdInEcho(false); }
};

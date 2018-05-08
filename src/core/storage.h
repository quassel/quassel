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

#ifndef STORAGE_H
#define STORAGE_H

#include <QtCore>

#include "types.h"
#include "coreidentity.h"
#include "message.h"
#include "network.h"

class Storage : public QObject
{
    Q_OBJECT

public:
    Storage(QObject *parent = 0);
    virtual ~Storage() {};

    enum State {
        IsReady,    // ready to go
        NeedsSetup, // need basic setup (ask the user for input)
        NotAvailable // remove the storage backend from the list of avaliable backends
    };

    enum HashVersion {
        Sha1,
        Sha2_512,
        Latest=Sha2_512

    };

public slots:
    /* General */

    //! Check if the storage type is available.
    /** A storage subclass should return true if it can be successfully used, i.e. if all
     *  prerequisites are in place (e.g. we have an appropriate DB driver etc.).
     * \return True if and only if the storage class can be successfully used.
     */
    virtual bool isAvailable() const = 0;

    //! Returns the identifier of the authenticator backend
    /** \return A string that can be used by the client to identify the authenticator backend */
    virtual QString backendId() const = 0;

    //! Returns the display name of the storage backend
    /** \return A string that can be used by the client to name the storage backend */
    virtual QString displayName() const = 0;

    //! Returns a description of this storage backend
    /** \return A string that can be displayed by the client to describe the storage backend */
    virtual QString description() const = 0;

    //! Returns data required to configure the authenticator backend
    /**
     * A list of flattened triples for each field: {key, translated field name, default value}
     * The default value's type determines the kind of input widget to be shown
     * (int -> QSpinBox; QString -> QLineEdit)
     * \return A list of triples defining the data to be shown in the configuration dialog
     */
    virtual QVariantList setupData() const = 0;

    //! Setup the storage provider.
    /** This prepares the storage provider (e.g. create tables, etc.) for use within Quassel.
     *  \param settings   Hostname, port, username, password, ...
     *  \return True if and only if the storage provider was initialized successfully.
     */
    virtual bool setup(const QVariantMap &settings = QVariantMap()) = 0;

    //! Initialize the storage provider
    /** \param settings   Hostname, port, username, password, ...
     *  \return the State the storage backend is now in (see Storage::State)
     */
    virtual State init(const QVariantMap &settings = QVariantMap()) = 0;

    //! Makes temp data persistent
    /** This Method is periodically called by the Quassel Core to make temporary
     *  data persistant. This reduces the data loss drastically in the
     *  unlikely case of a Core crash.
     */
    virtual void sync() = 0;

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */

    //! Add a new core user to the storage.
    /** \param user     The username of the new user
     *  \param password The cleartext password for the new user
     *  \return The new user's UserId
     */
    virtual UserId addUser(const QString &user, const QString &password, const QString &authenticator = "Database") = 0;

    //! Update a core user's password.
    /** \param user     The user's id
     *  \param password The user's new password
     *  \return true on success.
     */
    virtual bool updateUser(UserId user, const QString &password) = 0;

    //! Rename a user
    /** \param user     The user's id
     *  \param newName  The user's new name
     */
    virtual void renameUser(UserId user, const QString &newName) = 0;

    //! Validate a username with a given password.
    /** \param user     The username to validate
     *  \param password The user's alleged password
     *  \return A valid UserId if the password matches the username; 0 else
     */
    virtual UserId validateUser(const QString &user, const QString &password) = 0;

    //! Check if a user with given username exists. Do not use for login purposes!
    /** \param username  The username to validate
     *  \return A valid UserId if the user exists; 0 else
     */
    virtual UserId getUserId(const QString &username) = 0;

    //! Get the authentication provider for a given user.
    /** \param username  The username to validate
     *  \return The name of the auth provider if the UserId exists, "" otherwise.
     */
    virtual QString getUserAuthenticator(const UserId userid) = 0;


    //! Determine the UserId of the internal user
    /** \return A valid UserId if the password matches the username; 0 else
     */
    virtual UserId internalUser() = 0;

    //! Remove a core user from storage.
    /** \param user     The userid to delete
     */
    virtual void delUser(UserId user) = 0;

    //! Store a user setting persistently
    /**
     * \param userId       The users Id
     * \param settingName  The Name of the Setting
     * \param data         The Value
     */
    virtual void setUserSetting(UserId userId, const QString &settingName, const QVariant &data) = 0;

    //! Retrieve a persistent user setting
    /**
     * \param userId       The users Id
     * \param settingName  The Name of the Setting
     * \param default      Value to return in case it's unset.
     * \return the Value of the Setting or the default value if it is unset.
     */
    virtual QVariant getUserSetting(UserId userId, const QString &settingName, const QVariant &data = QVariant()) = 0;

    /* Identity handling */
    virtual IdentityId createIdentity(UserId user, CoreIdentity &identity) = 0;
    virtual bool updateIdentity(UserId user, const CoreIdentity &identity) = 0;
    virtual void removeIdentity(UserId user, IdentityId identityId) = 0;
    virtual QList<CoreIdentity> identities(UserId user) = 0;

    /* Network handling */

    //! Create a new Network in the storage backend and return it unique Id
    /** \param user        The core user who owns this network
     *  \param networkInfo The networkInfo holding the network definition
     *  \return the NetworkId of the newly created Network. Possibly invalid.
     */
    virtual NetworkId createNetwork(UserId user, const NetworkInfo &info) = 0;

    //! Apply the changes to NetworkInfo info to the storage engine
    /**
     *  \param user        The core user
     *  \param networkInfo The Updated NetworkInfo
     *  \return true if successfull.
     */
    virtual bool updateNetwork(UserId user, const NetworkInfo &info) = 0;

    //! Permanently remove a Network and all the data associated with it.
    /** \note This method is thredsafe.
     *
     *  \param user        The core user
     *  \param networkId   The network to delete
     *  \return true if successfull.
     */
    virtual bool removeNetwork(UserId user, const NetworkId &networkId) = 0;

    //! Returns a list of all NetworkInfos for the given UserId user
    /** \note This method is thredsafe.
     *
     *  \param user        The core user
     *  \return QList<NetworkInfo>.
     */
    virtual QList<NetworkInfo> networks(UserId user) = 0;

    //! Get a list of Networks to restore
    /** Return a list of networks the user was connected at the time of core shutdown
     *  \note This method is threadsafe.
     *
     *  \param user  The User Id in question
     */
    virtual QList<NetworkId> connectedNetworks(UserId user) = 0;

    //! Update the connected state of a network
    /** \note This method is threadsafe
     *
     *  \param user        The Id of the networks owner
     *  \param networkId   The Id of the network
     *  \param isConnected whether the network is connected or not
     */
    virtual void setNetworkConnected(UserId user, const NetworkId &networkId, bool isConnected) = 0;

    //! Get a hash of channels with their channel keys for a given network
    /** The keys are channel names and values are passwords (possibly empty)
     *  \note This method is threadsafe
     *
     *  \param user       The id of the networks owner
     *  \param networkId  The Id of the network
     */
    virtual QHash<QString, QString> persistentChannels(UserId user, const NetworkId &networkId) = 0;

    //! Update the connected state of a channel
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param channel    The name of the channel
     *  \param isJoined   whether the channel is connected or not
     */
    virtual void setChannelPersistent(UserId user, const NetworkId &networkId, const QString &channel, bool isJoined) = 0;

    //! Update the key of a channel
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param channel    The name of the channel
     *  \param key        The key of the channel (possibly empty)
     */
    virtual void setPersistentChannelKey(UserId user, const NetworkId &networkId, const QString &channel, const QString &key) = 0;

    //! retrieve last known away message for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     */
    virtual QString awayMessage(UserId user, NetworkId networkId) = 0;

    //! Make away message persistent for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param awayMsg    The current away message of own user
     */
    virtual void setAwayMessage(UserId user, NetworkId networkId, const QString &awayMsg) = 0;

    //! retrieve last known user mode for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     */
    virtual QString userModes(UserId user, NetworkId networkId) = 0;

    //! Make our user modes persistent for session restore
    /** \note This method is threadsafe
     *
     *  \param user       The Id of the networks owner
     *  \param networkId  The Id of the network
     *  \param userModes  The current user modes of own user
     */
    virtual void setUserModes(UserId user, NetworkId networkId, const QString &userModes) = 0;

    /* Buffer handling */

    //! Get the unique BufferInfo for the given combination of network and buffername for a user.
    /** \param user      The core user who owns this buffername
     *  \param networkId The network id
     *  \param type      The type of the buffer (StatusBuffer, Channel, etc.)
     *  \param buffer  The buffer name (if empty, the net's status buffer is returned)
     *  \param create    Whether or not the buffer should be created if it doesnt exist
     *  \return The BufferInfo corresponding to the given network and buffer name, or an invalid BufferInfo if not found
     */
    virtual BufferInfo bufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "", bool create = true) = 0;

    //! Get the unique BufferInfo for a bufferId
    /** \param user      The core user who owns this buffername
     *  \param bufferId  The id of the buffer
     *  \return The BufferInfo corresponding to the given buffer id, or an invalid BufferInfo if not found.
     */
    virtual BufferInfo getBufferInfo(UserId user, const BufferId &bufferId) = 0;

    //! Request a list of all buffers known to a user.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  \param user  The user whose buffers we request
     *  \return A list of the BufferInfos for all buffers as requested
     */
    virtual QList<BufferInfo> requestBuffers(UserId user) = 0;

    //! Request a list of BufferIds for a given NetworkId
    /** \note This method is threadsafe.
     *
     *  \param user  The user whose buffers we request
     *  \param networkId  The NetworkId of the network in question
     *  \return List of BufferIds belonging to the Network
     */
    virtual QList<BufferId> requestBufferIdsForNetwork(UserId user, NetworkId networkId) = 0;

    //! Remove permanently a buffer and it's content from the storage backend
    /** This call cannot be reverted!
     *  \param user      The user who is the owner of the buffer
     *  \param bufferId  The bufferId
     *  \return true if successfull
     */
    virtual bool removeBuffer(const UserId &user, const BufferId &bufferId) = 0;

    //! Rename a Buffer
    /** \note This method is threadsafe.
     *  \param user      The id of the buffer owner
     *  \param bufferId  The bufferId
     *  \param newName   The new name of the buffer
     *  \return true if successfull
     */
    virtual bool renameBuffer(const UserId &user, const BufferId &bufferId, const QString &newName) = 0;

    //! Merge the content of two Buffers permanently. This cannot be reversed!
    /** \note This method is threadsafe.
     *  \param user      The id of the buffer owner
     *  \param bufferId1 The bufferId of the remaining buffer
     *  \param bufferId2 The buffer that is about to be removed
     *  \return true if successfull
     */
    virtual bool mergeBuffersPermanently(const UserId &user, const BufferId &bufferId1, const BufferId &bufferId2) = 0;

    //! Update the LastSeenDate for a Buffer
    /** This Method is used to make the LastSeenDate of a Buffer persistent
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id of the message that has been just seen
     */
    virtual void setBufferLastSeenMsg(UserId user, const BufferId &bufferId, const MsgId &msgId) = 0;

    //! Get a Hash of all last seen message ids
    /** This Method is called when the Quassel Core is started to restore the lastSeenMsgIds
     * \param user      The Owner of the buffers
     */
    virtual QHash<BufferId, MsgId> bufferLastSeenMsgIds(UserId user) = 0;

    //! Update the MarkerLineMsgId for a Buffer
    /** This Method is used to make the marker line position of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id where the marker line should be placed
     */
    virtual void setBufferMarkerLineMsg(UserId user, const BufferId &bufferId, const MsgId &msgId) = 0;

    //! Get a Hash of all marker line message ids
    /** This Method is called when the Quassel Core is started to restore the MarkerLineMsgIds
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    virtual QHash<BufferId, MsgId> bufferMarkerLineMsgIds(UserId user) = 0;

    //! Update the BufferActivity for a Buffer
    /** This Method is used to make the activity state of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id where the marker line should be placed
     */
    virtual void setBufferActivity(UserId id, BufferId bufferId, Message::Types type) = 0;

    //! Get a Hash of all buffer activity states
    /** This Method is called when the Quassel Core is started to restore the BufferActivities
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    virtual QHash<BufferId, Message::Types> bufferActivities(UserId id) = 0;

    //! Get the bitset of buffer activity states for a buffer
    /** This method is used to load the activity state of a buffer when its last seen message changes.
     *  \note This method is threadsafe.
     *
     * \param bufferId The buffer
     * \param lastSeenMsgId     The last seen message
     */
    virtual Message::Types bufferActivity(BufferId bufferId, MsgId lastSeenMsgId) = 0;

    //! Get a hash of buffers with their ciphers for a given network
    /** The keys are channel names and values are ciphers (possibly empty)
     *  \note This method is threadsafe
     *
     *  \param user       The id of the networks owner
     *  \param networkId  The Id of the network
     */
    virtual QHash<QString, QByteArray> bufferCiphers(UserId user, const NetworkId &networkId) = 0;

    //! Update the cipher of a buffer
    /** \note This method is threadsafe
     *
     *  \param user        The Id of the networks owner
     *  \param networkId   The Id of the network
     *  \param bufferName The Cname of the buffer
     *  \param cipher      The cipher for the buffer
     */
    virtual void setBufferCipher(UserId user, const NetworkId &networkId, const QString &bufferName, const QByteArray &cipher) = 0;

    //! Update the highlight count for a Buffer
    /** This Method is used to make the activity state of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param MsgId     The Message id where the marker line should be placed
     */
    virtual void setHighlightCount(UserId id, BufferId bufferId, int count) = 0;

    //! Get a Hash of all highlight count states
    /** This Method is called when the Quassel Core is started to restore the HighlightCounts
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    virtual QHash<BufferId, int> highlightCounts(UserId id) = 0;

    //! Get the highlight count states for a buffer
    /** This method is used to load the activity state of a buffer when its last seen message changes.
     *  \note This method is threadsafe.
     *
     * \param bufferId The buffer
     * \param lastSeenMsgId     The last seen message
     */
    virtual int highlightCount(BufferId bufferId, MsgId lastSeenMsgId) = 0;

    /* Message handling */

    //! Store a Message in the storage backend and set its unique Id.
    /** \param msg  The message object to be stored
     *  \return true on success
     */
    virtual bool logMessage(Message &msg) = 0;

    //! Store a list of Messages in the storage backend and set their unique Id.
    /** \param msgs The list message objects to be stored
     *  \return true on success
     */
    virtual bool logMessages(MessageList &msgs) = 0;

    //! Request a certain number messages stored in a given buffer.
    /** \param buffer   The buffer we request messages from
     *  \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    if != -1 limit the returned list to a max of \limit entries
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgs(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1) = 0;

    //! Request a certain number messages stored in a given buffer, matching certain filters
    /** \param buffer   The buffer we request messages from
     *  \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    if != -1 limit the returned list to a max of \limit entries
     *  \param type     The Message::Types that should be returned
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgsFiltered(UserId user, BufferId bufferId, MsgId first = -1, MsgId last = -1,
                                               int limit = -1, Message::Types type = Message::Types{-1},
                                               Message::Flags flags = Message::Flags{-1}) = 0;

    //! Request a certain number of messages across all buffers
    /** \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    Max amount of messages
     *  \return The requested list of messages
     */
    virtual QList<Message> requestAllMsgs(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1) = 0;

    //! Request a certain number of messages across all buffers, matching certain filters
    /** \param first    if != -1 return only messages with a MsgId >= first
     *  \param last     if != -1 return only messages with a MsgId < last
     *  \param limit    Max amount of messages
     *  \param type     The Message::Types that should be returned
     *  \return The requested list of messages
     */
    virtual QList<Message> requestAllMsgsFiltered(UserId user, MsgId first = -1, MsgId last = -1, int limit = -1,
                                                  Message::Types type = Message::Types{-1},
                                                  Message::Flags flags = Message::Flags{-1}) = 0;

    //! Fetch all authusernames
    /** \return      Map of all current UserIds to permitted idents
     */
    virtual QMap<UserId, QString> getAllAuthUserNames() = 0;

    //! Get the auth username associated with a userId
    /** \param user  The user to retrieve the username for
     *  \return      The username for the user
     */
    virtual QString getAuthUserName(UserId user) = 0;

signals:
    //! Sent when a new BufferInfo is created, or an existing one changed somehow.
    void bufferInfoUpdated(UserId user, const BufferInfo &);
    //! Sent when a Buffer was renamed
    void bufferRenamed(const QString &newName, const QString &oldName);
    //! Sent when a new user has been added
    void userAdded(UserId, const QString &username);
    //! Sent when a user has been renamed
    void userRenamed(UserId, const QString &newname);
    //! Sent when a user has been removed
    void userRemoved(UserId);

protected:
    QString hashPassword(const QString &password);
    bool checkHashedPassword(const UserId user, const QString &password, const QString &hashedPassword, const Storage::HashVersion version);

private:
    QString hashPasswordSha1(const QString &password);
    bool checkHashedPasswordSha1(const QString &password, const QString &hashedPassword);

    QString hashPasswordSha2_512(const QString &password);
    bool checkHashedPasswordSha2_512(const QString &password, const QString &hashedPassword);
    QString sha2_512(const QString &input);
};


#endif

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

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <QtCore>

#include "types.h"
#include "message.h"
#include "network.h"

class Storage : public QObject {
  Q_OBJECT

  public:
    Storage(QObject *parent = 0);
    virtual ~Storage() {};

  public slots:
    /* General */

    //! Check if the storage type is available.
    /** A storage subclass should return true if it can be successfully used, i.e. if all
     *  prerequisites are in place (e.g. we have an appropriate DB driver etc.).
     * \return True if and only if the storage class can be successfully used.
     */
    virtual bool isAvailable() const = 0;

    //! Returns the display name of the storage backend
    /** \return A string that can be used by the client to name the storage backend */
    virtual QString displayName() const = 0;

    //! Returns a description of this storage backend
    /** \return A string that can be displayed by the client to describe the storage backend */
    virtual QString description() const = 0;

    //! Setup the storage provider.
    /** This prepares the storage provider (e.g. create tables, etc.) for use within Quassel.
     *  \param settings   Hostname, port, username, password, ...
     *  \return True if and only if the storage provider was initialized successfully.
     */
    virtual bool setup(const QVariantMap &settings = QVariantMap()) = 0;

    //! Initialize the storage provider
    /** \param settings   Hostname, port, username, password, ...  
     *  \return True if and only if the storage provider was initialized successfully.
     */
    virtual bool init(const QVariantMap &settings = QVariantMap()) = 0;

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
    virtual UserId addUser(const QString &user, const QString &password) = 0;

    //! Update a core user's password.
    /** \param user     The user's id
     *  \param password The user's new password
     */
    virtual void updateUser(UserId user, const QString &password) = 0;

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

    //! Remove a core user from storage.
    /** \param user     The userid to delete
     */
    virtual void delUser(UserId user) = 0;

    /* Network handling */

    //! Create a new Network in the storage backend and return it unique Id
    /** \param user        The core user who owns this network
     *  \param networkInfo The networkInfo holding the network definition
     *  \return the NetworkId of the newly created Network. Possibly invalid.
     */
    virtual NetworkId createNetwork(UserId user, const NetworkInfo &info) = 0;

    //! Apply the changes to NetworkInfo info to the storage engine
    /** \note This method is thredsafe.
     *
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
  
    //! Get the unique NetworkId of the network for a user.
    /** \param user    The core user who owns this network
     *  \param network The network name
     *  \return The NetworkId corresponding to the given network, or 0 if not found
     */
    virtual NetworkId getNetworkId(UserId user, const QString &network) = 0;

    /* Buffer handling */

    //! Get the unique BufferInfo for the given combination of network and buffername for a user.
    /** \param user      The core user who owns this buffername
     *  \param networkId The network id
     *  \param type      The type of the buffer (StatusBuffer, Channel, etc.)
     *  \param buffer  The buffer name (if empty, the net's status buffer is returned)
     *  \return The BufferInfo corresponding to the given network and buffer name, or 0 if not found
     */
    virtual BufferInfo getBufferInfo(UserId user, const NetworkId &networkId, BufferInfo::Type type, const QString &buffer = "") = 0;

    //! Request a list of all buffers known to a user since a certain point in time.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  Optionally, a QDateTime can be given, so that only buffers are listed that where active
     *  since that point in time.
     *  \param user  The user whose buffers we request
     *  \param since If this is defined, older buffers will be ignored
     *  \return A list of the BufferInfos for all buffers as requested
     */
    virtual QList<BufferInfo> requestBuffers(UserId user, QDateTime since = QDateTime()) = 0;

    //! Update the LastSeenDate for a Buffer
    /** This Method is used to make the LastSeenDate of a Buffer persistent
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param seenDate  Time the Buffer has been visited the last time
     */
    virtual void setBufferLastSeen(UserId user, const BufferId &bufferId, const QDateTime &seenDate) = 0;

    //! Get a Hash of all last seen dates. 
    /** This Method is called when the Quassel Core is started to restore the lastSeenDates
     * \param user      The Owner of the buffers
     */
    virtual QHash<BufferId, QDateTime> bufferLastSeenDates(UserId user) = 0;

  
    /* Message handling */

    //! Store a Message in the backlog.
    /** \param msg  The message object to be stored
     *  \return The globally unique id for the stored message
     */
    virtual MsgId logMessage(Message msg) = 0;

    //! Request a certain number (or all) messages stored in a given buffer.
    /** \param buffer   The buffer we request messages from
     *  \param lastmsgs The number of messages we would like to receive, or -1 if we'd like all messages from that buffername
     *  \param offset   Do not return (but DO count) messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgs(BufferInfo buffer, int lastmsgs = -1, int offset = -1) = 0;

    //! Request messages stored in a given buffer since a certain point in time.
    /** \param buffer   The buffer we request messages from
     *  \param since    Only return messages newer than this point in time
     *  \param offset   Do not return messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgs(BufferInfo buffer, QDateTime since, int offset = -1) = 0;

    //! Request a range of messages stored in a given buffer.
    /** \param buffer   The buffer we request messages from
     *  \param first    Return messages with first <= MsgId <= last
     *  \param last     Return messages with first <= MsgId <= last
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgRange(BufferInfo buffer, int first, int last) = 0;

  signals:
    //! Sent when a new BufferInfo is created, or an existing one changed somehow.
    void bufferInfoUpdated(UserId user, const BufferInfo &);
    //! Sent when a new user has been added
    void userAdded(UserId, const QString &username);
    //! Sent when a user has been renamed
    void userRenamed(UserId, const QString &newname);
    //! Sent when a user has been removed
    void userRemoved(UserId);

  public:

};


#endif

/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#include <QSqlDatabase>

#include "message.h"

class Storage : public QObject {
  Q_OBJECT

  public:
    Storage() {};
    virtual ~Storage() {};

    /* General */

    //! Check if the storage type is available.
    /** A storage subclass should return true if it can be successfully used, i.e. if all
     *  prerequisites are in place (e.g. we have an appropriate DB driver etc.).
     * \return True if and only if the storage class can be successfully used.
     */
    static bool isAvailable() { return false; }

    //! Returns the display name of the storage backend
    /** \return A string that can be used by the GUI to describe the storage backend */
    static QString displayName() { return ""; }

    //! Setup the storage provider.
    /** This prepares the storage provider (e.g. create tables, etc.) for use within Quassel.
     *  \param settings   Hostname, port, username, password, ...
     *  \return True if and only if the storage provider was initialized successfully.
     */
    virtual bool setup(const QVariantMap &settings = QVariantMap()) { return false; }
    
    //! Initialize the storage provider
    /** \param settings   Hostname, port, username, password, ...  
     *  \return True if and only if the storage provider was initialized successfully.
     */
    virtual bool init(const QVariantMap &settings = QVariantMap()) = 0;
    
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

    //! Get the unique NetworkId of the network for a user.
    /** \param user    The core user who owns this buffername
     *  \param network The network name
     *  \return The BufferInfo corresponding to the given network and buffer name, or 0 if not found
     */
    virtual uint getNetworkId(UserId user, const QString &network) = 0;

    /* Buffer handling */

    //! Get the unique BufferInfo for the given combination of network and buffername for a user.
    /** \param user    The core user who owns this buffername
     *  \param network The network name
     *  \param buffer  The buffer name (if empty, the net's status buffer is returned)
     *  \return The BufferInfo corresponding to the given network and buffer name, or 0 if not found
     */
    virtual BufferInfo getBufferInfo(UserId user, const QString &network, const QString &buffer = "") = 0;

    //! Request a list of all buffers known to a user since a certain point in time.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  Optionally, a QDateTime can be given, so that only buffers are listed that where active
     *  since that point in time.
     *  \param user  The user whose buffers we request
     *  \param since If this is defined, older buffers will be ignored
     *  \return A list of the BufferInfos for all buffers as requested
     */
    virtual QList<BufferInfo> requestBuffers(UserId user, QDateTime since = QDateTime()) = 0;

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
    void bufferInfoUpdated(BufferInfo);
    //! Sent when a new user has been added
    void userAdded(UserId, const QString &username);
    //! Sent when a user has been renamed
    void userRenamed(UserId, const QString &newname);
    //! Sent when a user has been removed
    void userRemoved(UserId);

  public:
    /* Exceptions */
    struct AuthError : public Exception {};
};


#endif

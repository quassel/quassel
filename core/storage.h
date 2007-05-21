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

#include "global.h"
#include "message.h"

class Storage : public QObject {
  Q_OBJECT

  public:
    Storage() {};
    virtual ~Storage() {};

    //! Initialize the static parts of the storage class
    /** This is called by the core before any other method of the storage backend is used.
     *  This should be used to perform any static initialization that might be necessary.
     *  DO NOT use this for creating database connection or similar stuff, since init() might be
     *  called even if the storage backend is never be actually used (because no user selected it).
     *  For anything like this, the constructor (which is called if and when we actually create an instance
     *  of the storage backend) is the right place.
     */
    static void init() {};

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

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */

    //! Add a new core user to the storage.
    /** \param user     The username of the new user
     *  \param password The cleartext password for the new user
     *  \return The new user's UserId
     */
    virtual UserId addUser(QString user, QString password) = 0;

    //! Update a core user's password.
    /** \param user     The user's name
     *  \param password The user's new password
     */
    virtual void updateUser(QString user, QString password) = 0;

    //! Validate a username with a given password.
    /** \param user     The username to validate
     *  \param password The user's alleged password
     *  \return A valid UserId if the password matches the username; 0 else
     */
    virtual UserId validateUser(QString user, QString password) = 0;

    //! Remove a core user from storage.
    /** \param user     The username to delete
     */
    virtual void delUser(QString user) = 0;

    /* Buffer handling */

    //! Get the unique BufferId for the given combination of network and buffername for a user.
    /** \param user    The core user who owns this buffername
     *  \param network The network name
     *  \param buffer  The buffer name (if empty, the net's status buffer is returned)
     *  \return The BufferId corresponding to the given network and buffer name, or 0 if not found
     */
    virtual BufferId getBufferId(UserId user, QString network, QString buffer = "") = 0;

    //! Request a list of all buffers known to a user since a certain point in time.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  Optionally, a QDateTime can be given, so that only buffers are listed that where active
     *  since that point in time.
     *  \param user  The user whose buffers we request
     *  \param since If this is defined, older buffers will be ignored
     *  \return A list of the BufferIds for all buffers as requested
     */
    virtual QList<BufferId> requestBuffers(UserId user, QDateTime since = QDateTime()) = 0;

    /* Message handling */

    //! Store a Message in the backlog.
    /** \param msg  The message object to be stored
     *  \return The globally uniqe id for the stored message
     */
    virtual MsgId logMessage(Message msg) = 0;

    //! Request a certain number (or all) messages stored in a given buffer.
    /** \param buffer   The buffer we request messages from
     *  \param lastmsgs The number of messages we would like to receive, or -1 if we'd like all messages from that buffername
     *  \param offset   Do not return (but DO count) messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgs(BufferId buffer, int lastmsgs = -1, int offset = -1) = 0;

    //! Request messages stored in a given buffer since a certain point in time.
    /** \param buffer   The buffer we request messages from
     *  \param since    Only return messages newer than this point in time
     *  \param offset   Do not return messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgs(BufferId buffer, QDateTime since, int offset = -1) = 0;

    //! Request a range of messages stored in a given buffer.
    /** \param buffer   The buffer we request messages from
     *  \param first    Return messages with first <= MsgId <= last
     *  \param last     Return messages with first <= MsgId <= last
     *  \return The requested list of messages
     */
    virtual QList<Message> requestMsgRange(BufferId buffer, int first, int last) = 0;

  public slots:
    //! This is just for importing the old file-based backlog */
    /** This slot needs to be implemented in the storage backends.
     *  It should first prepare (delete?) the database, then call initBackLogOld(UserId id).
     *  If the importing was successful, backLogEnabledOld will be true afterwards.
     */
    virtual void importOldBacklog() = 0;

  signals:
    //! Sent if a new BufferId is created, or an existing one changed somehow.
    void bufferIdUpdated(BufferId);

  protected:
    // Old stuff, just for importing old file-based data
    void initBackLogOld(UserId id);

    bool backLogEnabledOld;
    QDir backLogDir;
    QHash<QString, QList<Message> > backLog;
    QHash<QString, QFile *> logFiles;
    QHash<QString, QDataStream *> logStreams;
    QHash<QString, QDate> logFileDates;
    QHash<QString, QDir> logFileDirs;

};


#endif

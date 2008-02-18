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
#include <QTcpServer>
#include <QTcpSocket>

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
    static QList<Message> requestMsgs(BufferInfo buffer, int lastmsgs = -1, int offset = -1);

    //! Request messages stored in a given buffer since a certain point in time.
    /** \note This method is threadsafe.
     *
     *  \param buffer   The buffer we request messages from
     *  \param since    Only return messages newer than this point in time
     *  \param offset   Do not return messages with MsgId >= offset, if offset >= 0
     *  \return The requested list of messages
     */
    static QList<Message> requestMsgs(BufferInfo buffer, QDateTime since, int offset = -1);

    //! Request a range of messages stored in a given buffer.
    /** \note This method is threadsafe.
     *
     *  \param buffer   The buffer we request messages from
     *  \param first    Return messages with first <= MsgId <= last
     *  \param last     Return messages with first <= MsgId <= last
     *  \return The requested list of messages
     */
    static QList<Message> requestMsgRange(BufferInfo buffer, int first, int last);

    //! Request a list of all buffers known to a user since a certain point in time.
    /** This method is used to get a list of all buffers we have stored a backlog from.
     *  Optionally, a QDateTime can be given, so that only buffers are listed that were active
     *  since that point in time.
     *  \note This method is threadsafe.
     *
     *  \param user  The user whose buffers we request
     *  \param since If this is defined, older buffers will be ignored
     *  \return A list of the BufferInfos for all buffers as requested
     */
    static QList<BufferInfo> requestBuffers(UserId user, QDateTime since = QDateTime());

    //! Remove permanently a buffer and it's content from the storage backend
    /** This call cannot be reverted!
     *  \note This method is threadsafe.
     *
     *  \param user      The user who is the owner of the buffer
     *  \param bufferId  The bufferId
     *  \return true if successfull
     */
    static bool removeBuffer(const UserId &user, const BufferId &bufferId);

    //! Update the LastSeenDate for a Buffer
    /** This Method is used to make the LastSeenDate of a Buffer persistent
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of that Buffer
     * \param bufferId  The buffer id
     * \param seenDate  Time the Buffer has been visited the last time
     */
    static void setBufferLastSeen(UserId user, const BufferId &bufferId, const QDateTime &seenDate);

    //! Get a Hash of all last seen dates. 
    /** This Method is called when the Quassel Core is started to restore the lastSeenDates
     *  \note This method is threadsafe.
     *
     * \param user      The Owner of the buffers
     */
    static QHash<BufferId, QDateTime> bufferLastSeenDates(UserId user);

  public slots:
    //! Make storage data persistent
    /** \note This method is threadsafe.
     */
    void syncStorage();
  
  signals:
    //! Sent when a BufferInfo is updated in storage.
    void bufferInfoUpdated(UserId user, const BufferInfo &info);

  private slots:
    bool startListening(uint port = Global::defaultPort);
    void stopListening();
    void incomingConnection();
    void clientHasData();
    void clientDisconnected();

    bool initStorage(QVariantMap dbSettings, bool setup = false);

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

    QTcpServer server; // TODO: implement SSL
    QHash<QTcpSocket *, quint32> blocksizes;
    QHash<QTcpSocket *, QVariantMap> clientInfo;

    QHash<QString, Storage *> _storageBackends;

    QDateTime startTime;

    bool configured;

    static QMutex mutex;
};

#endif

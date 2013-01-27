/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <QList>
#include <QPointer>

#include "bufferinfo.h"
#include "coreconnection.h"
#include "quassel.h"
#include "types.h"

class Message;
class MessageModel;
class AbstractMessageProcessor;

class Identity;
class CertIdentity;
class Network;

class AbstractUi;
class AbstractUiMsg;
class NetworkModel;
class BufferModel;
class BufferSyncer;
class BufferViewOverlay;
class ClientAliasManager;
class ClientBacklogManager;
class ClientBufferViewManager;
class ClientIgnoreListManager;
class ClientIrcListHelper;
class ClientUserInputHandler;
class CoreAccountModel;
class CoreConnection;
class IrcUser;
class IrcChannel;
class NetworkConfig;
class SignalProxy;
struct NetworkInfo;

class Client : public QObject
{
    Q_OBJECT

public:
    enum ClientMode {
        LocalCore,
        RemoteCore
    };

    static bool instanceExists();
    static Client *instance();
    static void destroy();
    static void init(AbstractUi *);
    static AbstractUi *mainUi();

    static QList<NetworkId> networkIds();
    static const Network *network(NetworkId);

    static QList<IdentityId> identityIds();
    static const Identity *identity(IdentityId);

    //! Request creation of an identity with the given data.
    /** The request will be sent to the core, and will be propagated back to all the clients
     *  with a new valid IdentityId.
     *  \param identity The identity template for the new identity. It does not need to have a valid ID.
     */
    static void createIdentity(const CertIdentity &identity);

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

    static void createNetwork(const NetworkInfo &info, const QStringList &persistentChannels = QStringList());
    static void updateNetwork(const NetworkInfo &info);
    static void removeNetwork(NetworkId id);

    static inline NetworkModel *networkModel() { return instance()->_networkModel; }
    static inline BufferModel *bufferModel() { return instance()->_bufferModel; }
    static inline MessageModel *messageModel() { return instance()->_messageModel; }
    static inline AbstractMessageProcessor *messageProcessor() { return instance()->_messageProcessor; }
    static inline SignalProxy *signalProxy() { return instance()->_signalProxy; }

    static inline ClientAliasManager *aliasManager() { return instance()->_aliasManager; }
    static inline ClientBacklogManager *backlogManager() { return instance()->_backlogManager; }
    static inline ClientIrcListHelper *ircListHelper() { return instance()->_ircListHelper; }
    static inline ClientBufferViewManager *bufferViewManager() { return instance()->_bufferViewManager; }
    static inline BufferViewOverlay *bufferViewOverlay() { return instance()->_bufferViewOverlay; }
    static inline ClientUserInputHandler *inputHandler() { return instance()->_inputHandler; }
    static inline NetworkConfig *networkConfig() { return instance()->_networkConfig; }
    static inline ClientIgnoreListManager *ignoreListManager() { return instance()->_ignoreListManager; }

    static inline CoreAccountModel *coreAccountModel() { return instance()->_coreAccountModel; }
    static inline CoreConnection *coreConnection() { return instance()->_coreConnection; }
    static inline CoreAccount currentCoreAccount() { return coreConnection()->currentAccount(); }
    static inline Quassel::Features coreFeatures() { return _coreFeatures; }

    static bool isConnected();
    static bool internalCore();

    static void userInput(const BufferInfo &bufferInfo, const QString &message);

    static void setBufferLastSeenMsg(BufferId id, const MsgId &msgId); // this is synced to core and other clients
    static void setMarkerLine(BufferId id, const MsgId &msgId); // this is synced to core and other clients
    static MsgId markerLine(BufferId id);

    static void removeBuffer(BufferId id);
    static void renameBuffer(BufferId bufferId, const QString &newName);
    static void mergeBuffersPermanently(BufferId bufferId1, BufferId bufferId2);
    static void purgeKnownBufferIds();

    static void logMessage(QtMsgType type, const char *msg);
    static inline const QString &debugLog() { return instance()->_debugLogBuffer; }

signals:
    void requestNetworkStates();

    void showConfigWizard(const QVariantMap &coredata);

    void connected();
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
    void requestCreateIdentity(const Identity &, const QVariantMap &);
    //! Sent to the core when an identity shall be removed. Should not be used elsewhere.
    void requestRemoveIdentity(IdentityId);

    void networkCreated(NetworkId id);
    void networkRemoved(NetworkId id);

    void requestCreateNetwork(const NetworkInfo &info, const QStringList &persistentChannels = QStringList());
    void requestRemoveNetwork(NetworkId);

    void logUpdated(const QString &msg);

    //! Emitted when a buffer has been marked as read
    /** This is currently triggered by setting lastSeenMsg, either local or remote,
     *  or by bringing the window to front.
     *  \param id The buffer that has been marked as read
     */
    void bufferMarkedAsRead(BufferId id);

public slots:
    void disconnectFromCore();

    void bufferRemoved(BufferId bufferId);
    void bufferRenamed(BufferId bufferId, const QString &newName);
    void buffersPermanentlyMerged(BufferId bufferId1, BufferId bufferId2);

    void markBufferAsRead(BufferId id);

private slots:
    void setSyncedToCore();
    void setDisconnectedFromCore();
    void connectionStateChanged(CoreConnection::ConnectionState);

    void recvMessage(const Message &message);
    void recvStatusMsg(QString network, QString message);

    void networkDestroyed();
    void coreIdentityCreated(const Identity &);
    void coreIdentityRemoved(IdentityId);
    void coreNetworkCreated(NetworkId);
    void coreNetworkRemoved(NetworkId);

    void requestInitialBacklog();

    void sendBufferedUserInput();

private:
    Client(QObject *parent = 0);
    virtual ~Client();
    void init();

    static void addNetwork(Network *);
    static void setCoreFeatures(Quassel::Features);
    static inline BufferSyncer *bufferSyncer() { return instance()->_bufferSyncer; }

    static QPointer<Client> instanceptr;

    SignalProxy *_signalProxy;
    AbstractUi *_mainUi;
    NetworkModel *_networkModel;
    BufferModel *_bufferModel;
    BufferSyncer *_bufferSyncer;
    ClientAliasManager *_aliasManager;
    ClientBacklogManager *_backlogManager;
    ClientBufferViewManager *_bufferViewManager;
    BufferViewOverlay *_bufferViewOverlay;
    ClientIrcListHelper *_ircListHelper;
    ClientUserInputHandler *_inputHandler;
    NetworkConfig *_networkConfig;
    ClientIgnoreListManager *_ignoreListManager;

    MessageModel *_messageModel;
    AbstractMessageProcessor *_messageProcessor;

    CoreAccountModel *_coreAccountModel;
    CoreConnection *_coreConnection;

    ClientMode clientMode;

    QHash<NetworkId, Network *> _networks;
    QHash<IdentityId, Identity *> _identities;

    bool _connected;
    static Quassel::Features _coreFeatures;

    QString _debugLogBuffer;
    QTextStream _debugLog;

    QList<QPair<BufferInfo, QString> > _userInputBuffer;

    friend class CoreConnection;
};


#endif

/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "client-export.h"

#include <memory>

#include <QList>
#include <QPointer>

#include "bufferinfo.h"
#include "coreaccount.h"
#include "coreconnection.h"
#include "coreinfo.h"
#include "highlightrulemanager.h"
#include "quassel.h"
#include "singleton.h"
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
class ClientTransferManager;
class ClientUserInputHandler;
class CoreAccountModel;
class CoreConnection;
class DccConfig;
class IrcUser;
class IrcChannel;
class NetworkConfig;
class SignalProxy;
class TransferModel;

struct NetworkInfo;

class CLIENT_EXPORT Client : public QObject, public Singleton<Client>
{
    Q_OBJECT

public:
    enum ClientMode
    {
        LocalCore,
        RemoteCore
    };

    Client(std::unique_ptr<AbstractUi>, QObject* parent = nullptr);
    ~Client() override;

    static AbstractUi* mainUi();

    static QList<NetworkId> networkIds();
    static const Network* network(NetworkId);

    static QList<IdentityId> identityIds();
    static const Identity* identity(IdentityId);

    //! Request creation of an identity with the given data.
    /** The request will be sent to the core, and will be propagated back to all the clients
     *  with a new valid IdentityId.
     *  \param identity The identity template for the new identity. It does not need to have a valid ID.
     */
    static void createIdentity(const CertIdentity& identity);

    //! Request update of an identity with the given data.
    /** The request will be sent to the core, and will be propagated back to all the clients.
     *  \param id The identity to be updated.
     *  \param serializedData The identity's content (cf. SyncableObject::toVariantMap())
     */
    static void updateIdentity(IdentityId id, const QVariantMap& serializedData);

    //! Request removal of the identity with the given ID from the core (and all the clients, of course).
    /** \param id The ID of the identity to be removed.
     */
    static void removeIdentity(IdentityId id);

    static void createNetwork(const NetworkInfo& info, const QStringList& persistentChannels = QStringList());
    static void updateNetwork(const NetworkInfo& info);
    static void removeNetwork(NetworkId id);

    static inline NetworkModel* networkModel() { return instance()->_networkModel; }
    static inline BufferModel* bufferModel() { return instance()->_bufferModel; }
    static inline MessageModel* messageModel() { return instance()->_messageModel; }
    static inline AbstractMessageProcessor* messageProcessor() { return instance()->_messageProcessor; }
    static inline SignalProxy* signalProxy() { return instance()->_signalProxy; }

    static inline ClientAliasManager* aliasManager() { return instance()->_aliasManager; }
    static inline ClientBacklogManager* backlogManager() { return instance()->_backlogManager; }
    static inline CoreInfo* coreInfo() { return instance()->_coreInfo; }
    static inline DccConfig* dccConfig() { return instance()->_dccConfig; }
    static inline ClientIrcListHelper* ircListHelper() { return instance()->_ircListHelper; }
    static inline ClientBufferViewManager* bufferViewManager() { return instance()->_bufferViewManager; }
    static inline BufferViewOverlay* bufferViewOverlay() { return instance()->_bufferViewOverlay; }
    static inline ClientUserInputHandler* inputHandler() { return instance()->_inputHandler; }
    static inline NetworkConfig* networkConfig() { return instance()->_networkConfig; }
    static inline ClientIgnoreListManager* ignoreListManager() { return instance()->_ignoreListManager; }
    static inline HighlightRuleManager* highlightRuleManager() { return instance()->_highlightRuleManager; }
    static inline ClientTransferManager* transferManager() { return instance()->_transferManager; }
    static inline TransferModel* transferModel() { return instance()->_transferModel; }

    static inline BufferSyncer* bufferSyncer() { return instance()->_bufferSyncer; }

    static inline CoreAccountModel* coreAccountModel() { return instance()->_coreAccountModel; }
    static inline CoreConnection* coreConnection() { return instance()->_coreConnection; }
    static inline CoreAccount currentCoreAccount() { return coreConnection()->currentAccount(); }
    static bool isCoreFeatureEnabled(Quassel::Feature feature);

    static bool isConnected();
    static bool internalCore();

    static void userInput(const BufferInfo& bufferInfo, const QString& message);

    static void setBufferLastSeenMsg(BufferId id, const MsgId& msgId);  // this is synced to core and other clients
    static void setMarkerLine(BufferId id, const MsgId& msgId);         // this is synced to core and other clients
    static MsgId markerLine(BufferId id);

    static void removeBuffer(BufferId id);
    static void renameBuffer(BufferId bufferId, const QString& newName);
    static void mergeBuffersPermanently(BufferId bufferId1, BufferId bufferId2);
    static void purgeKnownBufferIds();

    /**
     * Requests client to resynchronize the CoreInfo object for legacy (pre-0.13) cores
     *
     * This provides compatibility with updating core information for legacy cores, and can be
     * removed after protocol break.
     *
     * NOTE: On legacy (pre-0.13) cores, any existing connected signals will be destroyed and must
     * be re-added after calling this, in addition to checking for existing data in coreInfo().
     */
    static void refreshLegacyCoreInfo();

    static void changePassword(const QString& oldPassword, const QString& newPassword);
    static void kickClient(int peerId);

    void displayIgnoreList(QString ignoreRule) { emit showIgnoreList(ignoreRule); }

    /**
     * Request to show the channel list dialog for the network, optionally searching by channel name
     *
     * @see Client::showChannelList()
     *
     * @param networkId        Network ID for associated network
     * @param channelFilters   Partial channel name to search for, or empty to show all
     * @param listImmediately  If true, immediately list channels, otherwise just show dialog
     */
    void displayChannelList(NetworkId networkId, const QString& channelFilters = {}, bool listImmediately = false)
    {
        emit showChannelList(networkId, channelFilters, listImmediately);
    }

signals:
    void requestNetworkStates();

    void showConfigWizard(const QVariantMap& coredata);

    /**
     * Request to show the channel list dialog for the network, optionally searching by channel name
     *
     * @see MainWin::showChannelList()
     *
     * @param networkId        Network ID for associated network
     * @param channelFilters   Partial channel name to search for, or empty to show all
     * @param listImmediately  If true, immediately list channels, otherwise just show dialog
     */
    void showChannelList(NetworkId networkId, const QString& channelFilters = {}, bool listImmediately = false);

    void showIgnoreList(QString ignoreRule);

    void connected();
    void disconnected();
    void coreConnectionStateChanged(bool);

    /**
     * Signals that core information has been resynchronized, removing existing signal handlers
     *
     * Whenever this is emitted, one should re-add any handlers for CoreInfo::coreDataChanged() and
     * apply any existing information in the coreInfo() object.
     *
     * Only emitted on legacy (pre-0.13) cores.  Generally, one should use the
     * CoreInfo::coreDataChanged() signal too.
     */
    void coreInfoResynchronized();

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
    void requestCreateIdentity(const Identity&, const QVariantMap&);
    //! Sent to the core when an identity shall be removed. Should not be used elsewhere.
    void requestRemoveIdentity(IdentityId);

    void networkCreated(NetworkId id);
    void networkRemoved(NetworkId id);

    void requestCreateNetwork(const NetworkInfo& info, const QStringList& persistentChannels = QStringList());
    void requestRemoveNetwork(NetworkId);

    //! Emitted when a buffer has been marked as read
    /** This is currently triggered by setting lastSeenMsg, either local or remote,
     *  or by bringing the window to front.
     *  \param id The buffer that has been marked as read
     */
    void bufferMarkedAsRead(BufferId id);

    //! Requests a password change (user name must match the currently logged in user)
    void requestPasswordChange(PeerPtr peer, const QString& userName, const QString& oldPassword, const QString& newPassword);

    void requestKickClient(int peerId);
    void passwordChanged(bool success);

    //! Emitted when database schema upgrade starts or ends (only mono client)
    void dbUpgradeInProgress(bool inProgress);

    //! Emitted before an exit request is handled
    void exitRequested(const QString& reason);

public slots:
    void disconnectFromCore();

    void bufferRemoved(BufferId bufferId);
    void bufferRenamed(BufferId bufferId, const QString& newName);
    void buffersPermanentlyMerged(BufferId bufferId1, BufferId bufferId2);

    void markBufferAsRead(BufferId id);

    void onDbUpgradeInProgress(bool inProgress);
    void onExitRequested(int exitCode, const QString& reason);

private slots:
    void setSyncedToCore();
    void setDisconnectedFromCore();
    void connectionStateChanged(CoreConnection::ConnectionState);

    void recvMessage(const Message& message);
    void recvStatusMsg(QString network, QString message);

    void networkDestroyed();
    void coreIdentityCreated(const Identity&);
    void coreIdentityRemoved(IdentityId);
    void coreNetworkCreated(NetworkId);
    void coreNetworkRemoved(NetworkId);

    void corePasswordChanged(PeerPtr, bool success);

    void finishConnectionInitialization();

    void sendBufferedUserInput();

private:
    void requestInitialBacklog();

    /**
     * Deletes and resynchronizes the CoreInfo object for legacy (pre-0.13) cores
     *
     * This provides compatibility with updating core information for legacy cores, and can be
     * removed after protocol break.
     *
     * NOTE: On legacy (pre-0.13) cores, any existing connected signals will be destroyed and must
     * be re-added after calling this, in addition to checking for existing data in coreInfo().
     */
    void requestLegacyCoreInfo();

    static void addNetwork(Network*);

    SignalProxy* _signalProxy{nullptr};
    std::unique_ptr<AbstractUi> _mainUi;
    NetworkModel* _networkModel{nullptr};
    BufferModel* _bufferModel{nullptr};
    BufferSyncer* _bufferSyncer{nullptr};
    ClientAliasManager* _aliasManager{nullptr};
    ClientBacklogManager* _backlogManager{nullptr};
    ClientBufferViewManager* _bufferViewManager{nullptr};
    BufferViewOverlay* _bufferViewOverlay{nullptr};
    CoreInfo* _coreInfo{nullptr};
    DccConfig* _dccConfig{nullptr};
    ClientIrcListHelper* _ircListHelper{nullptr};
    ClientUserInputHandler* _inputHandler{nullptr};
    NetworkConfig* _networkConfig{nullptr};
    ClientIgnoreListManager* _ignoreListManager{nullptr};
    HighlightRuleManager* _highlightRuleManager{nullptr};
    ClientTransferManager* _transferManager{nullptr};
    TransferModel* _transferModel{nullptr};

    MessageModel* _messageModel{nullptr};
    AbstractMessageProcessor* _messageProcessor{nullptr};

    CoreAccountModel* _coreAccountModel{nullptr};
    CoreConnection* _coreConnection{nullptr};

    ClientMode clientMode{};

    QHash<NetworkId, Network*> _networks;
    QHash<IdentityId, Identity*> _identities;

    bool _connected{false};

    QList<QPair<BufferInfo, QString>> _userInputBuffer;

    friend class CoreConnection;
};

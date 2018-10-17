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

#include "client.h"

#include <cstdio>
#include <cstdlib>

#include "abstractmessageprocessor.h"
#include "abstractui.h"
#include "bufferinfo.h"
#include "buffermodel.h"
#include "buffersettings.h"
#include "buffersyncer.h"
#include "bufferviewconfig.h"
#include "bufferviewoverlay.h"
#include "clientaliasmanager.h"
#include "clientauthhandler.h"
#include "clientbacklogmanager.h"
#include "clientbufferviewmanager.h"
#include "clientidentity.h"
#include "clientignorelistmanager.h"
#include "clientirclisthelper.h"
#include "clienttransfermanager.h"
#include "clientuserinputhandler.h"
#include "coreaccountmodel.h"
#include "coreconnection.h"
#include "dccconfig.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "message.h"
#include "messagemodel.h"
#include "network.h"
#include "networkconfig.h"
#include "networkmodel.h"
#include "quassel.h"
#include "signalproxy.h"
#include "transfermodel.h"
#include "util.h"

Client::Client(std::unique_ptr<AbstractUi> ui, QObject* parent)
    : QObject(parent)
    , Singleton<Client>(this)
    , _signalProxy(new SignalProxy(SignalProxy::Client, this))
    , _mainUi(std::move(ui))
    , _networkModel(new NetworkModel(this))
    , _bufferModel(new BufferModel(_networkModel))
    , _backlogManager(new ClientBacklogManager(this))
    , _bufferViewOverlay(new BufferViewOverlay(this))
    , _coreInfo(new CoreInfo(this))
    , _ircListHelper(new ClientIrcListHelper(this))
    , _inputHandler(new ClientUserInputHandler(this))
    , _transferModel(new TransferModel(this))
    , _messageModel(_mainUi->createMessageModel(this))
    , _messageProcessor(_mainUi->createMessageProcessor(this))
    , _coreAccountModel(new CoreAccountModel(this))
    , _coreConnection(new CoreConnection(this))
{
#ifdef EMBED_DATA
    Q_INIT_RESOURCE(data);
#endif

    connect(mainUi(), &AbstractUi::disconnectFromCore, this, &Client::disconnectFromCore);
    connect(this, &Client::connected, mainUi(), &AbstractUi::connectedToCore);
    connect(this, &Client::disconnected, mainUi(), &AbstractUi::disconnectedFromCore);

    connect(this, &Client::networkRemoved, _networkModel, &NetworkModel::networkRemoved);
    connect(this, &Client::networkRemoved, _messageProcessor, &AbstractMessageProcessor::networkRemoved);

    connect(backlogManager(), &ClientBacklogManager::messagesReceived, _messageModel, &MessageModel::messagesReceived);
    connect(coreConnection(), &CoreConnection::stateChanged, this, &Client::connectionStateChanged);

    SignalProxy* p = signalProxy();

    p->attachSlot(SIGNAL(displayMsg(Message)), this, &Client::recvMessage);
    p->attachSlot(SIGNAL(displayStatusMsg(QString,QString)), this, &Client::recvStatusMsg);

    p->attachSlot(SIGNAL(bufferInfoUpdated(BufferInfo)), _networkModel, &NetworkModel::bufferUpdated);
    p->attachSignal(inputHandler(), &ClientUserInputHandler::sendInput);
    p->attachSignal(this, &Client::requestNetworkStates);

    p->attachSignal(this, &Client::requestCreateIdentity, SIGNAL(createIdentity(Identity,QVariantMap)));
    p->attachSignal(this, &Client::requestRemoveIdentity, SIGNAL(removeIdentity(IdentityId)));
    p->attachSlot(SIGNAL(identityCreated(Identity)), this, &Client::coreIdentityCreated);
    p->attachSlot(SIGNAL(identityRemoved(IdentityId)), this, &Client::coreIdentityRemoved);

    p->attachSignal(this, &Client::requestCreateNetwork, SIGNAL(createNetwork(NetworkInfo,QStringList)));
    p->attachSignal(this, &Client::requestRemoveNetwork, SIGNAL(removeNetwork(NetworkId)));
    p->attachSlot(SIGNAL(networkCreated(NetworkId)), this, &Client::coreNetworkCreated);
    p->attachSlot(SIGNAL(networkRemoved(NetworkId)), this, &Client::coreNetworkRemoved);

    p->attachSignal(this, &Client::requestPasswordChange, SIGNAL(changePassword(PeerPtr,QString,QString,QString)));
    p->attachSlot(SIGNAL(passwordChanged(PeerPtr,bool)), this, &Client::corePasswordChanged);

    p->attachSignal(this, &Client::requestKickClient, SIGNAL(kickClient(int)));
    p->attachSlot(SIGNAL(disconnectFromCore()), this, &Client::disconnectFromCore);

    p->synchronize(backlogManager());
    p->synchronize(coreInfo());
    p->synchronize(_ircListHelper);

    coreAccountModel()->load();
    coreConnection()->init();
}

Client::~Client()
{
    disconnectFromCore();
}

AbstractUi* Client::mainUi()
{
    return instance()->_mainUi.get();
}

bool Client::isCoreFeatureEnabled(Quassel::Feature feature)
{
    return coreConnection()->peer() ? coreConnection()->peer()->hasFeature(feature) : false;
}

bool Client::isConnected()
{
    return instance()->_connected;
}

bool Client::internalCore()
{
    return currentCoreAccount().isInternal();
}

void Client::onDbUpgradeInProgress(bool inProgress)
{
    emit dbUpgradeInProgress(inProgress);
}

void Client::onExitRequested(int exitCode, const QString& reason)
{
    if (!reason.isEmpty()) {
        qCritical() << reason;
        emit exitRequested(reason);
    }
    QCoreApplication::exit(exitCode);
}

/*** Network handling ***/

QList<NetworkId> Client::networkIds()
{
    return instance()->_networks.keys();
}

const Network* Client::network(NetworkId networkid)
{
    if (instance()->_networks.contains(networkid))
        return instance()->_networks[networkid];
    else
        return nullptr;
}

void Client::createNetwork(const NetworkInfo& info, const QStringList& persistentChannels)
{
    emit instance()->requestCreateNetwork(info, persistentChannels);
}

void Client::removeNetwork(NetworkId id)
{
    emit instance()->requestRemoveNetwork(id);
}

void Client::updateNetwork(const NetworkInfo& info)
{
    Network* netptr = instance()->_networks.value(info.networkId, 0);
    if (!netptr) {
        qWarning() << "Update for unknown network requested:" << info;
        return;
    }
    netptr->requestSetNetworkInfo(info);
}

void Client::addNetwork(Network* net)
{
    net->setProxy(signalProxy());
    signalProxy()->synchronize(net);
    networkModel()->attachNetwork(net);
    connect(net, &QObject::destroyed, instance(), &Client::networkDestroyed);
    instance()->_networks[net->networkId()] = net;
    emit instance()->networkCreated(net->networkId());
}

void Client::coreNetworkCreated(NetworkId id)
{
    if (_networks.contains(id)) {
        qWarning() << "Creation of already existing network requested!";
        return;
    }
    auto* net = new Network(id, this);
    addNetwork(net);
}

void Client::coreNetworkRemoved(NetworkId id)
{
    if (!_networks.contains(id))
        return;
    Network* net = _networks.take(id);
    emit networkRemoved(net->networkId());
    net->deleteLater();
}

/*** Identity handling ***/

QList<IdentityId> Client::identityIds()
{
    return instance()->_identities.keys();
}

const Identity* Client::identity(IdentityId id)
{
    if (instance()->_identities.contains(id))
        return instance()->_identities[id];
    else
        return nullptr;
}

void Client::createIdentity(const CertIdentity& id)
{
    QVariantMap additional;
#ifdef HAVE_SSL
    additional["KeyPem"] = id.sslKey().toPem();
    additional["CertPem"] = id.sslCert().toPem();
#endif
    emit instance()->requestCreateIdentity(id, additional);
}

void Client::updateIdentity(IdentityId id, const QVariantMap& ser)
{
    Identity* idptr = instance()->_identities.value(id, 0);
    if (!idptr) {
        qWarning() << "Update for unknown identity requested:" << id;
        return;
    }
    idptr->requestUpdate(ser);
}

void Client::removeIdentity(IdentityId id)
{
    emit instance()->requestRemoveIdentity(id);
}

void Client::coreIdentityCreated(const Identity& other)
{
    if (!_identities.contains(other.id())) {
        auto* identity = new Identity(other, this);
        _identities[other.id()] = identity;
        identity->setInitialized();
        signalProxy()->synchronize(identity);
        emit identityCreated(other.id());
    }
    else {
        qWarning() << tr("Identity already exists in client!");
    }
}

void Client::coreIdentityRemoved(IdentityId id)
{
    if (_identities.contains(id)) {
        emit identityRemoved(id);
        Identity* i = _identities.take(id);
        i->deleteLater();
    }
}

/*** User input handling ***/

void Client::userInput(const BufferInfo& bufferInfo, const QString& message)
{
    // we need to make sure that AliasManager is ready before processing input
    if (aliasManager() && aliasManager()->isInitialized())
        inputHandler()->handleUserInput(bufferInfo, message);
    else
        instance()->_userInputBuffer.append(qMakePair(bufferInfo, message));
}

void Client::sendBufferedUserInput()
{
    for (int i = 0; i < _userInputBuffer.count(); i++)
        userInput(_userInputBuffer.at(i).first, _userInputBuffer.at(i).second);

    _userInputBuffer.clear();
}

/*** core connection stuff ***/

void Client::connectionStateChanged(CoreConnection::ConnectionState state)
{
    switch (state) {
    case CoreConnection::Disconnected:
        setDisconnectedFromCore();
        break;
    case CoreConnection::Synchronized:
        setSyncedToCore();
        break;
    default:
        break;
    }
}

void Client::setSyncedToCore()
{
    // create buffersyncer
    Q_ASSERT(!_bufferSyncer);
    _bufferSyncer = new BufferSyncer(this);
    connect(bufferSyncer(), &BufferSyncer::lastSeenMsgSet, _networkModel, &NetworkModel::setLastSeenMsgId);
    connect(bufferSyncer(), &BufferSyncer::markerLineSet, _networkModel, &NetworkModel::setMarkerLineMsgId);
    connect(bufferSyncer(), &BufferSyncer::bufferRemoved, this, &Client::bufferRemoved);
    connect(bufferSyncer(), &BufferSyncer::bufferRenamed, this, &Client::bufferRenamed);
    connect(bufferSyncer(), &BufferSyncer::buffersPermanentlyMerged, this, &Client::buffersPermanentlyMerged);
    connect(bufferSyncer(), &BufferSyncer::buffersPermanentlyMerged, _messageModel, &MessageModel::buffersPermanentlyMerged);
    connect(bufferSyncer(), &BufferSyncer::bufferMarkedAsRead, this, &Client::bufferMarkedAsRead);
    connect(bufferSyncer(), &BufferSyncer::bufferActivityChanged, _networkModel, &NetworkModel::bufferActivityChanged);
    connect(bufferSyncer(), &BufferSyncer::highlightCountChanged, _networkModel, &NetworkModel::highlightCountChanged);
    connect(networkModel(), &NetworkModel::requestSetLastSeenMsg, bufferSyncer(), &BufferSyncer::requestSetLastSeenMsg);

    SignalProxy* p = signalProxy();
    p->synchronize(bufferSyncer());

    // create a new BufferViewManager
    Q_ASSERT(!_bufferViewManager);
    _bufferViewManager = new ClientBufferViewManager(p, this);
    connect(_bufferViewManager, &SyncableObject::initDone, _bufferViewOverlay, &BufferViewOverlay::restore);

    // create AliasManager
    Q_ASSERT(!_aliasManager);
    _aliasManager = new ClientAliasManager(this);
    connect(aliasManager(), &SyncableObject::initDone, this, &Client::sendBufferedUserInput);
    p->synchronize(aliasManager());

    // create NetworkConfig
    Q_ASSERT(!_networkConfig);
    _networkConfig = new NetworkConfig("GlobalNetworkConfig", this);
    p->synchronize(networkConfig());

    // create IgnoreListManager
    Q_ASSERT(!_ignoreListManager);
    _ignoreListManager = new ClientIgnoreListManager(this);
    p->synchronize(ignoreListManager());

    // create Core-Side HighlightRuleManager
    Q_ASSERT(!_highlightRuleManager);
    _highlightRuleManager = new HighlightRuleManager(this);
    p->synchronize(highlightRuleManager());
    // Listen to network removed events
    connect(this, &Client::networkRemoved, _highlightRuleManager, &HighlightRuleManager::networkRemoved);

    /*  not ready yet
        // create TransferManager and DccConfig if core supports them
        Q_ASSERT(!_dccConfig);
        Q_ASSERT(!_transferManager);
        if (isCoreFeatureEnabled(Quassel::Feature::DccFileTransfer)) {
            _dccConfig = new DccConfig(this);
            p->synchronize(dccConfig());
            _transferManager = new ClientTransferManager(this);
            _transferModel->setManager(_transferManager);
            p->synchronize(transferManager());
        }
    */

    // trigger backlog request once all active bufferviews are initialized
    connect(bufferViewOverlay(), &BufferViewOverlay::initDone, this, &Client::finishConnectionInitialization);

    _connected = true;
    emit connected();
    emit coreConnectionStateChanged(true);
}

void Client::finishConnectionInitialization()
{
    // usually it _should_ take longer until the bufferViews are initialized, so that's what
    // triggers this slot. But we have to make sure that we know all buffers yet.
    // so we check the BufferSyncer and in case it wasn't initialized we wait for that instead
    if (!bufferSyncer()->isInitialized()) {
        disconnect(bufferViewOverlay(), &BufferViewOverlay::initDone, this, &Client::finishConnectionInitialization);
        connect(bufferSyncer(), &SyncableObject::initDone, this, &Client::finishConnectionInitialization);
        return;
    }
    disconnect(bufferViewOverlay(), &BufferViewOverlay::initDone, this, &Client::finishConnectionInitialization);
    disconnect(bufferSyncer(), &SyncableObject::initDone, this, &Client::finishConnectionInitialization);

    requestInitialBacklog();
    if (isCoreFeatureEnabled(Quassel::Feature::BufferActivitySync)) {
        bufferSyncer()->markActivitiesChanged();
        bufferSyncer()->markHighlightCountsChanged();
    }
}

void Client::requestInitialBacklog()
{
    _backlogManager->requestInitialBacklog();
}

void Client::requestLegacyCoreInfo()
{
    // On older cores, the CoreInfo object was only synchronized on demand.  Synchronize now if
    // needed.
    if (isConnected() && !isCoreFeatureEnabled(Quassel::Feature::SyncedCoreInfo)) {
        // Delete the existing core info object (it will always exist as client is single-threaded)
        _coreInfo->deleteLater();
        // No need to set to null when creating new one immediately after

        // Create a fresh, unsynchronized CoreInfo object, emulating legacy behavior of CoreInfo not
        // persisting
        _coreInfo = new CoreInfo(this);
        // Synchronize the new object
        signalProxy()->synchronize(_coreInfo);

        // Let others know signal handlers have been reset
        emit coreInfoResynchronized();
    }
}

void Client::disconnectFromCore()
{
    if (!coreConnection()->isConnected())
        return;

    coreConnection()->disconnectFromCore();
}

void Client::setDisconnectedFromCore()
{
    _connected = false;

    emit disconnected();
    emit coreConnectionStateChanged(false);

    backlogManager()->reset();
    messageProcessor()->reset();

    // Clear internal data. Hopefully nothing relies on it at this point.

    if (_bufferSyncer) {
        _bufferSyncer->deleteLater();
        _bufferSyncer = nullptr;
    }

    _coreInfo->reset();

    if (_bufferViewManager) {
        _bufferViewManager->deleteLater();
        _bufferViewManager = nullptr;
    }

    _bufferViewOverlay->reset();

    if (_aliasManager) {
        _aliasManager->deleteLater();
        _aliasManager = nullptr;
    }

    if (_ignoreListManager) {
        _ignoreListManager->deleteLater();
        _ignoreListManager = nullptr;
    }

    if (_highlightRuleManager) {
        _highlightRuleManager->deleteLater();
        _highlightRuleManager = nullptr;
    }

    if (_transferManager) {
        _transferModel->setManager(nullptr);
        _transferManager->deleteLater();
        _transferManager = nullptr;
    }

    if (_dccConfig) {
        _dccConfig->deleteLater();
        _dccConfig = nullptr;
    }

    // we probably don't want to save pending input for reconnect
    _userInputBuffer.clear();

    _messageModel->clear();
    _networkModel->clear();

    QHash<NetworkId, Network*>::iterator netIter = _networks.begin();
    while (netIter != _networks.end()) {
        Network* net = netIter.value();
        emit networkRemoved(net->networkId());
        disconnect(net, &Network::destroyed, this, nullptr);
        netIter = _networks.erase(netIter);
        net->deleteLater();
    }
    Q_ASSERT(_networks.isEmpty());

    QHash<IdentityId, Identity*>::iterator idIter = _identities.begin();
    while (idIter != _identities.end()) {
        emit identityRemoved(idIter.key());
        Identity* id = idIter.value();
        idIter = _identities.erase(idIter);
        id->deleteLater();
    }
    Q_ASSERT(_identities.isEmpty());

    if (_networkConfig) {
        _networkConfig->deleteLater();
        _networkConfig = nullptr;
    }
}

/*** ***/

void Client::networkDestroyed()
{
    auto* net = static_cast<Network*>(sender());
    QHash<NetworkId, Network*>::iterator netIter = _networks.begin();
    while (netIter != _networks.end()) {
        if (*netIter == net) {
            netIter = _networks.erase(netIter);
            break;
        }
        else {
            ++netIter;
        }
    }
}

// Hmm... we never used this...
void Client::recvStatusMsg(QString /*net*/, QString /*msg*/)
{
    // recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));
}

void Client::recvMessage(const Message& msg)
{
    Message msg_ = msg;
    messageProcessor()->process(msg_);
}

void Client::setBufferLastSeenMsg(BufferId id, const MsgId& msgId)
{
    if (bufferSyncer())
        bufferSyncer()->requestSetLastSeenMsg(id, msgId);
}

void Client::setMarkerLine(BufferId id, const MsgId& msgId)
{
    if (bufferSyncer())
        bufferSyncer()->requestSetMarkerLine(id, msgId);
}

MsgId Client::markerLine(BufferId id)
{
    if (id.isValid() && networkModel())
        return networkModel()->markerLineMsgId(id);
    return {};
}

void Client::removeBuffer(BufferId id)
{
    if (!bufferSyncer())
        return;
    bufferSyncer()->requestRemoveBuffer(id);
}

void Client::renameBuffer(BufferId bufferId, const QString& newName)
{
    if (!bufferSyncer())
        return;
    bufferSyncer()->requestRenameBuffer(bufferId, newName);
}

void Client::mergeBuffersPermanently(BufferId bufferId1, BufferId bufferId2)
{
    if (!bufferSyncer())
        return;
    bufferSyncer()->requestMergeBuffersPermanently(bufferId1, bufferId2);
}

void Client::purgeKnownBufferIds()
{
    if (!bufferSyncer())
        return;
    bufferSyncer()->requestPurgeBufferIds();
}

void Client::bufferRemoved(BufferId bufferId)
{
    // select a sane buffer (status buffer)
    /* we have to manually select a buffer because otherwise inconsitent changes
     * to the model might occur:
     * the result of a buffer removal triggers a change in the selection model.
     * the newly selected buffer might be a channel that hasn't been selected yet
     * and a new nickview would be created (which never heard of the "rowsAboutToBeRemoved").
     * this new view (and/or) its sort filter will then only receive a "rowsRemoved" signal.
     */
    QModelIndex current = bufferModel()->currentIndex();
    if (current.data(NetworkModel::BufferIdRole).value<BufferId>() == bufferId) {
        bufferModel()->setCurrentIndex(current.sibling(0, 0));
    }

    // and remove it from the model
    networkModel()->removeBuffer(bufferId);
}

void Client::bufferRenamed(BufferId bufferId, const QString& newName)
{
    QModelIndex bufferIndex = networkModel()->bufferIndex(bufferId);
    if (bufferIndex.isValid()) {
        networkModel()->setData(bufferIndex, newName, Qt::DisplayRole);
    }
}

void Client::buffersPermanentlyMerged(BufferId bufferId1, BufferId bufferId2)
{
    QModelIndex idx = networkModel()->bufferIndex(bufferId1);
    bufferModel()->setCurrentIndex(bufferModel()->mapFromSource(idx));
    networkModel()->removeBuffer(bufferId2);
}

void Client::markBufferAsRead(BufferId id)
{
    if (bufferSyncer() && id.isValid())
        bufferSyncer()->requestMarkBufferAsRead(id);
}

void Client::refreshLegacyCoreInfo()
{
    instance()->requestLegacyCoreInfo();
}

void Client::changePassword(const QString& oldPassword, const QString& newPassword)
{
    CoreAccount account = currentCoreAccount();
    account.setPassword(newPassword);
    coreAccountModel()->createOrUpdateAccount(account);
    emit instance()->requestPasswordChange(nullptr, account.user(), oldPassword, newPassword);
}

void Client::kickClient(int peerId)
{
    emit instance()->requestKickClient(peerId);
}

void Client::corePasswordChanged(PeerPtr, bool success)
{
    if (success)
        coreAccountModel()->save();
    emit passwordChanged(success);
}

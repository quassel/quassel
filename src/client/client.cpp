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

#include "client.h"

#include "abstractmessageprocessor.h"
#include "abstractui.h"
#include "bufferinfo.h"
#include "buffermodel.h"
#include "buffersettings.h"
#include "buffersyncer.h"
#include "bufferviewconfig.h"
#include "bufferviewoverlay.h"
#include "clientaliasmanager.h"
#include "clientbacklogmanager.h"
#include "clientbufferviewmanager.h"
#include "clientirclisthelper.h"
#include "clientidentity.h"
#include "clientignorelistmanager.h"
#include "clientuserinputhandler.h"
#include "coreaccountmodel.h"
#include "coreconnection.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "message.h"
#include "messagemodel.h"
#include "network.h"
#include "networkconfig.h"
#include "networkmodel.h"
#include "quassel.h"
#include "signalproxy.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

QPointer<Client> Client::instanceptr = 0;
Quassel::Features Client::_coreFeatures = 0;

/*** Initialization/destruction ***/

bool Client::instanceExists()
{
    return instanceptr;
}


Client *Client::instance()
{
    if (!instanceptr)
        instanceptr = new Client();
    return instanceptr;
}


void Client::destroy()
{
    if (instanceptr) {
        delete instanceptr->mainUi();
        instanceptr->deleteLater();
        instanceptr = 0;
    }
}


void Client::init(AbstractUi *ui)
{
    instance()->_mainUi = ui;
    instance()->init();
}


Client::Client(QObject *parent)
    : QObject(parent),
    _signalProxy(new SignalProxy(SignalProxy::Client, this)),
    _mainUi(0),
    _networkModel(0),
    _bufferModel(0),
    _bufferSyncer(0),
    _aliasManager(0),
    _backlogManager(new ClientBacklogManager(this)),
    _bufferViewManager(0),
    _bufferViewOverlay(new BufferViewOverlay(this)),
    _ircListHelper(new ClientIrcListHelper(this)),
    _inputHandler(0),
    _networkConfig(0),
    _ignoreListManager(0),
    _messageModel(0),
    _messageProcessor(0),
    _coreAccountModel(new CoreAccountModel(this)),
    _coreConnection(new CoreConnection(_coreAccountModel, this)),
    _connected(false),
    _debugLog(&_debugLogBuffer)
{
    _signalProxy->synchronize(_ircListHelper);
}


Client::~Client()
{
    disconnectFromCore();
}


void Client::init()
{
    _networkModel = new NetworkModel(this);

    connect(this, SIGNAL(networkRemoved(NetworkId)),
        _networkModel, SLOT(networkRemoved(NetworkId)));

    _bufferModel = new BufferModel(_networkModel);
    _messageModel = mainUi()->createMessageModel(this);
    _messageProcessor = mainUi()->createMessageProcessor(this);
    _inputHandler = new ClientUserInputHandler(this);

    SignalProxy *p = signalProxy();

    p->attachSlot(SIGNAL(displayMsg(const Message &)), this, SLOT(recvMessage(const Message &)));
    p->attachSlot(SIGNAL(displayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));

    p->attachSlot(SIGNAL(bufferInfoUpdated(BufferInfo)), _networkModel, SLOT(bufferUpdated(BufferInfo)));
    p->attachSignal(inputHandler(), SIGNAL(sendInput(BufferInfo, QString)));
    p->attachSignal(this, SIGNAL(requestNetworkStates()));

    p->attachSignal(this, SIGNAL(requestCreateIdentity(const Identity &, const QVariantMap &)), SIGNAL(createIdentity(const Identity &, const QVariantMap &)));
    p->attachSignal(this, SIGNAL(requestRemoveIdentity(IdentityId)), SIGNAL(removeIdentity(IdentityId)));
    p->attachSlot(SIGNAL(identityCreated(const Identity &)), this, SLOT(coreIdentityCreated(const Identity &)));
    p->attachSlot(SIGNAL(identityRemoved(IdentityId)), this, SLOT(coreIdentityRemoved(IdentityId)));

    p->attachSignal(this, SIGNAL(requestCreateNetwork(const NetworkInfo &, const QStringList &)), SIGNAL(createNetwork(const NetworkInfo &, const QStringList &)));
    p->attachSignal(this, SIGNAL(requestRemoveNetwork(NetworkId)), SIGNAL(removeNetwork(NetworkId)));
    p->attachSlot(SIGNAL(networkCreated(NetworkId)), this, SLOT(coreNetworkCreated(NetworkId)));
    p->attachSlot(SIGNAL(networkRemoved(NetworkId)), this, SLOT(coreNetworkRemoved(NetworkId)));

    //connect(mainUi(), SIGNAL(connectToCore(const QVariantMap &)), this, SLOT(connectToCore(const QVariantMap &)));
    connect(mainUi(), SIGNAL(disconnectFromCore()), this, SLOT(disconnectFromCore()));
    connect(this, SIGNAL(connected()), mainUi(), SLOT(connectedToCore()));
    connect(this, SIGNAL(disconnected()), mainUi(), SLOT(disconnectedFromCore()));

    // attach backlog manager
    p->synchronize(backlogManager());
    connect(backlogManager(), SIGNAL(messagesReceived(BufferId, int)), _messageModel, SLOT(messagesReceived(BufferId, int)));

    coreAccountModel()->load();

    connect(coreConnection(), SIGNAL(stateChanged(CoreConnection::ConnectionState)), SLOT(connectionStateChanged(CoreConnection::ConnectionState)));
    coreConnection()->init();
}


/*** public static methods ***/

AbstractUi *Client::mainUi()
{
    return instance()->_mainUi;
}


void Client::setCoreFeatures(Quassel::Features features)
{
    _coreFeatures = features;
}


bool Client::isConnected()
{
    return instance()->_connected;
}


bool Client::internalCore()
{
    return currentCoreAccount().isInternal();
}


/*** Network handling ***/

QList<NetworkId> Client::networkIds()
{
    return instance()->_networks.keys();
}


const Network *Client::network(NetworkId networkid)
{
    if (instance()->_networks.contains(networkid)) return instance()->_networks[networkid];
    else return 0;
}


void Client::createNetwork(const NetworkInfo &info, const QStringList &persistentChannels)
{
    emit instance()->requestCreateNetwork(info, persistentChannels);
}


void Client::removeNetwork(NetworkId id)
{
    emit instance()->requestRemoveNetwork(id);
}


void Client::updateNetwork(const NetworkInfo &info)
{
    Network *netptr = instance()->_networks.value(info.networkId, 0);
    if (!netptr) {
        qWarning() << "Update for unknown network requested:" << info;
        return;
    }
    netptr->requestSetNetworkInfo(info);
}


void Client::addNetwork(Network *net)
{
    net->setProxy(signalProxy());
    signalProxy()->synchronize(net);
    networkModel()->attachNetwork(net);
    connect(net, SIGNAL(destroyed()), instance(), SLOT(networkDestroyed()));
    instance()->_networks[net->networkId()] = net;
    emit instance()->networkCreated(net->networkId());
}


void Client::coreNetworkCreated(NetworkId id)
{
    if (_networks.contains(id)) {
        qWarning() << "Creation of already existing network requested!";
        return;
    }
    Network *net = new Network(id, this);
    addNetwork(net);
}


void Client::coreNetworkRemoved(NetworkId id)
{
    if (!_networks.contains(id))
        return;
    Network *net = _networks.take(id);
    emit networkRemoved(net->networkId());
    net->deleteLater();
}


/*** Identity handling ***/

QList<IdentityId> Client::identityIds()
{
    return instance()->_identities.keys();
}


const Identity *Client::identity(IdentityId id)
{
    if (instance()->_identities.contains(id)) return instance()->_identities[id];
    else return 0;
}


void Client::createIdentity(const CertIdentity &id)
{
    QVariantMap additional;
#ifdef HAVE_SSL
    additional["KeyPem"] = id.sslKey().toPem();
    additional["CertPem"] = id.sslCert().toPem();
#endif
    emit instance()->requestCreateIdentity(id, additional);
}


void Client::updateIdentity(IdentityId id, const QVariantMap &ser)
{
    Identity *idptr = instance()->_identities.value(id, 0);
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


void Client::coreIdentityCreated(const Identity &other)
{
    if (!_identities.contains(other.id())) {
        Identity *identity = new Identity(other, this);
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
        Identity *i = _identities.take(id);
        i->deleteLater();
    }
}


/*** User input handling ***/

void Client::userInput(const BufferInfo &bufferInfo, const QString &message)
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
    connect(bufferSyncer(), SIGNAL(lastSeenMsgSet(BufferId, MsgId)), _networkModel, SLOT(setLastSeenMsgId(BufferId, MsgId)));
    connect(bufferSyncer(), SIGNAL(markerLineSet(BufferId, MsgId)), _networkModel, SLOT(setMarkerLineMsgId(BufferId, MsgId)));
    connect(bufferSyncer(), SIGNAL(bufferRemoved(BufferId)), this, SLOT(bufferRemoved(BufferId)));
    connect(bufferSyncer(), SIGNAL(bufferRenamed(BufferId, QString)), this, SLOT(bufferRenamed(BufferId, QString)));
    connect(bufferSyncer(), SIGNAL(buffersPermanentlyMerged(BufferId, BufferId)), this, SLOT(buffersPermanentlyMerged(BufferId, BufferId)));
    connect(bufferSyncer(), SIGNAL(buffersPermanentlyMerged(BufferId, BufferId)), _messageModel, SLOT(buffersPermanentlyMerged(BufferId, BufferId)));
    connect(bufferSyncer(), SIGNAL(bufferMarkedAsRead(BufferId)), SIGNAL(bufferMarkedAsRead(BufferId)));
    connect(networkModel(), SIGNAL(requestSetLastSeenMsg(BufferId, MsgId)), bufferSyncer(), SLOT(requestSetLastSeenMsg(BufferId, const MsgId &)));
    signalProxy()->synchronize(bufferSyncer());

    // create a new BufferViewManager
    Q_ASSERT(!_bufferViewManager);
    _bufferViewManager = new ClientBufferViewManager(signalProxy(), this);
    connect(_bufferViewManager, SIGNAL(initDone()), _bufferViewOverlay, SLOT(restore()));

    // create AliasManager
    Q_ASSERT(!_aliasManager);
    _aliasManager = new ClientAliasManager(this);
    connect(aliasManager(), SIGNAL(initDone()), SLOT(sendBufferedUserInput()));
    signalProxy()->synchronize(aliasManager());

    // create NetworkConfig
    Q_ASSERT(!_networkConfig);
    _networkConfig = new NetworkConfig("GlobalNetworkConfig", this);
    signalProxy()->synchronize(networkConfig());

    // create IgnoreListManager
    Q_ASSERT(!_ignoreListManager);
    _ignoreListManager = new ClientIgnoreListManager(this);
    signalProxy()->synchronize(ignoreListManager());

    // trigger backlog request once all active bufferviews are initialized
    connect(bufferViewOverlay(), SIGNAL(initDone()), this, SLOT(requestInitialBacklog()));

    _connected = true;
    emit connected();
    emit coreConnectionStateChanged(true);
}


void Client::requestInitialBacklog()
{
    // usually it _should_ take longer until the bufferViews are initialized, so that's what
    // triggers this slot. But we have to make sure that we know all buffers yet.
    // so we check the BufferSyncer and in case it wasn't initialized we wait for that instead
    if (!bufferSyncer()->isInitialized()) {
        disconnect(bufferViewOverlay(), SIGNAL(initDone()), this, SLOT(requestInitialBacklog()));
        connect(bufferSyncer(), SIGNAL(initDone()), this, SLOT(requestInitialBacklog()));
        return;
    }
    disconnect(bufferViewOverlay(), SIGNAL(initDone()), this, SLOT(requestInitialBacklog()));
    disconnect(bufferSyncer(), SIGNAL(initDone()), this, SLOT(requestInitialBacklog()));

    _backlogManager->requestInitialBacklog();
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
    _coreFeatures = 0;

    emit disconnected();
    emit coreConnectionStateChanged(false);

    backlogManager()->reset();
    messageProcessor()->reset();

    // Clear internal data. Hopefully nothing relies on it at this point.

    if (_bufferSyncer) {
        _bufferSyncer->deleteLater();
        _bufferSyncer = 0;
    }

    if (_bufferViewManager) {
        _bufferViewManager->deleteLater();
        _bufferViewManager = 0;
    }

    _bufferViewOverlay->reset();

    if (_aliasManager) {
        _aliasManager->deleteLater();
        _aliasManager = 0;
    }

    if (_ignoreListManager) {
        _ignoreListManager->deleteLater();
        _ignoreListManager = 0;
    }
    // we probably don't want to save pending input for reconnect
    _userInputBuffer.clear();

    _messageModel->clear();
    _networkModel->clear();

    QHash<NetworkId, Network *>::iterator netIter = _networks.begin();
    while (netIter != _networks.end()) {
        Network *net = netIter.value();
        emit networkRemoved(net->networkId());
        disconnect(net, SIGNAL(destroyed()), this, 0);
        netIter = _networks.erase(netIter);
        net->deleteLater();
    }
    Q_ASSERT(_networks.isEmpty());

    QHash<IdentityId, Identity *>::iterator idIter = _identities.begin();
    while (idIter != _identities.end()) {
        emit identityRemoved(idIter.key());
        Identity *id = idIter.value();
        idIter = _identities.erase(idIter);
        id->deleteLater();
    }
    Q_ASSERT(_identities.isEmpty());

    if (_networkConfig) {
        _networkConfig->deleteLater();
        _networkConfig = 0;
    }
}


/*** ***/

void Client::networkDestroyed()
{
    Network *net = static_cast<Network *>(sender());
    QHash<NetworkId, Network *>::iterator netIter = _networks.begin();
    while (netIter != _networks.end()) {
        if (*netIter == net) {
            netIter = _networks.erase(netIter);
            break;
        }
        else {
            netIter++;
        }
    }
}


// Hmm... we never used this...
void Client::recvStatusMsg(QString /*net*/, QString /*msg*/)
{
    //recvMessage(net, Message::server("", QString("[STATUS] %1").arg(msg)));
}


void Client::recvMessage(const Message &msg)
{
    Message msg_ = msg;
    messageProcessor()->process(msg_);
}


void Client::setBufferLastSeenMsg(BufferId id, const MsgId &msgId)
{
    if (bufferSyncer())
        bufferSyncer()->requestSetLastSeenMsg(id, msgId);
}


void Client::setMarkerLine(BufferId id, const MsgId &msgId)
{
    if (bufferSyncer())
        bufferSyncer()->requestSetMarkerLine(id, msgId);
}


MsgId Client::markerLine(BufferId id)
{
    if (id.isValid() && networkModel())
        return networkModel()->markerLineMsgId(id);
    return MsgId();
}


void Client::removeBuffer(BufferId id)
{
    if (!bufferSyncer()) return;
    bufferSyncer()->requestRemoveBuffer(id);
}


void Client::renameBuffer(BufferId bufferId, const QString &newName)
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


void Client::bufferRenamed(BufferId bufferId, const QString &newName)
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


void Client::logMessage(QtMsgType type, const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
    if (type == QtFatalMsg) {
        Quassel::logFatalMessage(msg);
    }
    else {
        QString msgString = QString("%1\n").arg(msg);

        //Check to see if there is an instance around, else we risk recursions
        //when calling instance() and creating new ones.
        if (!instanceExists())
            return;

        instance()->_debugLog << msgString;
        emit instance()->logUpdated(msgString);
    }
}

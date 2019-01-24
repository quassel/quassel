/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "coresession.h"

#include <QtScript>

#include "core.h"
#include "coreuserinputhandler.h"
#include "corebuffersyncer.h"
#include "corebacklogmanager.h"
#include "corebufferviewmanager.h"
#include "coredccconfig.h"
#include "coreeventmanager.h"
#include "coreidentity.h"
#include "coreignorelistmanager.h"
#include "coreinfo.h"
#include "coreirclisthelper.h"
#include "corenetwork.h"
#include "corenetworkconfig.h"
#include "coresessioneventprocessor.h"
#include "coretransfermanager.h"
#include "coreusersettings.h"
#include "ctcpparser.h"
#include "eventstringifier.h"
#include "internalpeer.h"
#include "ircchannel.h"
#include "ircparser.h"
#include "ircuser.h"
#include "logmessage.h"
#include "messageevent.h"
#include "remotepeer.h"
#include "storage.h"
#include "util.h"


class ProcessMessagesEvent : public QEvent
{
public:
    ProcessMessagesEvent() : QEvent(QEvent::User) {}
};


CoreSession::CoreSession(UserId uid, bool restoreState, bool strictIdentEnabled, QObject *parent)
    : QObject(parent),
    _user(uid),
    _strictIdentEnabled(strictIdentEnabled),
    _signalProxy(new SignalProxy(SignalProxy::Server, this)),
    _aliasManager(this),
    _bufferSyncer(new CoreBufferSyncer(this)),
    _backlogManager(new CoreBacklogManager(this)),
    _bufferViewManager(new CoreBufferViewManager(_signalProxy, this)),
    _dccConfig(new CoreDccConfig(this)),
    _ircListHelper(new CoreIrcListHelper(this)),
    _networkConfig(new CoreNetworkConfig("GlobalNetworkConfig", this)),
    _coreInfo(new CoreInfo(this)),
    _transferManager(new CoreTransferManager(this)),
    _eventManager(new CoreEventManager(this)),
    _eventStringifier(new EventStringifier(this)),
    _sessionEventProcessor(new CoreSessionEventProcessor(this)),
    _ctcpParser(new CtcpParser(this)),
    _ircParser(new IrcParser(this)),
    scriptEngine(new QScriptEngine(this)),
    _processMessages(false),
    _ignoreListManager(this),
    _highlightRuleManager(this)
{
    SignalProxy *p = signalProxy();
    p->setHeartBeatInterval(30);
    p->setMaxHeartBeatCount(60); // 30 mins until we throw a dead socket out

    connect(p, SIGNAL(peerRemoved(Peer*)), SLOT(removeClient(Peer*)));

    connect(p, SIGNAL(connected()), SLOT(clientsConnected()));
    connect(p, SIGNAL(disconnected()), SLOT(clientsDisconnected()));

    p->attachSlot(SIGNAL(sendInput(BufferInfo, QString)), this, SLOT(msgFromClient(BufferInfo, QString)));
    p->attachSignal(this, SIGNAL(displayMsg(Message)));
    p->attachSignal(this, SIGNAL(displayStatusMsg(QString, QString)));

    p->attachSignal(this, SIGNAL(identityCreated(const Identity &)));
    p->attachSignal(this, SIGNAL(identityRemoved(IdentityId)));
    p->attachSlot(SIGNAL(createIdentity(const Identity &, const QVariantMap &)), this, SLOT(createIdentity(const Identity &, const QVariantMap &)));
    p->attachSlot(SIGNAL(removeIdentity(IdentityId)), this, SLOT(removeIdentity(IdentityId)));

    p->attachSignal(this, SIGNAL(networkCreated(NetworkId)));
    p->attachSignal(this, SIGNAL(networkRemoved(NetworkId)));
    p->attachSlot(SIGNAL(createNetwork(const NetworkInfo &, const QStringList &)), this, SLOT(createNetwork(const NetworkInfo &, const QStringList &)));
    p->attachSlot(SIGNAL(removeNetwork(NetworkId)), this, SLOT(removeNetwork(NetworkId)));

    p->attachSlot(SIGNAL(changePassword(PeerPtr,QString,QString,QString)), this, SLOT(changePassword(PeerPtr,QString,QString,QString)));
    p->attachSignal(this, SIGNAL(passwordChanged(PeerPtr,bool)));

    p->attachSlot(SIGNAL(kickClient(int)), this, SLOT(kickClient(int)));
    p->attachSignal(this, SIGNAL(disconnectFromCore()));

    QVariantMap data;
    data["quasselVersion"] = Quassel::buildInfo().fancyVersionString;
    data["quasselBuildDate"] = Quassel::buildInfo().commitDate; // "BuildDate" for compatibility
    data["startTime"] = Core::instance()->startTime();
    data["sessionConnectedClients"] = 0;
    _coreInfo->setCoreData(data);

    loadSettings();
    initScriptEngine();

    eventManager()->registerObject(ircParser(), EventManager::NormalPriority);
    eventManager()->registerObject(sessionEventProcessor(), EventManager::HighPriority); // needs to process events *before* the stringifier!
    eventManager()->registerObject(ctcpParser(), EventManager::NormalPriority);
    eventManager()->registerObject(eventStringifier(), EventManager::NormalPriority);
    eventManager()->registerObject(this, EventManager::LowPriority); // for sending MessageEvents to the client
    // some events need to be handled after msg generation
    eventManager()->registerObject(sessionEventProcessor(), EventManager::LowPriority, "lateProcess");
    eventManager()->registerObject(ctcpParser(), EventManager::LowPriority, "send");

    // periodically save our session state
    connect(Core::instance()->syncTimer(), SIGNAL(timeout()), this, SLOT(saveSessionState()));

    p->synchronize(_bufferSyncer);
    p->synchronize(&aliasManager());
    p->synchronize(_backlogManager);
    p->synchronize(dccConfig());
    p->synchronize(ircListHelper());
    p->synchronize(networkConfig());
    p->synchronize(_coreInfo);
    p->synchronize(&_ignoreListManager);
    p->synchronize(&_highlightRuleManager);
    // Listen to network removed events
    connect(this, SIGNAL(networkRemoved(NetworkId)),
        &_highlightRuleManager, SLOT(networkRemoved(NetworkId)));
    p->synchronize(transferManager());
    // Restore session state
    if (restoreState)
        restoreSessionState();

    emit initialized();
}


void CoreSession::shutdown()
{
    saveSessionState();

    // Request disconnect from all connected networks in parallel, and wait until every network
    // has emitted the disconnected() signal before deleting the session itself
    for (CoreNetwork *net : _networks.values()) {
        if (net->socketState() != QAbstractSocket::UnconnectedState) {
            _networksPendingDisconnect.insert(net->networkId());
            connect(net, SIGNAL(disconnected(NetworkId)), this, SLOT(onNetworkDisconnected(NetworkId)));
            net->shutdown();
        }
    }

    if (_networksPendingDisconnect.isEmpty()) {
        // Nothing to do, suicide so the core can shut down
        deleteLater();
    }
}


void CoreSession::onNetworkDisconnected(NetworkId networkId)
{
    _networksPendingDisconnect.remove(networkId);
    if (_networksPendingDisconnect.isEmpty()) {
        // We're done, suicide so the core can shut down
        deleteLater();
    }
}


CoreNetwork *CoreSession::network(NetworkId id) const
{
    if (_networks.contains(id)) return _networks[id];
    return 0;
}


CoreIdentity *CoreSession::identity(IdentityId id) const
{
    if (_identities.contains(id)) return _identities[id];
    return 0;
}


void CoreSession::loadSettings()
{
    CoreUserSettings s(user());

    // migrate to db
    QList<IdentityId> ids = s.identityIds();
    QList<NetworkInfo> networkInfos = Core::networks(user());
    foreach(IdentityId id, ids) {
        CoreIdentity identity(s.identity(id));
        IdentityId newId = Core::createIdentity(user(), identity);
        QList<NetworkInfo>::iterator networkIter = networkInfos.begin();
        while (networkIter != networkInfos.end()) {
            if (networkIter->identity == id) {
                networkIter->identity = newId;
                Core::updateNetwork(user(), *networkIter);
                networkIter = networkInfos.erase(networkIter);
            }
            else {
                ++networkIter;
            }
        }
        s.removeIdentity(id);
    }
    // end of migration

    foreach(CoreIdentity identity, Core::identities(user())) {
        createIdentity(identity);
    }

    foreach(NetworkInfo info, Core::networks(user())) {
        createNetwork(info);
    }
}


void CoreSession::saveSessionState() const
{
    _bufferSyncer->storeDirtyIds();
    _bufferViewManager->saveBufferViews();
    _networkConfig->save();
}


void CoreSession::restoreSessionState()
{
    QList<NetworkId> nets = Core::connectedNetworks(user());
    CoreNetwork *net = 0;
    foreach(NetworkId id, nets) {
        net = network(id);
        Q_ASSERT(net);
        net->connectToIrc();
    }
}


void CoreSession::addClient(RemotePeer *peer)
{
    signalProxy()->setTargetPeer(peer);

    peer->dispatch(sessionState());
    signalProxy()->addPeer(peer);
    _coreInfo->setConnectedClientData(signalProxy()->peerCount(), signalProxy()->peerData());

    signalProxy()->setTargetPeer(nullptr);
}


void CoreSession::addClient(InternalPeer *peer)
{
    signalProxy()->addPeer(peer);
    emit sessionState(sessionState());
}


void CoreSession::removeClient(Peer *peer)
{
    RemotePeer *p = qobject_cast<RemotePeer *>(peer);
    if (p)
        quInfo() << qPrintable(tr("Client")) << p->description() << qPrintable(tr("disconnected (UserId: %1).").arg(user().toInt()));
    _coreInfo->setConnectedClientData(signalProxy()->peerCount(), signalProxy()->peerData());
}


QHash<QString, QString> CoreSession::persistentChannels(NetworkId id) const
{
    return Core::persistentChannels(user(), id);
}


QHash<QString, QByteArray> CoreSession::bufferCiphers(NetworkId id) const
{
    return Core::bufferCiphers(user(), id);
}

void CoreSession::setBufferCipher(NetworkId id, const QString &bufferName, const QByteArray &cipher) const
{
    Core::setBufferCipher(user(), id, bufferName, cipher);
}


// FIXME switch to BufferId
void CoreSession::msgFromClient(BufferInfo bufinfo, QString msg)
{
    CoreNetwork *net = network(bufinfo.networkId());
    if (net) {
        net->userInput(bufinfo, msg);
    }
    else {
        qWarning() << "Trying to send to unconnected network:" << msg;
    }
}


// ALL messages coming pass through these functions before going to the GUI.
// So this is the perfect place for storing the backlog and log stuff.
void CoreSession::recvMessageFromServer(NetworkId networkId, Message::Type type, BufferInfo::Type bufferType,
    const QString &target, const QString &text_, const QString &sender, Message::Flags flags)
{
    // U+FDD0 and U+FDD1 are special characters for Qt's text engine, specifically they mark the boundaries of
    // text frames in a QTextDocument. This might lead to problems in widgets displaying QTextDocuments (such as
    // KDE's notifications), hence we remove those just to be safe.
    QString text = text_;
    text.remove(QChar(0xfdd0)).remove(QChar(0xfdd1));
    RawMessage rawMsg(networkId, type, bufferType, target, text, sender, flags);

    // check for HardStrictness ignore
    CoreNetwork *currentNetwork = network(networkId);
    QString networkName = currentNetwork ? currentNetwork->networkName() : QString("");
    if (_ignoreListManager.match(rawMsg, networkName) == IgnoreListManager::HardStrictness)
        return;


    if (currentNetwork && _highlightRuleManager.match(rawMsg, currentNetwork->myNick(), currentNetwork->identityPtr()->nicks()))
        rawMsg.flags |= Message::Flag::Highlight;

    _messageQueue << rawMsg;
    if (!_processMessages) {
        _processMessages = true;
        QCoreApplication::postEvent(this, new ProcessMessagesEvent());
    }
}


void CoreSession::recvStatusMsgFromServer(QString msg)
{
    CoreNetwork *net = qobject_cast<CoreNetwork *>(sender());
    Q_ASSERT(net);
    emit displayStatusMsg(net->networkName(), msg);
}


void CoreSession::processMessageEvent(MessageEvent *event)
{
    recvMessageFromServer(event->networkId(), event->msgType(), event->bufferType(),
        event->target().isNull() ? "" : event->target(),
        event->text().isNull() ? "" : event->text(),
        event->sender().isNull() ? "" : event->sender(),
        event->msgFlags());
}


QList<BufferInfo> CoreSession::buffers() const
{
    return Core::requestBuffers(user());
}


void CoreSession::customEvent(QEvent *event)
{
    if (event->type() != QEvent::User)
        return;

    processMessages();
    event->accept();
}


void CoreSession::processMessages()
{
    if (_messageQueue.count() == 1) {
        const RawMessage &rawMsg = _messageQueue.first();
        bool createBuffer = !(rawMsg.flags & Message::Redirected);
        BufferInfo bufferInfo = Core::bufferInfo(user(), rawMsg.networkId, rawMsg.bufferType, rawMsg.target, createBuffer);
        if (!bufferInfo.isValid()) {
            Q_ASSERT(!createBuffer);
            bufferInfo = Core::bufferInfo(user(), rawMsg.networkId, BufferInfo::StatusBuffer, "");
        }
        Message msg(bufferInfo, rawMsg.type, rawMsg.text, rawMsg.sender, senderPrefixes(rawMsg.sender, bufferInfo),
                    realName(rawMsg.sender, rawMsg.networkId),  avatarUrl(rawMsg.sender, rawMsg.networkId),
                    rawMsg.flags);
        if(Core::storeMessage(msg))
            emit displayMsg(msg);
    }
    else {
        QHash<NetworkId, QHash<QString, BufferInfo> > bufferInfoCache;
        MessageList messages;
        QList<RawMessage> redirectedMessages; // list of Messages which don't enforce a buffer creation
        BufferInfo bufferInfo;
        for (int i = 0; i < _messageQueue.count(); i++) {
            const RawMessage &rawMsg = _messageQueue.at(i);
            if (bufferInfoCache.contains(rawMsg.networkId) && bufferInfoCache[rawMsg.networkId].contains(rawMsg.target)) {
                bufferInfo = bufferInfoCache[rawMsg.networkId][rawMsg.target];
            }
            else {
                bool createBuffer = !(rawMsg.flags & Message::Redirected);
                bufferInfo = Core::bufferInfo(user(), rawMsg.networkId, rawMsg.bufferType, rawMsg.target, createBuffer);
                if (!bufferInfo.isValid()) {
                    Q_ASSERT(!createBuffer);
                    redirectedMessages << rawMsg;
                    continue;
                }
                bufferInfoCache[rawMsg.networkId][rawMsg.target] = bufferInfo;
            }
            Message msg(bufferInfo, rawMsg.type, rawMsg.text, rawMsg.sender, senderPrefixes(rawMsg.sender, bufferInfo),
                        realName(rawMsg.sender, rawMsg.networkId),  avatarUrl(rawMsg.sender, rawMsg.networkId),
                        rawMsg.flags);
            messages << msg;
        }

        // recheck if there exists a buffer to store a redirected message in
        for (int i = 0; i < redirectedMessages.count(); i++) {
            const RawMessage &rawMsg = redirectedMessages.at(i);
            if (bufferInfoCache.contains(rawMsg.networkId) && bufferInfoCache[rawMsg.networkId].contains(rawMsg.target)) {
                bufferInfo = bufferInfoCache[rawMsg.networkId][rawMsg.target];
            }
            else {
                // no luck -> we store them in the StatusBuffer
                bufferInfo = Core::bufferInfo(user(), rawMsg.networkId, BufferInfo::StatusBuffer, "");
                // add the StatusBuffer to the Cache in case there are more Messages for the original target
                bufferInfoCache[rawMsg.networkId][rawMsg.target] = bufferInfo;
            }
            Message msg(bufferInfo, rawMsg.type, rawMsg.text, rawMsg.sender, senderPrefixes(rawMsg.sender, bufferInfo),
                        realName(rawMsg.sender, rawMsg.networkId),  avatarUrl(rawMsg.sender, rawMsg.networkId),
                        rawMsg.flags);
            messages << msg;
        }

        if(Core::storeMessages(messages)) {
            // FIXME: extend protocol to a displayMessages(MessageList)
            for (int i = 0; i < messages.count(); i++) {
                emit displayMsg(messages[i]);
            }
        }
    }
    _processMessages = false;
    _messageQueue.clear();
}

QString CoreSession::senderPrefixes(const QString &sender, const BufferInfo &bufferInfo) const
{
    CoreNetwork *currentNetwork = network(bufferInfo.networkId());
    if (!currentNetwork) {
        return {};
    }

    if (bufferInfo.type() != BufferInfo::ChannelBuffer) {
        return {};
    }

    IrcChannel *currentChannel = currentNetwork->ircChannel(bufferInfo.bufferName());
    if (!currentChannel) {
        return {};
    }

    const QString modes = currentChannel->userModes(nickFromMask(sender).toLower());
    return currentNetwork->modesToPrefixes(modes);
}

QString CoreSession::realName(const QString &sender, NetworkId networkId) const
{
    CoreNetwork *currentNetwork = network(networkId);
    if (!currentNetwork) {
        return {};
    }

    IrcUser *currentUser = currentNetwork->ircUser(nickFromMask(sender));
    if (!currentUser) {
        return {};
    }

    return currentUser->realName();
}

QString CoreSession::avatarUrl(const QString &sender, NetworkId networkId) const
{
    Q_UNUSED(sender);
    Q_UNUSED(networkId);
    // Currently we do not have a way to retrieve this value yet.
    //
    // This likely will require implementing IRCv3's METADATA spec.
    // See https://ircv3.net/irc/
    // And https://blog.irccloud.com/avatars/
    return "";
}

Protocol::SessionState CoreSession::sessionState() const
{
    QVariantList bufferInfos;
    QVariantList networkIds;
    QVariantList identities;

    foreach(const BufferInfo &id, buffers())
        bufferInfos << QVariant::fromValue(id);
    foreach(const NetworkId &id, _networks.keys())
        networkIds << QVariant::fromValue(id);
    foreach(const Identity *i, _identities.values())
        identities << QVariant::fromValue(*i);

    return Protocol::SessionState(identities, bufferInfos, networkIds);
}


void CoreSession::initScriptEngine()
{
    signalProxy()->attachSlot(SIGNAL(scriptRequest(QString)), this, SLOT(scriptRequest(QString)));
    signalProxy()->attachSignal(this, SIGNAL(scriptResult(QString)));

    // FIXME
    //QScriptValue storage_ = scriptEngine->newQObject(storage);
    //scriptEngine->globalObject().setProperty("storage", storage_);
}


void CoreSession::scriptRequest(QString script)
{
    emit scriptResult(scriptEngine->evaluate(script).toString());
}


/*** Identity Handling ***/
void CoreSession::createIdentity(const Identity &identity, const QVariantMap &additional)
{
#ifndef HAVE_SSL
    Q_UNUSED(additional)
#endif

    CoreIdentity coreIdentity(identity);
#ifdef HAVE_SSL
    if (additional.contains("KeyPem"))
        coreIdentity.setSslKey(additional["KeyPem"].toByteArray());
    if (additional.contains("CertPem"))
        coreIdentity.setSslCert(additional["CertPem"].toByteArray());
#endif
    qDebug() << Q_FUNC_INFO;
    IdentityId id = Core::createIdentity(user(), coreIdentity);
    if (!id.isValid())
        return;
    else
        createIdentity(coreIdentity);
}

const QString CoreSession::strictCompliantIdent(const CoreIdentity *identity) {
    if (_strictIdentEnabled) {
        // Strict mode enabled: only allow the user's Quassel username as an ident
        return Core::instance()->strictSysIdent(_user);
    } else {
        // Strict mode disabled: allow any identity specified
        return identity->ident();
    }
}

void CoreSession::createIdentity(const CoreIdentity &identity)
{
    CoreIdentity *coreIdentity = new CoreIdentity(identity, this);
    _identities[identity.id()] = coreIdentity;
    // CoreIdentity has its own synchronize method since its "private" sslManager needs to be synced as well
    coreIdentity->synchronize(signalProxy());
    connect(coreIdentity, SIGNAL(updated()), this, SLOT(updateIdentityBySender()));
    emit identityCreated(*coreIdentity);
}


void CoreSession::updateIdentityBySender()
{
    CoreIdentity *identity = qobject_cast<CoreIdentity *>(sender());
    if (!identity)
        return;
    Core::updateIdentity(user(), *identity);
}


void CoreSession::removeIdentity(IdentityId id)
{
    CoreIdentity *identity = _identities.take(id);
    if (identity) {
        emit identityRemoved(id);
        Core::removeIdentity(user(), id);
        identity->deleteLater();
    }
}


/*** Network Handling ***/

void CoreSession::createNetwork(const NetworkInfo &info_, const QStringList &persistentChans)
{
    NetworkInfo info = info_;
    int id;

    if (!info.networkId.isValid())
        Core::createNetwork(user(), info);

    if (!info.networkId.isValid()) {
        qWarning() << qPrintable(tr("CoreSession::createNetwork(): Got invalid networkId from Core when trying to create network %1!").arg(info.networkName));
        return;
    }

    id = info.networkId.toInt();
    if (!_networks.contains(id)) {
        // create persistent chans
        QRegExp rx("\\s*(\\S+)(?:\\s*(\\S+))?\\s*");
        foreach(QString channel, persistentChans) {
            if (!rx.exactMatch(channel)) {
                qWarning() << QString("Invalid persistent channel declaration: %1").arg(channel);
                continue;
            }
            Core::bufferInfo(user(), info.networkId, BufferInfo::ChannelBuffer, rx.cap(1), true);
            Core::setChannelPersistent(user(), info.networkId, rx.cap(1), true);
            if (!rx.cap(2).isEmpty())
                Core::setPersistentChannelKey(user(), info.networkId, rx.cap(1), rx.cap(2));
        }

        CoreNetwork *net = new CoreNetwork(id, this);
        connect(net, SIGNAL(displayMsg(NetworkId, Message::Type, BufferInfo::Type, const QString &, const QString &, const QString &, Message::Flags)),
            SLOT(recvMessageFromServer(NetworkId, Message::Type, BufferInfo::Type, const QString &, const QString &, const QString &, Message::Flags)));
        connect(net, SIGNAL(displayStatusMsg(QString)), SLOT(recvStatusMsgFromServer(QString)));
        connect(net, SIGNAL(disconnected(NetworkId)), SIGNAL(networkDisconnected(NetworkId)));

        net->setNetworkInfo(info);
        net->setProxy(signalProxy());
        _networks[id] = net;
        signalProxy()->synchronize(net);
        emit networkCreated(id);
    }
    else {
        qWarning() << qPrintable(tr("CoreSession::createNetwork(): Trying to create a network that already exists, updating instead!"));
        _networks[info.networkId]->requestSetNetworkInfo(info);
    }
}


void CoreSession::removeNetwork(NetworkId id)
{
    // Make sure the network is disconnected!
    CoreNetwork *net = network(id);
    if (!net)
        return;

    if (net->connectionState() != Network::Disconnected) {
        // make sure we no longer receive data from the tcp buffer
        disconnect(net, SIGNAL(displayMsg(NetworkId, Message::Type, BufferInfo::Type, const QString &, const QString &, const QString &, Message::Flags)), this, 0);
        disconnect(net, SIGNAL(displayStatusMsg(QString)), this, 0);
        connect(net, SIGNAL(disconnected(NetworkId)), this, SLOT(destroyNetwork(NetworkId)));
        net->disconnectFromIrc();
    }
    else {
        destroyNetwork(id);
    }
}


void CoreSession::destroyNetwork(NetworkId id)
{
    QList<BufferId> removedBuffers = Core::requestBufferIdsForNetwork(user(), id);
    Network *net = _networks.take(id);
    if (net && Core::removeNetwork(user(), id)) {
        // make sure that all unprocessed RawMessages from this network are removed
        QList<RawMessage>::iterator messageIter = _messageQueue.begin();
        while (messageIter != _messageQueue.end()) {
            if (messageIter->networkId == id) {
                messageIter = _messageQueue.erase(messageIter);
            }
            else {
                ++messageIter;
            }
        }
        // remove buffers from syncer
        foreach(BufferId bufferId, removedBuffers) {
            _bufferSyncer->removeBuffer(bufferId);
        }
        emit networkRemoved(id);
        net->deleteLater();
    }
}


void CoreSession::renameBuffer(const NetworkId &networkId, const QString &newName, const QString &oldName)
{
    BufferInfo bufferInfo = Core::bufferInfo(user(), networkId, BufferInfo::QueryBuffer, oldName, false);
    if (bufferInfo.isValid()) {
        _bufferSyncer->renameBuffer(bufferInfo.bufferId(), newName);
    }
}


void CoreSession::clientsConnected()
{
    QHash<NetworkId, CoreNetwork *>::iterator netIter = _networks.begin();
    Identity *identity = 0;
    CoreNetwork *net = 0;
    IrcUser *me = 0;
    while (netIter != _networks.end()) {
        net = *netIter;
        ++netIter;

        if (!net->isConnected())
            continue;
        identity = net->identityPtr();
        if (!identity)
            continue;
        me = net->me();
        if (!me)
            continue;

        if (identity->detachAwayEnabled() && me->isAway()) {
            net->userInputHandler()->handleAway(BufferInfo(), QString());
        }
    }
}


void CoreSession::clientsDisconnected()
{
    QHash<NetworkId, CoreNetwork *>::iterator netIter = _networks.begin();
    Identity *identity = 0;
    CoreNetwork *net = 0;
    IrcUser *me = 0;
    QString awayReason;
    while (netIter != _networks.end()) {
        net = *netIter;
        ++netIter;

        if (!net->isConnected())
            continue;

        identity = net->identityPtr();
        if (!identity)
            continue;
        me = net->me();
        if (!me)
            continue;

        if (identity->detachAwayEnabled() && !me->isAway()) {
            if (!identity->detachAwayReason().isEmpty())
                awayReason = identity->detachAwayReason();
            net->setAutoAwayActive(true);
            // Allow handleAway() to format the current date/time in the string.
            net->userInputHandler()->handleAway(BufferInfo(), awayReason);
        }
    }
}


void CoreSession::globalAway(const QString &msg, const bool skipFormatting)
{
    QHash<NetworkId, CoreNetwork *>::iterator netIter = _networks.begin();
    CoreNetwork *net = 0;
    while (netIter != _networks.end()) {
        net = *netIter;
        ++netIter;

        if (!net->isConnected())
            continue;

        net->userInputHandler()->issueAway(msg, false /* no force away */, skipFormatting);
    }
}

void CoreSession::changePassword(PeerPtr peer, const QString &userName, const QString &oldPassword, const QString &newPassword) {
    Q_UNUSED(peer);

    bool success = false;
    UserId uid = Core::validateUser(userName, oldPassword);
    if (uid.isValid() && uid == user())
        success = Core::changeUserPassword(uid, newPassword);

    signalProxy()->restrictTargetPeers(signalProxy()->sourcePeer(), [&]{
        emit passwordChanged(nullptr, success);
    });
}

void CoreSession::kickClient(int peerId) {
    auto peer = signalProxy()->peerById(peerId);
    if (peer == nullptr) {
        qWarning() << "Invalid peer Id: " << peerId;
        return;
    }
    signalProxy()->restrictTargetPeers(peer, [&]{
        emit disconnectFromCore();
    });
}

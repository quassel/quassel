/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "coreconnection.h"

#include "client.h"
#include "clientauthhandler.h"
#include "clientsettings.h"
#include "coreaccountmodel.h"
#include "identity.h"
#include "internalpeer.h"
#include "network.h"
#include "networkmodel.h"
#include "quassel.h"
#include "signalproxy.h"
#include "util.h"

#include "protocols/legacy/legacypeer.h"

CoreConnection::CoreConnection(QObject* parent)
    : QObject(parent)
    , _authHandler(nullptr)
{
    qRegisterMetaType<ConnectionState>("CoreConnection::ConnectionState");
}

void CoreConnection::init()
{
    Client::signalProxy()->setHeartBeatInterval(30);
    connect(Client::signalProxy(), &SignalProxy::lagUpdated, this, &CoreConnection::lagUpdated);

    _reconnectTimer.setSingleShot(true);
    connect(&_reconnectTimer, &QTimer::timeout, this, &CoreConnection::reconnectTimeout);

    _qNetworkConfigurationManager = new QNetworkConfigurationManager(this);
    connect(_qNetworkConfigurationManager.data(), &QNetworkConfigurationManager::onlineStateChanged, this, &CoreConnection::onlineStateChanged);

    CoreConnectionSettings s;
    s.initAndNotify("PingTimeoutInterval", this, &CoreConnection::pingTimeoutIntervalChanged, 60);
    s.initAndNotify("ReconnectInterval", this, &CoreConnection::reconnectIntervalChanged, 60);
    s.notify("NetworkDetectionMode", this, &CoreConnection::networkDetectionModeChanged);
    networkDetectionModeChanged(s.networkDetectionMode());
}

CoreAccountModel* CoreConnection::accountModel() const
{
    return Client::coreAccountModel();
}

void CoreConnection::setProgressText(const QString& text)
{
    if (_progressText != text) {
        _progressText = text;
        emit progressTextChanged(text);
    }
}

void CoreConnection::setProgressValue(int value)
{
    if (_progressValue != value) {
        _progressValue = value;
        emit progressValueChanged(value);
    }
}

void CoreConnection::setProgressMinimum(int minimum)
{
    if (_progressMinimum != minimum) {
        _progressMinimum = minimum;
        emit progressRangeChanged(minimum, _progressMaximum);
    }
}

void CoreConnection::setProgressMaximum(int maximum)
{
    if (_progressMaximum != maximum) {
        _progressMaximum = maximum;
        emit progressRangeChanged(_progressMinimum, maximum);
    }
}

void CoreConnection::updateProgress(int value, int max)
{
    if (max != _progressMaximum) {
        _progressMaximum = max;
        emit progressRangeChanged(_progressMinimum, _progressMaximum);
    }
    setProgressValue(value);
}

void CoreConnection::reconnectTimeout()
{
    if (!_peer) {
        CoreConnectionSettings s;
        if (_wantReconnect && s.autoReconnect()) {
            // If using QNetworkConfigurationManager, we don't want to reconnect if we're offline
            if (s.networkDetectionMode() == CoreConnectionSettings::UseQNetworkConfigurationManager) {
                if (!_qNetworkConfigurationManager->isOnline()) {
                    return;
                }
            }
            reconnectToCore();
        }
    }
}

void CoreConnection::networkDetectionModeChanged(const QVariant& vmode)
{
    CoreConnectionSettings s;
    auto mode = (CoreConnectionSettings::NetworkDetectionMode)vmode.toInt();
    if (mode == CoreConnectionSettings::UsePingTimeout)
        Client::signalProxy()->setMaxHeartBeatCount(s.pingTimeoutInterval() / 30);
    else {
        Client::signalProxy()->setMaxHeartBeatCount(-1);
    }
}

void CoreConnection::pingTimeoutIntervalChanged(const QVariant& interval)
{
    CoreConnectionSettings s;
    if (s.networkDetectionMode() == CoreConnectionSettings::UsePingTimeout)
        Client::signalProxy()->setMaxHeartBeatCount(interval.toInt() / 30);  // interval is 30 seconds
}

void CoreConnection::reconnectIntervalChanged(const QVariant& interval)
{
    _reconnectTimer.setInterval(interval.toInt() * 1000);
}

void CoreConnection::onlineStateChanged(bool isOnline)
{
    CoreConnectionSettings s;
    if (s.networkDetectionMode() != CoreConnectionSettings::UseQNetworkConfigurationManager)
        return;

    if (isOnline) {
        // qDebug() << "QNetworkConfigurationManager reports Online";
        if (state() == Disconnected) {
            if (_wantReconnect && s.autoReconnect()) {
                reconnectToCore();
            }
        }
    }
    else {
        // qDebug() << "QNetworkConfigurationManager reports Offline";
        if (state() != Disconnected && !isLocalConnection())
            disconnectFromCore(tr("Network is down"), true);
    }
}

QPointer<Peer> CoreConnection::peer() const
{
    if (_peer) {
        return _peer;
    }
    return _authHandler ? _authHandler->peer() : nullptr;
}

bool CoreConnection::isEncrypted() const
{
    return _peer && _peer->isSecure();
}

bool CoreConnection::isLocalConnection() const
{
    if (!isConnected())
        return false;
    if (currentAccount().isInternal())
        return true;
    if (_authHandler)
        return _authHandler->isLocal();
    if (_peer)
        return _peer->isLocal();

    return false;
}

void CoreConnection::onConnectionReady()
{
    setState(Connected);
}

void CoreConnection::setState(ConnectionState state)
{
    if (state != _state) {
        _state = state;
        emit stateChanged(state);
        if (state == Connected)
            _wantReconnect = true;
        if (state == Disconnected)
            emit disconnected();
    }
}

void CoreConnection::coreSocketError(QAbstractSocket::SocketError error, const QString& errorString)
{
    Q_UNUSED(error)

    disconnectFromCore(errorString, true);
}

void CoreConnection::coreSocketDisconnected()
{
    setState(Disconnected);
    _wasReconnect = false;
    resetConnection(_wantReconnect);
}

void CoreConnection::disconnectFromCore()
{
    disconnectFromCore(QString(), false);  // requested disconnect, so don't try to reconnect
}

void CoreConnection::disconnectFromCore(const QString& errorString, bool wantReconnect)
{
    if (wantReconnect)
        _reconnectTimer.start();
    else
        _reconnectTimer.stop();

    _wantReconnect = wantReconnect;  // store if disconnect was requested
    _wasReconnect = false;

    if (_authHandler)
        _authHandler->close();
    else if (_peer)
        _peer->close();

    if (errorString.isEmpty())
        emit connectionError(tr("Disconnected"));
    else
        emit connectionError(errorString);
}

void CoreConnection::resetConnection(bool wantReconnect)
{
    if (_resetting)
        return;
    _resetting = true;

    _wantReconnect = wantReconnect;

    if (_authHandler) {
        disconnect(_authHandler, nullptr, this, nullptr);
        _authHandler->close();
        _authHandler->deleteLater();
        _authHandler = nullptr;
    }

    if (_peer) {
        disconnect(_peer, nullptr, this, nullptr);
        // peer belongs to the sigproxy and thus gets deleted by it
        _peer->close();
        _peer = nullptr;
    }

    _netsToSync.clear();
    _numNetsToSync = 0;

    setProgressMaximum(-1);  // disable
    setState(Disconnected);
    emit lagUpdated(-1);

    emit connectionMsg(tr("Disconnected from core."));
    emit encrypted(false);
    setState(Disconnected);

    // initiate if a reconnect if appropriate
    CoreConnectionSettings s;
    if (wantReconnect && s.autoReconnect()) {
        _reconnectTimer.start();
    }

    _resetting = false;
}

void CoreConnection::reconnectToCore()
{
    if (currentAccount().isValid()) {
        _wasReconnect = true;
        connectToCore(currentAccount().accountId());
    }
}

bool CoreConnection::connectToCore(AccountId accId)
{
    if (isConnected())
        return false;

    CoreAccountSettings s;

    // FIXME: Don't force connection to internal core in mono client
    if (Quassel::runMode() == Quassel::Monolithic) {
        _account = accountModel()->account(accountModel()->internalAccount());
        Q_ASSERT(_account.isValid());
    }
    else {
        if (!accId.isValid()) {
            // check our settings and figure out what to do
            if (!s.autoConnectOnStartup())
                return false;
            if (s.autoConnectToFixedAccount())
                accId = s.autoConnectAccount();
            else
                accId = s.lastAccount();
            if (!accId.isValid())
                return false;
        }
        _account = accountModel()->account(accId);
        if (!_account.accountId().isValid()) {
            return false;
        }
        if (Quassel::runMode() != Quassel::Monolithic) {
            if (_account.isInternal())
                return false;
        }
    }

    s.setLastAccount(accId);
    connectToCurrentAccount();
    return true;
}

void CoreConnection::connectToCurrentAccount()
{
    if (_authHandler) {
        qWarning() << Q_FUNC_INFO << "Already connected!";
        return;
    }

    if (currentAccount().isInternal()) {
        if (Quassel::runMode() != Quassel::Monolithic) {
            qWarning() << "Cannot connect to internal core in client-only mode!";
            return;
        }

        auto* peer = new InternalPeer();
        _peer = peer;
        Client::instance()->signalProxy()->addPeer(peer);  // sigproxy will take ownership
        emit connectionMsg(tr("Initializing..."));
        emit connectToInternalCore(peer);
        setState(Connected);
        return;
    }

    _authHandler = new ClientAuthHandler(currentAccount(), this);

    connect(_authHandler, &ClientAuthHandler::disconnected, this, &CoreConnection::coreSocketDisconnected);
    connect(_authHandler, &ClientAuthHandler::connectionReady, this, &CoreConnection::onConnectionReady);
    connect(_authHandler, &ClientAuthHandler::socketError, this, &CoreConnection::coreSocketError);
    connect(_authHandler, &ClientAuthHandler::transferProgress, this, &CoreConnection::updateProgress);
    connect(_authHandler,
            &ClientAuthHandler::requestDisconnect,
            this,
            selectOverload<const QString&, bool>(&CoreConnection::disconnectFromCore));

    connect(_authHandler, &ClientAuthHandler::errorMessage, this, &CoreConnection::connectionError);
    connect(_authHandler, &ClientAuthHandler::errorPopup, this, &CoreConnection::connectionErrorPopup, Qt::QueuedConnection);
    connect(_authHandler, &ClientAuthHandler::statusMessage, this, &CoreConnection::connectionMsg);
    connect(_authHandler, &ClientAuthHandler::encrypted, this, &CoreConnection::encrypted);
    connect(_authHandler, &ClientAuthHandler::startCoreSetup, this, &CoreConnection::startCoreSetup);
    connect(_authHandler, &ClientAuthHandler::coreSetupFailed, this, &CoreConnection::coreSetupFailed);
    connect(_authHandler, &ClientAuthHandler::coreSetupSuccessful, this, &CoreConnection::coreSetupSuccess);
    connect(_authHandler, &ClientAuthHandler::userAuthenticationRequired, this, &CoreConnection::userAuthenticationRequired);
    connect(_authHandler, &ClientAuthHandler::handleNoSslInClient, this, &CoreConnection::handleNoSslInClient);
    connect(_authHandler, &ClientAuthHandler::handleNoSslInCore, this, &CoreConnection::handleNoSslInCore);
    connect(_authHandler, &ClientAuthHandler::handleSslErrors, this, &CoreConnection::handleSslErrors);
    connect(_authHandler, &ClientAuthHandler::loginSuccessful, this, &CoreConnection::onLoginSuccessful);
    connect(_authHandler, &ClientAuthHandler::handshakeComplete, this, &CoreConnection::onHandshakeComplete);

    setState(Connecting);
    _authHandler->connectToCore();
}

void CoreConnection::setupCore(const Protocol::SetupData& setupData)
{
    _authHandler->setupCore(setupData);
}

void CoreConnection::loginToCore(const QString& user, const QString& password, bool remember)
{
    _authHandler->login(user, password, remember);
}

void CoreConnection::onLoginSuccessful(const CoreAccount& account)
{
    updateProgress(0, 0);

    // save current account data
    accountModel()->createOrUpdateAccount(account);
    accountModel()->save();

    _reconnectTimer.stop();

    setProgressText(tr("Receiving session state"));
    setState(Synchronizing);
    emit connectionMsg(tr("Synchronizing to %1...").arg(account.accountName()));
}

void CoreConnection::onHandshakeComplete(RemotePeer* peer, const Protocol::SessionState& sessionState)
{
    updateProgress(100, 100);

    disconnect(_authHandler, nullptr, this, nullptr);
    _authHandler->deleteLater();
    _authHandler = nullptr;

    _peer = peer;
    connect(peer, &Peer::disconnected, this, &CoreConnection::coreSocketDisconnected);
    connect(peer, &RemotePeer::statusMessage, this, &CoreConnection::connectionMsg);
    connect(peer, &RemotePeer::socketError, this, &CoreConnection::coreSocketError);

    Client::signalProxy()->addPeer(_peer);  // sigproxy takes ownership of the peer!

    syncToCore(sessionState);
}

void CoreConnection::internalSessionStateReceived(const Protocol::SessionState& sessionState)
{
    updateProgress(100, 100);
    setState(Synchronizing);
    syncToCore(sessionState);
}

void CoreConnection::syncToCore(const Protocol::SessionState& sessionState)
{
    setProgressText(tr("Receiving network states"));
    updateProgress(0, 100);

    // create identities
    foreach (const QVariant& vid, sessionState.identities) {
        Client::instance()->coreIdentityCreated(vid.value<Identity>());
    }

    // create buffers
    // FIXME: get rid of this crap -- why?
    NetworkModel* networkModel = Client::networkModel();
    Q_ASSERT(networkModel);
    foreach (const QVariant& vinfo, sessionState.bufferInfos)
        networkModel->bufferUpdated(vinfo.value<BufferInfo>());  // create BufferItems

    // prepare sync progress thingys...
    // FIXME: Care about removal of networks
    _numNetsToSync = sessionState.networkIds.count();
    updateProgress(0, _numNetsToSync);

    // create network objects
    foreach (const QVariant& networkid, sessionState.networkIds) {
        NetworkId netid = networkid.value<NetworkId>();
        if (Client::network(netid))
            continue;
        auto* net = new Network(netid, Client::instance());
        _netsToSync.insert(net);
        connect(net, &SyncableObject::initDone, this, &CoreConnection::networkInitDone);
        connect(net, &QObject::destroyed, this, &CoreConnection::networkInitDone);
        Client::addNetwork(net);
    }
    checkSyncState();
}

// this is also called for destroyed networks!
void CoreConnection::networkInitDone()
{
    QObject* net = sender();
    Q_ASSERT(net);
    disconnect(net, nullptr, this, nullptr);
    _netsToSync.remove(net);
    updateProgress(_numNetsToSync - _netsToSync.count(), _numNetsToSync);
    checkSyncState();
}

void CoreConnection::checkSyncState()
{
    if (_netsToSync.isEmpty() && state() >= Synchronizing) {
        setState(Synchronized);
        setProgressText(tr("Synchronized to %1").arg(currentAccount().accountName()));
        setProgressMaximum(-1);
        emit synchronized();
    }
}

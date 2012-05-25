/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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

#ifndef QT_NO_NETWORKPROXY
#  include <QNetworkProxy>
#endif

#include "client.h"
#include "clientsettings.h"
#include "coreaccountmodel.h"
#include "identity.h"
#include "network.h"
#include "networkmodel.h"
#include "quassel.h"
#include "signalproxy.h"
#include "util.h"

CoreConnection::CoreConnection(CoreAccountModel *model, QObject *parent)
    : QObject(parent),
    _model(model),
    _blockSize(0),
    _state(Disconnected),
    _wantReconnect(false),
    _progressMinimum(0),
    _progressMaximum(-1),
    _progressValue(-1),
    _wasReconnect(false),
    _requestedDisconnect(false)
{
    qRegisterMetaType<ConnectionState>("CoreConnection::ConnectionState");
}


void CoreConnection::init()
{
    Client::signalProxy()->setHeartBeatInterval(30);
    connect(Client::signalProxy(), SIGNAL(disconnected()), SLOT(coreSocketDisconnected()));
    connect(Client::signalProxy(), SIGNAL(lagUpdated(int)), SIGNAL(lagUpdated(int)));

    _reconnectTimer.setSingleShot(true);
    connect(&_reconnectTimer, SIGNAL(timeout()), SLOT(reconnectTimeout()));

#ifdef HAVE_KDE
    connect(Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
        SLOT(solidNetworkStatusChanged(Solid::Networking::Status)));
#endif

    CoreConnectionSettings s;
    s.initAndNotify("PingTimeoutInterval", this, SLOT(pingTimeoutIntervalChanged(QVariant)), 60);
    s.initAndNotify("ReconnectInterval", this, SLOT(reconnectIntervalChanged(QVariant)), 60);
    s.notify("NetworkDetectionMode", this, SLOT(networkDetectionModeChanged(QVariant)));
    networkDetectionModeChanged(s.networkDetectionMode());
}


void CoreConnection::setProgressText(const QString &text)
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
    if (!_socket) {
        CoreConnectionSettings s;
        if (_wantReconnect && s.autoReconnect()) {
#ifdef HAVE_KDE
            // If using Solid, we don't want to reconnect if we're offline
            if (s.networkDetectionMode() == CoreConnectionSettings::UseSolid) {
                if (Solid::Networking::status() != Solid::Networking::Connected
                    && Solid::Networking::status() != Solid::Networking::Unknown) {
                    return;
                }
            }
#endif /* HAVE_KDE */

            reconnectToCore();
        }
    }
}


void CoreConnection::networkDetectionModeChanged(const QVariant &vmode)
{
    CoreConnectionSettings s;
    CoreConnectionSettings::NetworkDetectionMode mode = (CoreConnectionSettings::NetworkDetectionMode)vmode.toInt();
    if (mode == CoreConnectionSettings::UsePingTimeout)
        Client::signalProxy()->setMaxHeartBeatCount(s.pingTimeoutInterval() / 30);
    else {
        Client::signalProxy()->setMaxHeartBeatCount(-1);
    }
}


void CoreConnection::pingTimeoutIntervalChanged(const QVariant &interval)
{
    CoreConnectionSettings s;
    if (s.networkDetectionMode() == CoreConnectionSettings::UsePingTimeout)
        Client::signalProxy()->setMaxHeartBeatCount(interval.toInt() / 30);  // interval is 30 seconds
}


void CoreConnection::reconnectIntervalChanged(const QVariant &interval)
{
    _reconnectTimer.setInterval(interval.toInt() * 1000);
}


#ifdef HAVE_KDE

void CoreConnection::solidNetworkStatusChanged(Solid::Networking::Status status)
{
    CoreConnectionSettings s;
    if (s.networkDetectionMode() != CoreConnectionSettings::UseSolid)
        return;

    switch (status) {
    case Solid::Networking::Unknown:
    case Solid::Networking::Connected:
        //qDebug() << "Solid: Network status changed to connected or unknown";
        if (state() == Disconnected) {
            if (_wantReconnect && s.autoReconnect()) {
                reconnectToCore();
            }
        }
        break;
    case Solid::Networking::Disconnecting:
    case Solid::Networking::Unconnected:
        if (state() != Disconnected && !isLocalConnection())
            disconnectFromCore(tr("Network is down"), true);
        break;
    default:
        break;
    }
}


#endif

bool CoreConnection::isEncrypted() const
{
#ifndef HAVE_SSL
    return false;
#else
    QSslSocket *sock = qobject_cast<QSslSocket *>(_socket);
    return isConnected() && sock && sock->isEncrypted();
#endif
}


bool CoreConnection::isLocalConnection() const
{
    if (!isConnected())
        return false;
    if (currentAccount().isInternal())
        return true;
    if (_socket->peerAddress().isInSubnet(QHostAddress::LocalHost, 0x00ffffff))
        return true;

    return false;
}


void CoreConnection::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    QString text;

    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        text = tr("Disconnected");
        break;
    case QAbstractSocket::HostLookupState:
        text = tr("Looking up %1...").arg(currentAccount().hostName());
        break;
    case QAbstractSocket::ConnectingState:
        text = tr("Connecting to %1...").arg(currentAccount().hostName());
        break;
    case QAbstractSocket::ConnectedState:
        text = tr("Connected to %1").arg(currentAccount().hostName());
        break;
    case QAbstractSocket::ClosingState:
        text = tr("Disconnecting from %1...").arg(currentAccount().hostName());
        break;
    default:
        break;
    }

    if (!text.isEmpty())
        emit progressTextChanged(text);

    setState(socketState);
}


void CoreConnection::setState(QAbstractSocket::SocketState socketState)
{
    ConnectionState state;

    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        state = Disconnected;
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
    case QAbstractSocket::ConnectedState: // we'll set it to Connected in connectionReady()
        state = Connecting;
        break;
    default:
        state = Disconnected;
    }

    setState(state);
}


void CoreConnection::setState(ConnectionState state)
{
    if (state != _state) {
        _state = state;
        emit stateChanged(state);
        if (state == Disconnected)
            emit disconnected();
    }
}


void CoreConnection::coreSocketError(QAbstractSocket::SocketError)
{
    qDebug() << "coreSocketError" << _socket << _socket->errorString();
    disconnectFromCore(_socket->errorString(), true);
}


void CoreConnection::coreSocketDisconnected()
{
    // qDebug() << Q_FUNC_INFO;
    _wasReconnect = !_requestedDisconnect;
    resetConnection(true);
    // FIXME handle disconnects gracefully
}


void CoreConnection::coreHasData()
{
    QVariant item;
    while (SignalProxy::readDataFromDevice(_socket, _blockSize, item)) {
        QVariantMap msg = item.toMap();
        if (!msg.contains("MsgType")) {
            // This core is way too old and does not even speak our init protocol...
            emit connectionErrorPopup(tr("The Quassel Core you try to connect to is too old! Please consider upgrading."));
            disconnectFromCore(QString(), false);
            return;
        }
        if (msg["MsgType"] == "ClientInitAck") {
            clientInitAck(msg);
        }
        else if (msg["MsgType"] == "ClientInitReject") {
            emit connectionErrorPopup(msg["Error"].toString());
            disconnectFromCore(QString(), false);
            return;
        }
        else if (msg["MsgType"] == "CoreSetupAck") {
            emit coreSetupSuccess();
        }
        else if (msg["MsgType"] == "CoreSetupReject") {
            emit coreSetupFailed(msg["Error"].toString());
        }
        else if (msg["MsgType"] == "ClientLoginReject") {
            loginFailed(msg["Error"].toString());
        }
        else if (msg["MsgType"] == "ClientLoginAck") {
            loginSuccess();
        }
        else if (msg["MsgType"] == "SessionInit") {
            // that's it, let's hand over to the signal proxy
            // if the socket is an orphan, the signalProxy adopts it.
            // -> we don't need to care about it anymore
            _socket->setParent(0);
            Client::signalProxy()->addPeer(_socket);

            sessionStateReceived(msg["SessionState"].toMap());
            break; // this is definitively the last message we process here!
        }
        else {
            disconnectFromCore(tr("Invalid data received from core"), false);
            return;
        }
    }
    if (_blockSize > 0) {
        updateProgress(_socket->bytesAvailable(), _blockSize);
    }
}


void CoreConnection::disconnectFromCore()
{
    _requestedDisconnect = true;
    disconnectFromCore(QString(), false); // requested disconnect, so don't try to reconnect
}


void CoreConnection::disconnectFromCore(const QString &errorString, bool wantReconnect)
{
    if (!wantReconnect)
        _reconnectTimer.stop();

    _wasReconnect = wantReconnect; // store if disconnect was requested

    if (errorString.isEmpty())
        emit connectionError(tr("Disconnected"));
    else
        emit connectionError(errorString);

    Client::signalProxy()->removeAllPeers();
    resetConnection(wantReconnect);
}


void CoreConnection::resetConnection(bool wantReconnect)
{
    _wantReconnect = wantReconnect;

    if (_socket) {
        disconnect(_socket, 0, this, 0);
        _socket->deleteLater();
        _socket = 0;
    }
    _requestedDisconnect = false;
    _blockSize = 0;

    _coreMsgBuffer.clear();

    _netsToSync.clear();
    _numNetsToSync = 0;

    setProgressMaximum(-1); // disable
    setState(Disconnected);
    emit lagUpdated(-1);

    emit connectionMsg(tr("Disconnected from core."));
    emit encrypted(false);

    // initiate if a reconnect if appropriate
    CoreConnectionSettings s;
    if (wantReconnect && s.autoReconnect()) {
        _reconnectTimer.start();
    }
}


void CoreConnection::reconnectToCore()
{
    if (currentAccount().isValid())
        connectToCore(currentAccount().accountId());
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
    resetConnection(false);

    if (currentAccount().isInternal()) {
        if (Quassel::runMode() != Quassel::Monolithic) {
            qWarning() << "Cannot connect to internal core in client-only mode!";
            return;
        }
        emit startInternalCore();
        emit connectToInternalCore(Client::instance()->signalProxy());
        return;
    }

    CoreAccountSettings s;

    Q_ASSERT(!_socket);
#ifdef HAVE_SSL
    QSslSocket *sock = new QSslSocket(Client::instance());
    // make sure the warning is shown if we happen to connect without SSL support later
    s.setAccountValue("ShowNoClientSslWarning", true);
#else
    if (_account.useSsl()) {
        if (s.accountValue("ShowNoClientSslWarning", true).toBool()) {
            bool accepted = false;
            emit handleNoSslInClient(&accepted);
            if (!accepted) {
                emit connectionError(tr("Unencrypted connection canceled"));
                return;
            }
            s.setAccountValue("ShowNoClientSslWarning", false);
        }
    }
    QTcpSocket *sock = new QTcpSocket(Client::instance());
#endif

#ifndef QT_NO_NETWORKPROXY
    if (_account.useProxy()) {
        QNetworkProxy proxy(_account.proxyType(), _account.proxyHostName(), _account.proxyPort(), _account.proxyUser(), _account.proxyPassword());
        sock->setProxy(proxy);
    }
#endif

    _socket = sock;
    connect(sock, SIGNAL(readyRead()), SLOT(coreHasData()));
    connect(sock, SIGNAL(connected()), SLOT(coreSocketConnected()));
    connect(sock, SIGNAL(disconnected()), SLOT(coreSocketDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(coreSocketError(QAbstractSocket::SocketError)));
    connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(socketStateChanged(QAbstractSocket::SocketState)));

    emit connectionMsg(tr("Connecting to %1...").arg(currentAccount().accountName()));
    sock->connectToHost(_account.hostName(), _account.port());
}


void CoreConnection::coreSocketConnected()
{
    // Phase One: Send client info and wait for core info

    emit connectionMsg(tr("Synchronizing to core..."));

    QVariantMap clientInit;
    clientInit["MsgType"] = "ClientInit";
    clientInit["ClientVersion"] = Quassel::buildInfo().fancyVersionString;
    clientInit["ClientDate"] = Quassel::buildInfo().buildDate;
    clientInit["ProtocolVersion"] = Quassel::buildInfo().protocolVersion;
    clientInit["UseSsl"] = _account.useSsl();
#ifndef QT_NO_COMPRESS
    clientInit["UseCompression"] = true;
#else
    clientInit["UseCompression"] = false;
#endif

    SignalProxy::writeDataToDevice(_socket, clientInit);
}


void CoreConnection::clientInitAck(const QVariantMap &msg)
{
    // Core has accepted our version info and sent its own. Let's see if we accept it as well...
    uint ver = msg["ProtocolVersion"].toUInt();
    if (ver < Quassel::buildInfo().clientNeedsProtocol) {
        emit connectionErrorPopup(tr("<b>The Quassel Core you are trying to connect to is too old!</b><br>"
                                     "Need at least core/client protocol v%1 to connect.").arg(Quassel::buildInfo().clientNeedsProtocol));
        disconnectFromCore(QString(), false);
        return;
    }

    Client::setCoreFeatures((Quassel::Features)msg["CoreFeatures"].toUInt());

#ifndef QT_NO_COMPRESS
    if (msg["SupportsCompression"].toBool()) {
        _socket->setProperty("UseCompression", true);
    }
#endif

    _coreMsgBuffer = msg;

#ifdef HAVE_SSL
    CoreAccountSettings s;
    if (currentAccount().useSsl()) {
        if (msg["SupportSsl"].toBool()) {
            // Make sure the warning is shown next time we don't have SSL in the core
            s.setAccountValue("ShowNoCoreSslWarning", true);

            QSslSocket *sslSocket = qobject_cast<QSslSocket *>(_socket);
            Q_ASSERT(sslSocket);
            connect(sslSocket, SIGNAL(encrypted()), SLOT(sslSocketEncrypted()));
            connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(sslErrors()));
            sslSocket->startClientEncryption();
        }
        else {
            if (s.accountValue("ShowNoCoreSslWarning", true).toBool()) {
                bool accepted = false;
                emit handleNoSslInCore(&accepted);
                if (!accepted) {
                    disconnectFromCore(tr("Unencrypted connection canceled"), false);
                    return;
                }
                s.setAccountValue("ShowNoCoreSslWarning", false);
                s.setAccountValue("SslCert", QString());
            }
            connectionReady();
        }
        return;
    }
#endif
    // if we use SSL we wait for the next step until every SSL warning has been cleared
    connectionReady();
}


#ifdef HAVE_SSL

void CoreConnection::sslSocketEncrypted()
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    Q_ASSERT(socket);

    if (!socket->sslErrors().count()) {
        // Cert is valid, so we don't want to store it as known
        // That way, a warning will appear in case it becomes invalid at some point
        CoreAccountSettings s;
        s.setAccountValue("SSLCert", QString());
    }

    emit encrypted(true);
    connectionReady();
}


void CoreConnection::sslErrors()
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    Q_ASSERT(socket);

    CoreAccountSettings s;
    QByteArray knownDigest = s.accountValue("SslCert").toByteArray();

    if (knownDigest != socket->peerCertificate().digest()) {
        bool accepted = false;
        bool permanently = false;
        emit handleSslErrors(socket, &accepted, &permanently);

        if (!accepted) {
            disconnectFromCore(tr("Unencrypted connection canceled"), false);
            return;
        }

        if (permanently)
            s.setAccountValue("SslCert", socket->peerCertificate().digest());
        else
            s.setAccountValue("SslCert", QString());
    }

    socket->ignoreSslErrors();
}


#endif /* HAVE_SSL */

void CoreConnection::connectionReady()
{
    setState(Connected);
    emit connectionMsg(tr("Connected to %1").arg(currentAccount().accountName()));

    if (!_coreMsgBuffer["Configured"].toBool()) {
        // start wizard
        emit startCoreSetup(_coreMsgBuffer["StorageBackends"].toList());
    }
    else if (_coreMsgBuffer["LoginEnabled"].toBool()) {
        loginToCore();
    }
    _coreMsgBuffer.clear();
}


void CoreConnection::loginToCore(const QString &user, const QString &password, bool remember)
{
    _account.setUser(user);
    _account.setPassword(password);
    _account.setStorePassword(remember);
    loginToCore();
}


void CoreConnection::loginToCore(const QString &prevError)
{
    emit connectionMsg(tr("Logging in..."));
    if (currentAccount().user().isEmpty() || currentAccount().password().isEmpty() || !prevError.isEmpty()) {
        bool valid = false;
        emit userAuthenticationRequired(&_account, &valid, prevError); // *must* be a synchronous call
        if (!valid || currentAccount().user().isEmpty() || currentAccount().password().isEmpty()) {
            disconnectFromCore(tr("Login canceled"), false);
            return;
        }
    }

    QVariantMap clientLogin;
    clientLogin["MsgType"] = "ClientLogin";
    clientLogin["User"] = currentAccount().user();
    clientLogin["Password"] = currentAccount().password();
    SignalProxy::writeDataToDevice(_socket, clientLogin);
}


void CoreConnection::loginFailed(const QString &error)
{
    loginToCore(error);
}


void CoreConnection::loginSuccess()
{
    updateProgress(0, 0);

    // save current account data
    _model->createOrUpdateAccount(currentAccount());
    _model->save();

    _reconnectTimer.stop();

    setProgressText(tr("Receiving session state"));
    setState(Synchronizing);
    emit connectionMsg(tr("Synchronizing to %1...").arg(currentAccount().accountName()));
}


void CoreConnection::sessionStateReceived(const QVariantMap &state)
{
    updateProgress(100, 100);

    // rest of communication happens through SignalProxy...
    disconnect(_socket, SIGNAL(readyRead()), this, 0);
    disconnect(_socket, SIGNAL(connected()), this, 0);

    syncToCore(state);
}


void CoreConnection::internalSessionStateReceived(const QVariant &packedState)
{
    updateProgress(100, 100);

    setState(Synchronizing);
    syncToCore(packedState.toMap());
}


void CoreConnection::syncToCore(const QVariantMap &sessionState)
{
    if (sessionState.contains("CoreFeatures"))
        Client::setCoreFeatures((Quassel::Features)sessionState["CoreFeatures"].toUInt());

    setProgressText(tr("Receiving network states"));
    updateProgress(0, 100);

    // create identities
    foreach(QVariant vid, sessionState["Identities"].toList()) {
        Client::instance()->coreIdentityCreated(vid.value<Identity>());
    }

    // create buffers
    // FIXME: get rid of this crap -- why?
    QVariantList bufferinfos = sessionState["BufferInfos"].toList();
    NetworkModel *networkModel = Client::networkModel();
    Q_ASSERT(networkModel);
    foreach(QVariant vinfo, bufferinfos)
    networkModel->bufferUpdated(vinfo.value<BufferInfo>());  // create BufferItems

    QVariantList networkids = sessionState["NetworkIds"].toList();

    // prepare sync progress thingys...
    // FIXME: Care about removal of networks
    _numNetsToSync = networkids.count();
    updateProgress(0, _numNetsToSync);

    // create network objects
    foreach(QVariant networkid, networkids) {
        NetworkId netid = networkid.value<NetworkId>();
        if (Client::network(netid))
            continue;
        Network *net = new Network(netid, Client::instance());
        _netsToSync.insert(net);
        connect(net, SIGNAL(initDone()), SLOT(networkInitDone()));
        connect(net, SIGNAL(destroyed()), SLOT(networkInitDone()));
        Client::addNetwork(net);
    }
    checkSyncState();
}


// this is also called for destroyed networks!
void CoreConnection::networkInitDone()
{
    QObject *net = sender();
    Q_ASSERT(net);
    disconnect(net, 0, this, 0);
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


void CoreConnection::doCoreSetup(const QVariant &setupData)
{
    QVariantMap setup;
    setup["MsgType"] = "CoreSetupData";
    setup["SetupData"] = setupData;
    SignalProxy::writeDataToDevice(_socket, setup);
}

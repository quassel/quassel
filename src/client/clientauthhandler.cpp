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

#include "clientauthhandler.h"

// TODO: support system application proxy (new in Qt 4.6)

#ifdef HAVE_SSL
    #include <QSslSocket>
#else
    #include <QTcpSocket>
#endif

#include "client.h"
#include "clientsettings.h"

#include "protocols/legacy/legacypeer.h"

using namespace Protocol;

ClientAuthHandler::ClientAuthHandler(CoreAccount account, QObject *parent)
    : AuthHandler(parent),
    _peer(0),
    _account(account)
{

}


void ClientAuthHandler::connectToCore()
{
    CoreAccountSettings s;

#ifdef HAVE_SSL
    QSslSocket *socket = new QSslSocket(this);
    // make sure the warning is shown if we happen to connect without SSL support later
    s.setAccountValue("ShowNoClientSslWarning", true);
#else
    if (_account.useSsl()) {
        if (s.accountValue("ShowNoClientSslWarning", true).toBool()) {
            bool accepted = false;
            emit handleNoSslInClient(&accepted);
            if (!accepted) {
                emit errorMessage(tr("Unencrypted connection canceled"));
                return;
            }
            s.setAccountValue("ShowNoClientSslWarning", false);
        }
    }
    QTcpSocket *socket = new QTcpSocket(this);
#endif

// TODO: Handle system proxy
#ifndef QT_NO_NETWORKPROXY
    if (_account.useProxy()) {
        QNetworkProxy proxy(_account.proxyType(), _account.proxyHostName(), _account.proxyPort(), _account.proxyUser(), _account.proxyPassword());
        socket->setProxy(proxy);
    }
#endif

    setSocket(socket);
    // handled by the base class for now; may need to rethink for protocol detection
    //connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
    //connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(connected()), SLOT(onSocketConnected()));

    emit statusMessage(tr("Connecting to %1...").arg(_account.accountName()));
    socket->connectToHost(_account.hostName(), _account.port());
}


// TODO: handle protocol detection
// This method might go away anyway, unless we really need our own states...
/*
void ClientAuthHandler::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << Q_FUNC_INFO << socketState;
    QString text;
    AuthHandler::State state = UnconnectedState;

    switch(socketState) {
        case QAbstractSocket::UnconnectedState:
            text = tr("Disconnected");
            state = UnconnectedState;
            break;
        case QAbstractSocket::HostLookupState:
            text = tr("Looking up %1...").arg(_account.hostName());
            state = HostLookupState;
            break;
        case QAbstractSocket::ConnectingState:
            text = tr("Connecting to %1...").arg(_account.hostName());
            state = ConnectingState;
            break;
        case QAbstractSocket::ConnectedState:
            text = tr("Connected to %1").arg(_account.hostName());
            state = ConnectedState;
            break;
        case QAbstractSocket::ClosingState:
            text = tr("Disconnecting from %1...").arg(_account.hostName());
            state = ClosingState;
            break;
        default:
            break;
    }

    if (!text.isEmpty()) {
        setState(state);
        emit statusMessage(text);
    }
}
*/

// TODO: handle protocol detection
/*
void ClientAuthHandler::onSocketError(QAbstractSocket::SocketError error)
{
    emit socketError(error, socket()->errorString());
}
*/

void ClientAuthHandler::onSocketConnected()
{
    // TODO: protocol detection

    if (_peer) {
        qWarning() << Q_FUNC_INFO << "Peer already exists!";
        return;
    }

    _peer = new LegacyPeer(this, socket(), this);

    connect(_peer, SIGNAL(transferProgress(int,int)), SIGNAL(transferProgress(int,int)));

    // compat only
    connect(_peer, SIGNAL(protocolVersionMismatch(int,int)), SLOT(onProtocolVersionMismatch(int,int)));

    emit statusMessage(tr("Synchronizing to core..."));

    bool useSsl = false;
#ifdef HAVE_SSL
    useSsl = _account.useSsl();
#endif

    _peer->dispatch(RegisterClient(Quassel::buildInfo().fancyVersionString, useSsl));
}


void ClientAuthHandler::onProtocolVersionMismatch(int actual, int expected)
{
    emit errorPopup(tr("<b>The Quassel Core you are trying to connect to is too old!</b><br>"
           "We need at least protocol v%1, but the core speaks v%2 only.").arg(expected, actual));
    requestDisconnect(tr("Incompatible protocol version, connection to core refused"));
}


void ClientAuthHandler::handle(const ClientDenied &msg)
{
    emit errorPopup(msg.errorString);
    requestDisconnect(tr("The core refused connection from this client"));
}


void ClientAuthHandler::handle(const ClientRegistered &msg)
{
    _coreConfigured = msg.coreConfigured;
    _backendInfo = msg.backendInfo;

    Client::setCoreFeatures(static_cast<Quassel::Features>(msg.coreFeatures));

#ifdef HAVE_SSL
    CoreAccountSettings s;
    if (_account.useSsl()) {
        if (msg.sslSupported) {
            // Make sure the warning is shown next time we don't have SSL in the core
            s.setAccountValue("ShowNoCoreSslWarning", true);

            QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket());
            Q_ASSERT(sslSocket);
            connect(sslSocket, SIGNAL(encrypted()), SLOT(onSslSocketEncrypted()));
            connect(sslSocket, SIGNAL(sslErrors(QList<QSslError>)), SLOT(onSslErrors()));
            qDebug() << "Starting encryption...";
            sslSocket->flush();
            sslSocket->startClientEncryption();
        }
        else {
            if (s.accountValue("ShowNoCoreSslWarning", true).toBool()) {
                bool accepted = false;
                emit handleNoSslInCore(&accepted);
                if (!accepted) {
                    requestDisconnect(tr("Unencrypted connection cancelled"));
                    return;
                }
                s.setAccountValue("ShowNoCoreSslWarning", false);
                s.setAccountValue("SslCert", QString());
            }
            onConnectionReady();
        }
        return;
    }
#endif
    // if we use SSL we wait for the next step until every SSL warning has been cleared
    onConnectionReady();
}


#ifdef HAVE_SSL

void ClientAuthHandler::onSslSocketEncrypted()
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
    onConnectionReady();
}


void ClientAuthHandler::onSslErrors()
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
            requestDisconnect(tr("Unencrypted connection canceled"));
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


void ClientAuthHandler::onConnectionReady()
{
    emit connectionReady();
    emit statusMessage(tr("Connected to %1").arg(_account.accountName()));

    if (!_coreConfigured) {
        // start wizard
        emit startCoreSetup(_backendInfo);
    }
    else // TODO: check if we need LoginEnabled
        login();
}


void ClientAuthHandler::setupCore(const SetupData &setupData)
{
    _peer->dispatch(setupData);
}


void ClientAuthHandler::handle(const SetupFailed &msg)
{
    emit coreSetupFailed(msg.errorString);
}


void ClientAuthHandler::handle(const SetupDone &msg)
{
    Q_UNUSED(msg)

    emit coreSetupSuccessful();
}


void ClientAuthHandler::login(const QString &user, const QString &password, bool remember)
{
    _account.setUser(user);
    _account.setPassword(password);
    _account.setStorePassword(remember);
    login();
}


void ClientAuthHandler::login(const QString &previousError)
{
    emit statusMessage(tr("Logging in..."));
    if (_account.user().isEmpty() || _account.password().isEmpty() || !previousError.isEmpty()) {
        bool valid = false;
        emit userAuthenticationRequired(&_account, &valid, previousError); // *must* be a synchronous call
        if (!valid || _account.user().isEmpty() || _account.password().isEmpty()) {
            requestDisconnect(tr("Login canceled"));
            return;
        }
    }

    _peer->dispatch(Login(_account.user(), _account.password()));
}


void ClientAuthHandler::handle(const LoginFailed &msg)
{
    login(msg.errorString);
}


void ClientAuthHandler::handle(const LoginSuccess &msg)
{
    Q_UNUSED(msg)

    emit loginSuccessful(_account);
}


void ClientAuthHandler::handle(const SessionState &msg)
{
    disconnect(socket(), 0, this, 0); // this is the last message we shall ever get

    // give up ownership of the peer; CoreSession takes responsibility now
    _peer->setParent(0);
    emit handshakeComplete(_peer, msg);
}

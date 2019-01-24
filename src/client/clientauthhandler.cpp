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

#include "clientauthhandler.h"

// TODO: support system application proxy (new in Qt 4.6)

#include <QtEndian>

#ifdef HAVE_SSL
    #include <QSslSocket>
#else
    #include <QTcpSocket>
#endif

#include "client.h"
#include "clientsettings.h"
#include "logmessage.h"
#include "peerfactory.h"

#if QT_VERSION < 0x050000
#    include "../../3rdparty/sha512/sha512.h"
#endif

using namespace Protocol;

ClientAuthHandler::ClientAuthHandler(CoreAccount account, QObject *parent)
    : AuthHandler(parent),
    _peer(nullptr),
    _account(account),
    _probing(false),
    _legacy(false),
    _connectionFeatures(0)
{

}


Peer *ClientAuthHandler::peer() const
{
    return _peer;
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

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    proxy.setType(_account.proxyType());
    if (_account.proxyType() == QNetworkProxy::Socks5Proxy ||
            _account.proxyType() == QNetworkProxy::HttpProxy) {
        proxy.setHostName(_account.proxyHostName());
        proxy.setPort(_account.proxyPort());
        proxy.setUser(_account.proxyUser());
        proxy.setPassword(_account.proxyPassword());
    }

    if (_account.proxyType() == QNetworkProxy::DefaultProxy) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        socket->setProxy(proxy);
    }
#endif

    setSocket(socket);
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(readyRead()), SLOT(onReadyRead()));
    connect(socket, SIGNAL(connected()), SLOT(onSocketConnected()));

    emit statusMessage(tr("Connecting to %1...").arg(_account.accountName()));
    socket->connectToHost(_account.hostName(), _account.port());
}


void ClientAuthHandler::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    QString text;

    switch(socketState) {
        case QAbstractSocket::HostLookupState:
            if (!_legacy)
                text = tr("Looking up %1...").arg(_account.hostName());
            break;
        case QAbstractSocket::ConnectingState:
            if (!_legacy)
                text = tr("Connecting to %1...").arg(_account.hostName());
            break;
        case QAbstractSocket::ConnectedState:
            text = tr("Connected to %1").arg(_account.hostName());
            break;
        case QAbstractSocket::ClosingState:
            if (!_probing)
                text = tr("Disconnecting from %1...").arg(_account.hostName());
            break;
        case QAbstractSocket::UnconnectedState:
            if (!_probing) {
                text = tr("Disconnected");
                // Ensure the disconnected() signal is sent even if we haven't reached the Connected state yet.
                // The baseclass implementation will make sure to only send the signal once.
                // However, we do want to prefer a potential socket error signal that may be on route already, so
                // give this a chance to overtake us by spinning the loop...
                QTimer::singleShot(0, this, SLOT(onSocketDisconnected()));
            }
            break;
        default:
            break;
    }

    if (!text.isEmpty()) {
        emit statusMessage(text);
    }
}

void ClientAuthHandler::onSocketError(QAbstractSocket::SocketError error)
{
    if (_probing && error == QAbstractSocket::RemoteHostClosedError) {
        _legacy = true;
        return;
    }

    _probing = false; // all other errors are unrelated to probing and should be handled
    AuthHandler::onSocketError(error);
}


void ClientAuthHandler::onSocketDisconnected()
{
    if (_probing && _legacy) {
        // Remote host has closed the connection while probing
        _probing = false;
        disconnect(socket(), SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        emit statusMessage(tr("Reconnecting in compatibility mode..."));
        socket()->connectToHost(_account.hostName(), _account.port());
        return;
    }

    AuthHandler::onSocketDisconnected();
}


void ClientAuthHandler::onSocketConnected()
{
    if (_peer) {
        qWarning() << Q_FUNC_INFO << "Peer already exists!";
        return;
    }

    socket()->setSocketOption(QAbstractSocket::KeepAliveOption, true);

    if (!_legacy) {
        // First connection attempt, try probing for a capable core
        _probing = true;

        QDataStream stream(socket()); // stream handles the endianness for us
        stream.setVersion(QDataStream::Qt_4_2);

        quint32 magic = Protocol::magic;
#ifdef HAVE_SSL
        if (_account.useSsl())
            magic |= Protocol::Encryption;
#endif
        magic |= Protocol::Compression;

        stream << magic;

        // here goes the list of protocols we support, in order of preference
        PeerFactory::ProtoList protos = PeerFactory::supportedProtocols();
        for (int i = 0; i < protos.count(); ++i) {
            quint32 reply = protos[i].first;
            reply |= protos[i].second<<8;
            if (i == protos.count() - 1)
                reply |= 0x80000000; // end list
            stream << reply;
        }

        socket()->flush(); // make sure the probing data is sent immediately
        return;
    }

    // If we arrive here, it's the second connection attempt, meaning probing was not successful -> enable legacy support

    qDebug() << "Legacy core detected, switching to compatibility mode";

    RemotePeer *peer = PeerFactory::createPeer(PeerFactory::ProtoDescriptor(Protocol::LegacyProtocol, 0), this, socket(), Compressor::NoCompression, this);
    // Only needed for the legacy peer, as all others check the protocol version before instantiation
    connect(peer, SIGNAL(protocolVersionMismatch(int,int)), SLOT(onProtocolVersionMismatch(int,int)));

    setPeer(peer);
}


void ClientAuthHandler::onReadyRead()
{
    if (socket()->bytesAvailable() < 4)
        return;

    if (!_probing)
        return; // make sure to not read more data than needed

    _probing = false;
    disconnect(socket(), SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    quint32 reply;
    socket()->read((char *)&reply, 4);
    reply = qFromBigEndian<quint32>(reply);

    Protocol::Type type = static_cast<Protocol::Type>(reply & 0xff);
    quint16 protoFeatures = static_cast<quint16>(reply>>8 & 0xffff);
    _connectionFeatures = static_cast<quint8>(reply>>24);

    Compressor::CompressionLevel level;
    if (_connectionFeatures & Protocol::Compression)
        level = Compressor::BestCompression;
    else
        level = Compressor::NoCompression;

    RemotePeer *peer = PeerFactory::createPeer(PeerFactory::ProtoDescriptor(type, protoFeatures), this, socket(), level, this);
    if (!peer) {
        qWarning() << "No valid protocol supported for this core!";
        emit errorPopup(tr("<b>Incompatible Quassel Core!</b><br>"
                           "None of the protocols this client speaks are supported by the core you are trying to connect to."));

        requestDisconnect(tr("Core speaks none of the protocols we support"));
        return;
    }

    if (peer->protocol() == Protocol::LegacyProtocol) {
        connect(peer, SIGNAL(protocolVersionMismatch(int,int)), SLOT(onProtocolVersionMismatch(int,int)));
        _legacy = true;
    }

    setPeer(peer);
}


void ClientAuthHandler::onProtocolVersionMismatch(int actual, int expected)
{
    emit errorPopup(tr("<b>The Quassel Core you are trying to connect to is too old!</b><br>"
           "We need at least protocol v%1, but the core speaks v%2 only.").arg(expected, actual));
    requestDisconnect(tr("Incompatible protocol version, connection to core refused"));
}


void ClientAuthHandler::setPeer(RemotePeer *peer)
{
    qDebug().nospace() << "Using " << qPrintable(peer->protocolName()) << "...";

    _peer = peer;
    connect(_peer, SIGNAL(transferProgress(int,int)), SIGNAL(transferProgress(int,int)));

    // The legacy protocol enables SSL later, after registration
    if (!_account.useSsl() || _legacy)
        startRegistration();
    // otherwise, do it now
    else
        checkAndEnableSsl(_connectionFeatures & Protocol::Encryption);
}


void ClientAuthHandler::startRegistration()
{
    emit statusMessage(tr("Synchronizing to core..."));

    // useSsl will be ignored by non-legacy peers
    bool useSsl = false;
#ifdef HAVE_SSL
    useSsl = _account.useSsl();
#endif

    _peer->dispatch(RegisterClient(Quassel::Features{}, Quassel::buildInfo().fancyVersionString, Quassel::buildInfo().commitDate, useSsl));
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
    _authenticatorInfo = msg.authenticatorInfo;

    _peer->setFeatures(std::move(msg.features));

    // The legacy protocol enables SSL at this point
    if(_legacy && _account.useSsl())
        checkAndEnableSsl(msg.sslSupported);
    else
        onConnectionReady();
}


void ClientAuthHandler::onConnectionReady()
{
    const auto &coreFeatures = _peer->features();
    auto unsupported = coreFeatures.toStringList(false);
    if (!unsupported.isEmpty()) {
        quInfo() << qPrintable(tr("Core does not support the following features: %1").arg(unsupported.join(", ")));
    }
    if (!coreFeatures.unknownFeatures().isEmpty()) {
        quInfo() << qPrintable(tr("Core supports unknown features: %1").arg(coreFeatures.unknownFeatures().join(", ")));
    }

    emit connectionReady();
    emit statusMessage(tr("Connected to %1").arg(_account.accountName()));

    if (!_coreConfigured) {
        // start wizard
        emit startCoreSetup(_backendInfo, _authenticatorInfo);
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


/*** SSL Stuff ***/

void ClientAuthHandler::checkAndEnableSsl(bool coreSupportsSsl)
{
#ifndef HAVE_SSL
    Q_UNUSED(coreSupportsSsl);
#else
    CoreAccountSettings s;
    if (coreSupportsSsl && _account.useSsl()) {
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
            s.setAccountValue("SslCertDigestVersion", QVariant(QVariant::Int));
        }
        if (_legacy)
            onConnectionReady();
        else
            startRegistration();
    }
#endif
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
        s.setAccountValue("SslCertDigestVersion", QVariant(QVariant::Int));
    }

    emit encrypted(true);

    if (_legacy)
        onConnectionReady();
    else
        startRegistration();
}


void ClientAuthHandler::onSslErrors()
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    Q_ASSERT(socket);

    CoreAccountSettings s;
    QByteArray knownDigest = s.accountValue("SslCert").toByteArray();
    ClientAuthHandler::DigestVersion knownDigestVersion = static_cast<ClientAuthHandler::DigestVersion>(s.accountValue("SslCertDigestVersion").toInt());

    QByteArray calculatedDigest;
    switch (knownDigestVersion) {
    case ClientAuthHandler::DigestVersion::Md5:
        calculatedDigest = socket->peerCertificate().digest(QCryptographicHash::Md5);
        break;

    case ClientAuthHandler::DigestVersion::Sha2_512:
#if QT_VERSION >= 0x050000
        calculatedDigest = socket->peerCertificate().digest(QCryptographicHash::Sha512);
#else
        calculatedDigest = sha2_512(socket->peerCertificate().toDer());
#endif
        break;

    default:
        qWarning() << "Certificate digest version" << QString(knownDigestVersion) << "is not supported";
    }

    if (knownDigest != calculatedDigest) {
        bool accepted = false;
        bool permanently = false;
        emit handleSslErrors(socket, &accepted, &permanently);

        if (!accepted) {
            requestDisconnect(tr("Unencrypted connection canceled"));
            return;
        }

        if (permanently) {
#if QT_VERSION >= 0x050000
            s.setAccountValue("SslCert", socket->peerCertificate().digest(QCryptographicHash::Sha512));
#else
            s.setAccountValue("SslCert", sha2_512(socket->peerCertificate().toDer()));
#endif
            s.setAccountValue("SslCertDigestVersion", ClientAuthHandler::DigestVersion::Latest);
        }
        else {
            s.setAccountValue("SslCert", QString());
            s.setAccountValue("SslCertDigestVersion", QVariant(QVariant::Int));
        }
    }
    else if (knownDigestVersion != ClientAuthHandler::DigestVersion::Latest) {
#if QT_VERSION >= 0x050000
        s.setAccountValue("SslCert", socket->peerCertificate().digest(QCryptographicHash::Sha512));
#else
        s.setAccountValue("SslCert", sha2_512(socket->peerCertificate().toDer()));
#endif
        s.setAccountValue("SslCertDigestVersion", ClientAuthHandler::DigestVersion::Latest);
    }

    socket->ignoreSslErrors();
}

#if QT_VERSION < 0x050000
QByteArray ClientAuthHandler::sha2_512(const QByteArray &input) {
    unsigned char output[64];
    sha512((unsigned char*) input.constData(), input.size(), output, false);
    // QByteArray::fromRawData() cannot be used here because that constructor
    // does not copy "output" and the data is clobbered when the variable goes
    // out of scope.
    QByteArray result;
    result.append((char*) output, 64);
    return result;
}
#endif

#endif /* HAVE_SSL */

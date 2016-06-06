/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include "coreauthhandler.h"

#ifdef HAVE_SSL
#  include <QSslSocket>
#endif

#include "core.h"
#include "logger.h"

using namespace Protocol;

CoreAuthHandler::CoreAuthHandler(QTcpSocket *socket, QObject *parent)
    : AuthHandler(parent),
    _peer(0),
    _magicReceived(false),
    _legacy(false),
    _clientRegistered(false),
    _connectionFeatures(0)
{
    setSocket(socket);
    connect(socket, SIGNAL(readyRead()), SLOT(onReadyRead()));

    // TODO: Timeout for the handshake phase

}


void CoreAuthHandler::onReadyRead()
{
    if (socket()->bytesAvailable() < 4)
        return;

    // once we have selected a peer, we certainly don't want to read more data!
    if (_peer)
        return;

    if (!_magicReceived) {
        quint32 magic;
        socket()->peek((char*)&magic, 4);
        magic = qFromBigEndian<quint32>(magic);

        if ((magic & 0xffffff00) != Protocol::magic) {
            // no magic, assume legacy protocol
            qDebug() << "Legacy client detected, switching to compatibility mode";
            _legacy = true;
            RemotePeer *peer = PeerFactory::createPeer(PeerFactory::ProtoDescriptor(Protocol::LegacyProtocol, 0), this, socket(), Compressor::NoCompression, this);
            connect(peer, SIGNAL(protocolVersionMismatch(int,int)), SLOT(onProtocolVersionMismatch(int,int)));
            setPeer(peer);
            return;
        }

        _magicReceived = true;
        quint8 features = magic & 0xff;
        // figure out which connection features we'll use based on the client's support
        if (Core::sslSupported() && (features & Protocol::Encryption))
            _connectionFeatures |= Protocol::Encryption;
        if (features & Protocol::Compression)
            _connectionFeatures |= Protocol::Compression;

        socket()->read((char*)&magic, 4); // read the 4 bytes we've just peeked at
    }

    // read the list of protocols supported by the client
    while (socket()->bytesAvailable() >= 4 && _supportedProtos.size() < 16) { // sanity check
        quint32 data;
        socket()->read((char*)&data, 4);
        data = qFromBigEndian<quint32>(data);

        Protocol::Type type = static_cast<Protocol::Type>(data & 0xff);
        quint16 protoFeatures = static_cast<quint16>(data>>8 & 0xffff);
        _supportedProtos.append(PeerFactory::ProtoDescriptor(type, protoFeatures));

        if (data >= 0x80000000) { // last protocol
            Compressor::CompressionLevel level;
            if (_connectionFeatures & Protocol::Compression)
                level = Compressor::BestCompression;
            else
                level = Compressor::NoCompression;

            RemotePeer *peer = PeerFactory::createPeer(_supportedProtos, this, socket(), level, this);
            if (!peer) {
                qWarning() << "Received invalid handshake data from client" << socket()->peerAddress().toString();
                close();
                return;
            }

            if (peer->protocol() == Protocol::LegacyProtocol) {
                _legacy = true;
                connect(peer, SIGNAL(protocolVersionMismatch(int,int)), SLOT(onProtocolVersionMismatch(int,int)));
            }
            setPeer(peer);

            // inform the client
            quint32 reply = peer->protocol() | peer->enabledFeatures()<<8 | _connectionFeatures<<24;
            reply = qToBigEndian<quint32>(reply);
            socket()->write((char*)&reply, 4);
            socket()->flush();

            if (!_legacy && (_connectionFeatures & Protocol::Encryption))
                startSsl(); // legacy peer enables it later
            return;
        }
    }
}


void CoreAuthHandler::setPeer(RemotePeer *peer)
{
    qDebug().nospace() << "Using " << qPrintable(peer->protocolName()) << "...";

    _peer = peer;
    disconnect(socket(), SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

// only in compat mode
void CoreAuthHandler::onProtocolVersionMismatch(int actual, int expected)
{
    qWarning() << qPrintable(tr("Client")) << _peer->description() << qPrintable(tr("too old, rejecting."));
    QString errorString = tr("<b>Your Quassel Client is too old!</b><br>"
                             "This core needs at least client/core protocol version %1 (got: %2).<br>"
                             "Please consider upgrading your client.").arg(expected, actual);
    _peer->dispatch(ClientDenied(errorString));
    _peer->close();
}


bool CoreAuthHandler::checkClientRegistered()
{
    if (!_clientRegistered) {
        qWarning() << qPrintable(tr("Client")) << qPrintable(socket()->peerAddress().toString()) << qPrintable(tr("did not send a registration message before trying to login, rejecting."));
        _peer->dispatch(ClientDenied(tr("<b>Client not initialized!</b><br>You need to send a registration message before trying to login.")));
        _peer->close();
        return false;
    }
    return true;
}


void CoreAuthHandler::handle(const RegisterClient &msg)
{
    bool useSsl;
    if (_legacy)
        useSsl = Core::sslSupported() && msg.sslSupported;
    else
        useSsl = _connectionFeatures & Protocol::Encryption;

    if (Quassel::isOptionSet("require-ssl") && !useSsl && !_peer->isLocal()) {
        quInfo() << qPrintable(tr("SSL required but non-SSL connection attempt from %1").arg(socket()->peerAddress().toString()));
        _peer->dispatch(ClientDenied(tr("<b>SSL is required!</b><br>You need to use SSL in order to connect to this core.")));
        _peer->close();
        return;
    }

    QVariantList backends;
    bool configured = Core::isConfigured();
    if (!configured)
        backends = Core::backendInfo();

    int uptime = Core::instance()->startTime().secsTo(QDateTime::currentDateTime().toUTC());
    int updays = uptime / 86400; uptime %= 86400;
    int uphours = uptime / 3600; uptime %= 3600;
    int upmins = uptime / 60;
    QString coreInfo = tr("<b>Quassel Core Version %1</b><br>"
                          "Version date: %2<br>"
                          "Up %3d%4h%5m (since %6)").arg(Quassel::buildInfo().fancyVersionString)
                          .arg(Quassel::buildInfo().commitDate)
                          .arg(updays).arg(uphours, 2, 10, QChar('0')).arg(upmins, 2, 10, QChar('0')).arg(Core::instance()->startTime().toString(Qt::TextDate));

    // useSsl and coreInfo are only used for the legacy protocol
    _peer->dispatch(ClientRegistered(Quassel::features(), configured, backends, useSsl, coreInfo));

    if (_legacy && useSsl)
        startSsl();

    _clientRegistered = true;
}


void CoreAuthHandler::handle(const SetupData &msg)
{
    if (!checkClientRegistered())
        return;

    QString result = Core::setup(msg.adminUser, msg.adminPassword, msg.backend, msg.setupData);
    if (!result.isEmpty())
        _peer->dispatch(SetupFailed(result));
    else
        _peer->dispatch(SetupDone());
}


void CoreAuthHandler::handle(const Login &msg)
{
    if (!checkClientRegistered())
        return;

    UserId uid = Core::validateUser(msg.user, msg.password);
    if (uid == 0) {
        quInfo() << qPrintable(tr("Invalid login attempt from %1 as \"%2\"").arg(socket()->peerAddress().toString(), msg.user));
        _peer->dispatch(LoginFailed(tr("<b>Invalid username or password!</b><br>The username/password combination you supplied could not be found in the database.")));
        return;
    }
    _peer->dispatch(LoginSuccess());

    quInfo() << qPrintable(tr("Client %1 initialized and authenticated successfully as \"%2\" (UserId: %3).").arg(socket()->peerAddress().toString(), msg.user, QString::number(uid.toInt())));

    disconnect(socket(), 0, this, 0);
    disconnect(_peer, 0, this, 0);
    _peer->setParent(0); // Core needs to take care of this one now!

    socket()->flush(); // Make sure all data is sent before handing over the peer (and socket) to the session thread (bug 682)
    emit handshakeComplete(_peer, uid);
}


/*** SSL Stuff ***/

void CoreAuthHandler::startSsl()
{
    #ifdef HAVE_SSL
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket());
    Q_ASSERT(sslSocket);

    qDebug() << qPrintable(tr("Starting encryption for Client:"))  << _peer->description();
    connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onSslErrors()));
    sslSocket->flush(); // ensure that the write cache is flushed before we switch to ssl (bug 682)
    sslSocket->startServerEncryption();
    #endif /* HAVE_SSL */
}


#ifdef HAVE_SSL
void CoreAuthHandler::onSslErrors()
{
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket());
    Q_ASSERT(sslSocket);
    sslSocket->ignoreSslErrors();
}
#endif


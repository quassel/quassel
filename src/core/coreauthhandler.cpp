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

#include "coreauthhandler.h"

#ifdef HAVE_SSL
#  include <QSslSocket>
#endif

#include "core.h"
#include "logger.h"

#include "protocols/legacy/legacypeer.h"

using namespace Protocol;

CoreAuthHandler::CoreAuthHandler(QTcpSocket *socket, QObject *parent)
    : AuthHandler(parent)
    , _peer(0)
    , _clientRegistered(false)
{
    setSocket(socket);

    // TODO: protocol detection

    // FIXME: make sure _peer gets deleted
    // TODO: socket ownership goes to the peer! (-> use shared ptr later...)
    _peer = new LegacyPeer(this, socket, this);
    // only in compat mode
    connect(_peer, SIGNAL(protocolVersionMismatch(int,int)), SLOT(onProtocolVersionMismatch(int,int)));
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


void CoreAuthHandler::startSsl()
{
#ifdef HAVE_SSL
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket());
    Q_ASSERT(sslSocket);

    qDebug() << qPrintable(tr("Starting encryption for Client:"))  << _peer->description();
    connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onSslErrors()));
    sslSocket->flush(); // ensure that the write cache is flushed before we switch to ssl (bug 682)
    sslSocket->startServerEncryption();
#endif
}


#ifdef HAVE_SSL
void CoreAuthHandler::onSslErrors()
{
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket());
    Q_ASSERT(sslSocket);
    sslSocket->ignoreSslErrors();
}
#endif


bool CoreAuthHandler::checkClientRegistered()
{
    if (!_clientRegistered) {
        qWarning() << qPrintable(tr("Client")) << qPrintable(socket()->peerAddress().toString()) << qPrintable(tr("did not send an init message before trying to login, rejecting."));
        _peer->dispatch(ClientDenied(tr("<b>Client not initialized!</b><br>You need to send an init message before trying to login.")));
        _peer->close();
        return false;
    }
    return true;
}


void CoreAuthHandler::handle(const RegisterClient &msg)
{
    // TODO: only in compat mode
    bool useSsl = false;
#ifdef HAVE_SSL
    if (Core::sslSupported() && msg.sslSupported)
        useSsl = true;
#endif
    QVariantList backends;
    bool configured = Core::isConfigured();
    if (!configured)
        backends = Core::backendInfo();

    _peer->dispatch(ClientRegistered(Quassel::features(), configured, backends, useSsl, Core::instance()->startTime()));
    // TODO: only in compat mode
    if (useSsl)
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

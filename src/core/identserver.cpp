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

#include <limits>

#include "corenetwork.h"
#include "identserver.h"
#include "logmessage.h"

IdentServer::IdentServer(QObject *parent)
    : QObject(parent)
{
    connect(&_server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
    connect(&_v6server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
}


bool IdentServer::startListening()
{
    bool success = false;

    uint16_t port = Quassel::optionValue("ident-port").toUShort();

    const QString listen = Quassel::optionValue("ident-listen");
    const QStringList listen_list = listen.split(",", QString::SkipEmptyParts);
    for (const QString &listen_term : listen_list) { // TODO: handle multiple interfaces for same TCP version gracefully
        QHostAddress addr;
        if (!addr.setAddress(listen_term)) {
            qCritical() << qPrintable(
                    tr("Invalid listen address %1")
                            .arg(listen_term)
            );
        }
        else {
            switch (addr.protocol()) {
                case QAbstractSocket::IPv6Protocol:
                    if (_v6server.listen(addr, port)) {
                        quInfo() << qPrintable(
                                tr("Listening for identd requests on IPv6 %1 port %2")
                                        .arg(addr.toString())
                                        .arg(_v6server.serverPort())
                        );
                        success = true;
                    }
                    else
                        quWarning() << qPrintable(
                                tr("Could not open IPv6 interface %1:%2: %3")
                                        .arg(addr.toString())
                                        .arg(port)
                                        .arg(_v6server.errorString()));
                    break;
                case QAbstractSocket::IPv4Protocol:
                    if (_server.listen(addr, port)) {
                        quInfo() << qPrintable(
                                tr("Listening for identd requests on IPv4 %1 port %2")
                                        .arg(addr.toString())
                                        .arg(_server.serverPort())
                        );
                        success = true;
                    }
                    else {
                        // if v6 succeeded on Any, the port will be already in use - don't display the error then
                        if (!success || _server.serverError() != QAbstractSocket::AddressInUseError)
                            quWarning() << qPrintable(
                                    tr("Could not open IPv4 interface %1:%2: %3")
                                            .arg(addr.toString())
                                            .arg(port)
                                            .arg(_server.errorString()));
                    }
                    break;
                default:
                    qCritical() << qPrintable(
                            tr("Invalid listen address %1, unknown network protocol")
                                    .arg(listen_term)
                    );
                    break;
            }
        }
    }

    if (!success) {
        quError() << qPrintable(tr("Identd could not open any network interfaces to listen on! No identd functionality will be available"));
    }

    return success;
}


void IdentServer::stopListening(const QString &msg)
{
    bool wasListening = false;

    if (_server.isListening()) {
        wasListening = true;
        _server.close();
    }
    if (_v6server.isListening()) {
        wasListening = true;
        _v6server.close();
    }

    if (wasListening) {
        if (msg.isEmpty())
            quInfo() << "No longer listening for identd clients.";
        else
            quInfo() << qPrintable(msg);
    }
}


void IdentServer::incomingConnection()
{
    auto server = qobject_cast<QTcpServer *>(sender());
    Q_ASSERT(server);
    while (server->hasPendingConnections()) {
        QTcpSocket *socket = server->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(respond()));
        connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    }
}


void IdentServer::respond()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    Q_ASSERT(socket);

    qint64 transactionId = _socketId;

    if (!socket->canReadLine()) {
        return;
    }

    QByteArray query = socket->readLine();
    if (query.endsWith("\r\n"))
        query.chop(2);
    else if (query.endsWith("\n"))
        query.chop(1);

    QList<QByteArray> split = query.split(',');

    bool success = false;

    quint16 localPort = 0;
    if (!split.empty()) {
        localPort = split[0].trimmed().toUShort(&success, 10);
    }

    Request request{socket, localPort, query, transactionId, _requestId++};
    if (!success) {
        request.respondError("INVALID-PORT");
    }
    else if (responseAvailable(request)) {
        // success
    }
    else if (lowestSocketId() < transactionId) {
        _requestQueue.emplace_back(request);
    }
    else {
        request.respondError("NO-USER");
    }
}


void Request::respondSuccess(const QString &user)
{
    if (socket) {
        QString data = query + " : USERID : Quassel : " + user + "\r\n";
        socket->write(data.toUtf8());
        socket->flush();
        socket->close();
    }
}


void Request::respondError(const QString &error)
{
    if (socket) {
        QString data = query + " : ERROR : " + error + "\r\n";
        socket->write(data.toUtf8());
        socket->flush();
        socket->close();
    }
}


bool IdentServer::responseAvailable(Request request) const
{
    if (!_connections.contains(request.localPort)) {
        return false;
    }

    request.respondSuccess(_connections[request.localPort]);
    return true;
}


void IdentServer::addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort,
                            const QHostAddress &peerAddress, quint16 peerPort, qint64 socketId)
{
    Q_UNUSED(localAddress)
    Q_UNUSED(peerAddress)
    Q_UNUSED(peerPort)

    const CoreNetwork *network = qobject_cast<CoreNetwork *>(sender());
    _connections[localPort] = network->coreSession()->strictCompliantIdent(identity);;
    processWaiting(socketId);
}


void IdentServer::removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort,
                               const QHostAddress &peerAddress, quint16 peerPort, qint64 socketId)
{
    Q_UNUSED(identity)
    Q_UNUSED(localAddress)
    Q_UNUSED(peerAddress)
    Q_UNUSED(peerPort)

    _connections.remove(localPort);
    processWaiting(socketId);
}


qint64 IdentServer::addWaitingSocket()
{
    qint64 newSocketId = _socketId++;
    _waiting.push_back(newSocketId);
    return newSocketId;
}


qint64 IdentServer::lowestSocketId() const
{
    if (_waiting.empty()) {
        return std::numeric_limits<qint64>::max();
    }

    return _waiting.front();
}


void IdentServer::removeWaitingSocket(qint64 socketId)
{
    _waiting.remove(socketId);
}


void IdentServer::processWaiting(qint64 socketId)
{
    removeWaitingSocket(socketId);

    _requestQueue.remove_if([this, socketId](Request request) {
        if (socketId < request.transactionId && responseAvailable(request)) {
            return true;
        }
        else if (lowestSocketId() < request.transactionId) {
            return false;
        }
        else {
            request.respondError("NO-USER");
            return true;
        }
    });
}


bool operator==(const Request &a, const Request &b)
{
    return a.requestId == b.requestId;
}

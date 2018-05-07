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

#include <logger.h>
#include <set>

#include "corenetwork.h"
#include "identserver.h"

IdentServer::IdentServer(bool strict, QObject *parent) : QObject(parent), _strict(strict), _socketId(0), _requestId(0) {
    connect(&_server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
    connect(&_v6server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
}

IdentServer::~IdentServer() = default;

bool IdentServer::startListening() {
    uint16_t port = 10113;

    bool success = false;
    if (_v6server.listen(QHostAddress("::1"), port)) {
        quInfo() << qPrintable(
                tr("Listening for identd clients on IPv6 %1 port %2")
                        .arg("::1")
                        .arg(_v6server.serverPort())
        );

        success = true;
    }

    if (_server.listen(QHostAddress("127.0.0.1"), port)) {
        success = true;

        quInfo() << qPrintable(
                tr("Listening for identd clients on IPv4 %1 port %2")
                        .arg("127.0.0.1")
                        .arg(_server.serverPort())
        );
    }

    if (!success) {
        quError() << qPrintable(
                tr("Identd could not open any network interfaces to listen on! No identd functionality will be available"));
    }

    return success;
}

void IdentServer::stopListening(const QString &msg) {
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

void IdentServer::incomingConnection() {
    auto *server = qobject_cast<QTcpServer *>(sender());
    Q_ASSERT(server);
    while (server->hasPendingConnections()) {
        QTcpSocket *socket = server->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(respond()));
    }
}

void IdentServer::respond() {
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    Q_ASSERT(socket);

    qint64 transactionId = _socketId;

    if (socket->canReadLine()) {
        QByteArray query = socket->readLine();
        if (query.endsWith("\r\n"))
            query.chop(2);
        else if (query.endsWith("\n"))
            query.chop(1);

        QList<QByteArray> split = query.split(',');

        bool success = false;

        quint16 localPort;
        if (!split.empty()) {
            localPort = split[0].trimmed().toUShort(&success, 10);
        }

        Request request{socket, localPort, query, transactionId, _requestId++};
        if (!success) {
            responseUnavailable(request);
        } else if (!responseAvailable(request)) {
            if (hasSocketsBelowId(transactionId)) {
                _requestQueue.emplace_back(request);
            } else {
                responseUnavailable(request);
            }
        }
    }
}

bool IdentServer::responseAvailable(Request request) {
    QString user;
    bool success = true;
    if (_connections.contains(request.localPort)) {
        user = _connections[request.localPort];
    } else {
        success = false;
    }

    QString data;
    if (success) {
        data += request.query + " : USERID : Quassel : " + user + "\r\n";

        request.socket->write(data.toUtf8());
        request.socket->flush();
        request.socket->close();
    }
    return success;
}

void IdentServer::responseUnavailable(Request request) {
    QString data = request.query + " : ERROR : NO-USER\r\n";

    request.socket->write(data.toUtf8());
    request.socket->flush();
    request.socket->close();
}


bool IdentServer::addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort,
                            const QHostAddress &peerAddress, quint16 peerPort, qint64 socketId) {
    Q_UNUSED(localAddress)
    Q_UNUSED(peerAddress)
    Q_UNUSED(peerPort)

    const CoreNetwork *network = qobject_cast<CoreNetwork *>(sender());
    _connections[localPort] = network->coreSession()->strictCompliantIdent(identity);;
    processWaiting(socketId);
    return true;
}


bool IdentServer::removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort,
                               const QHostAddress &peerAddress, quint16 peerPort, qint64 socketId) {
    Q_UNUSED(identity)
    Q_UNUSED(localAddress)
    Q_UNUSED(peerAddress)
    Q_UNUSED(peerPort)

    _connections.remove(localPort);
    processWaiting(socketId);
    return true;
}

qint64 IdentServer::addWaitingSocket() {
    qint64 newSocketId = _socketId++;
    _waiting.push_back(newSocketId);
    return newSocketId;
}

bool IdentServer::hasSocketsBelowId(qint64 id) {
    return std::any_of(_waiting.begin(), _waiting.end(), [=](qint64 socketId) {
        return socketId <= id;
    });
}

void IdentServer::removeWaitingSocket(qint64 socketId) {
    _waiting.remove(socketId);
}

void IdentServer::processWaiting(qint64 socketId) {
    qint64 lowestSocketId = std::numeric_limits<qint64 >::max();
    for (qint64 id : _waiting) {
        if (id < lowestSocketId) {
            lowestSocketId = id;
        }
    }
    removeWaitingSocket(socketId);
    _requestQueue.remove_if([=](Request request) {
        if (request.transactionId < lowestSocketId) {
            responseUnavailable(request);
            return true;
        } else if (request.transactionId > socketId) {
            return responseAvailable(request);
        } else {
            return false;
        }
    });
}

bool operator==(const Request &a, const Request &b) {
    return a.requestId == b.requestId;
}

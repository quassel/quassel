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

#include "corenetwork.h"
#include "identserver.h"

IdentServer::IdentServer(bool strict, QObject *parent) : QObject(parent), _strict(strict) {
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
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    Q_ASSERT(socket);

    if (socket->canReadLine()) {
        QByteArray s = socket->readLine();
        if (s.endsWith("\r\n"))
            s.chop(2);
        else if (s.endsWith("\n"))
            s.chop(1);

        QList<QByteArray> split = s.split(',');

        bool success = false;

        uint16_t localPort;
        if (!split.empty()) {
            localPort = split[0].toUShort(&success, 10);
        }

        QString user;
        if (success) {
            if (_connections.contains(localPort)) {
                user = _connections[localPort];
            } else {
                success = false;
            }
        }

        QString data;
        if (success) {
            data += s + " : USERID : Quassel : " + user + "\r\n";
        } else {
            data += s + " : ERROR : NO-USER\r\n";
        }

        socket->write(data.toUtf8());
        socket->flush();
        socket->close();
        socket->deleteLater();
    }
}


bool IdentServer::addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort,
                            const QHostAddress &peerAddress, quint16 peerPort) {
    Q_UNUSED(localAddress)
    Q_UNUSED(peerAddress)
    Q_UNUSED(peerPort)

    const CoreNetwork *network = qobject_cast<CoreNetwork *>(sender());
    _connections[localPort] = network->coreSession()->strictCompliantIdent(identity);;
    return true;
}


//! not yet implemented
bool IdentServer::removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort,
                               const QHostAddress &peerAddress, quint16 peerPort) {
    Q_UNUSED(identity)
    Q_UNUSED(localAddress)
    Q_UNUSED(peerAddress)
    Q_UNUSED(peerPort)

    _connections.remove(localPort);
    return true;
}

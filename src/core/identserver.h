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

#pragma once

#include <list>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

#include "coreidentity.h"

struct Request
{
    QPointer<QTcpSocket> socket;
    uint16_t localPort;
    QString query;
    qint64 transactionId;
    qint64 requestId;

    friend bool operator==(const Request &a, const Request &b);

    void respondSuccess(const QString &user);
    void respondError(const QString &error);
};


class IdentServer : public QObject
{
    Q_OBJECT

public:
    IdentServer(QObject *parent = nullptr);

    bool startListening();
    void stopListening(const QString &msg);
    qint64 addWaitingSocket();

public slots:
    void addSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort, qint64 socketId);
    void removeSocket(const CoreIdentity *identity, const QHostAddress &localAddress, quint16 localPort, const QHostAddress &peerAddress, quint16 peerPort, qint64 socketId);

private slots:
    void incomingConnection();
    void respond();

private:
    bool responseAvailable(Request request) const;

    qint64 lowestSocketId() const;

    void processWaiting(qint64 socketId);

    void removeWaitingSocket(qint64 socketId);

    QTcpServer _server, _v6server;

    QHash<uint16_t, QString> _connections;
    std::list<Request> _requestQueue;
    std::list<qint64> _waiting;
    qint64 _socketId{0};
    qint64 _requestId{0};
};

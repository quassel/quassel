/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <QHash>
#include <QObject>
#include <QString>
#include <QTcpServer>

#include "coreidentity.h"

class MetricsServer : public QObject
{
    Q_OBJECT

public:
    explicit MetricsServer(QObject* parent = nullptr);

    bool startListening();
    void stopListening(const QString& msg);

    void addLoginAttempt(UserId user, bool successful);
    void addLoginAttempt(const QString& user, bool successful);

    void addSession(UserId user, const QString& name);
    void removeSession(UserId user);

    void addClient(UserId user);
    void removeClient(UserId user);

    void addNetwork(UserId user);
    void removeNetwork(UserId user);

    void transmitDataNetwork(UserId user, uint64_t size);
    void receiveDataNetwork(UserId user, uint64_t size);

    void messageQueue(UserId user, uint64_t size);

    void setCertificateExpires(QDateTime expires);

private slots:
    void incomingConnection();
    void respond();

private:
    QTcpServer _server, _v6server;

    QHash<UserId, uint64_t> _loginAttempts{};
    QHash<UserId, uint64_t> _successfulLogins{};

    QHash<UserId, QString> _sessions{};

    QHash<UserId, int32_t> _clientSessions{};
    QHash<UserId, int32_t> _networkSessions{};

    QHash<UserId, uint64_t> _networkDataTransmit{};
    QHash<UserId, uint64_t> _networkDataReceive{};

    QHash<UserId, uint64_t> _messageQueue{};

    QDateTime _certificateExpires{};
};

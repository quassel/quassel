/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "common-export.h"

#include <QTcpSocket>

#include "protocol.h"

class Peer;

class COMMON_EXPORT AuthHandler : public QObject
{
    Q_OBJECT

public:
    AuthHandler(QObject* parent = nullptr);

    QTcpSocket* socket() const;

    virtual bool isLocal() const;

    virtual void handle(const Protocol::RegisterClient&) { invalidMessage(); }
    virtual void handle(const Protocol::ClientDenied&) { invalidMessage(); }
    virtual void handle(const Protocol::ClientRegistered&) { invalidMessage(); }
    virtual void handle(const Protocol::SetupData&) { invalidMessage(); }
    virtual void handle(const Protocol::SetupFailed&) { invalidMessage(); }
    virtual void handle(const Protocol::SetupDone&) { invalidMessage(); }
    virtual void handle(const Protocol::Login&) { invalidMessage(); }
    virtual void handle(const Protocol::LoginFailed&) { invalidMessage(); }
    virtual void handle(const Protocol::LoginSuccess&) { invalidMessage(); }
    virtual void handle(const Protocol::SessionState&) { invalidMessage(); }

    // fallback for unknown types, will trigger an error
    template<class T>
    void handle(const T&)
    {
        invalidMessage();
    }

public slots:
    void close();

signals:
    void disconnected();
    void socketError(QAbstractSocket::SocketError error, const QString& errorString);

protected:
    void setSocket(QTcpSocket* socket);

protected slots:
    virtual void onSocketError(QAbstractSocket::SocketError error);
    virtual void onSocketDisconnected();

private:
    void invalidMessage();

    QTcpSocket* _socket{nullptr};  // FIXME: should be a QSharedPointer? -> premature disconnect before the peer has taken over
    bool _disconnectedSent{false};
};

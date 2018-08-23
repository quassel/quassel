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

#include <QPointer>

#include "transfer.h"
#include "peer.h"

class QTcpSocket;

class CoreTransfer : public Transfer
{
    Q_OBJECT

public:
    CoreTransfer(Direction direction, const QString &nick, const QString &fileName, const QHostAddress &address, quint16 port, quint64 size = 0, QObject *parent = 0);

    quint64 transferred() const override;

public slots:
    void start();

    // called through sync calls
    void requestAccepted(PeerPtr peer) override;
    void requestRejected(PeerPtr peer) override;

private slots:
    void startReceiving();
    void onDataReceived();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void setupConnectionForReceive();
    bool relayData(const QByteArray &data, bool requireChunkSize);
    void cleanUp() override;

    QPointer<Peer> _peer;
    QTcpSocket *_socket;
    quint64 _pos;
    QByteArray _buffer;
    bool _reading;
};

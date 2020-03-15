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

#include <QDateTime>

#include "compressor.h"
#include "peer.h"
#include "protocol.h"
#include "proxyline.h"
#include "signalproxy.h"

class QTimer;

class AuthHandler;

class COMMON_EXPORT RemotePeer : public Peer
{
    Q_OBJECT

public:
    // import the virtuals from the baseclass
    using Peer::dispatch;
    using Peer::handle;

    RemotePeer(AuthHandler* authHandler, QTcpSocket* socket, Compressor::CompressionLevel level, QObject* parent = nullptr);

    void setSignalProxy(SignalProxy* proxy) override;

    void setProxyLine(ProxyLine proxyLine);

    virtual QString protocolName() const = 0;
    QString description() const override;
    virtual quint16 enabledFeatures() const { return 0; }

    QString address() const override;
    QHostAddress hostAddress() const;
    quint16 port() const override;

    bool isOpen() const override;
    bool isSecure() const override;
    bool isLocal() const override;

    int lag() const override;

    bool compressionEnabled() const;
    void setCompressionEnabled(bool enabled);

    QTcpSocket* socket() const;

public slots:
    void close(const QString& reason = QString()) override;

signals:
    void transferProgress(int current, int max);
    void socketError(QAbstractSocket::SocketError error, const QString& errorString);
    void statusMessage(const QString& msg);

    // Only used by LegacyPeer
    void protocolVersionMismatch(int actual, int expected);

protected:
    SignalProxy* signalProxy() const override;

    void writeMessage(const QByteArray& msg);
    virtual void processMessage(const QByteArray& msg) = 0;

    // These protocol messages get handled internally and won't reach SignalProxy
    void handle(const Protocol::HeartBeat& heartBeat);
    void handle(const Protocol::HeartBeatReply& heartBeatReply);
    virtual void dispatch(const Protocol::HeartBeat& msg) = 0;
    virtual void dispatch(const Protocol::HeartBeatReply& msg) = 0;

protected slots:
    virtual void onSocketStateChanged(QAbstractSocket::SocketState state);
    virtual void onSocketError(QAbstractSocket::SocketError error);

private slots:
    void onReadyRead();
    void onCompressionError(Compressor::Error error);

    void sendHeartBeat();
    void changeHeartBeatInterval(int secs);

private:
    bool readMessage(QByteArray& msg);

private:
    QTcpSocket* _socket;
    Compressor* _compressor;
    SignalProxy* _signalProxy;
    ProxyLine _proxyLine;
    bool _useProxyLine;
    QTimer* _heartBeatTimer;
    int _heartBeatCount;
    int _lag;
    quint32 _msgSize;
};

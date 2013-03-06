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

#ifndef REMOTEPEER_H
#define REMOTEPEER_H

#include <QDateTime>

#include "peer.h"
#include "protocol.h"
#include "signalproxy.h"

class QTcpSocket;
class QTimer;

class RemotePeer : public Peer
{
    Q_OBJECT

public:
    RemotePeer(QTcpSocket *socket, QObject *parent = 0);
    virtual ~RemotePeer() {};

    void setSignalProxy(SignalProxy *proxy);

    QString description() const;

    bool isOpen() const;
    bool isSecure() const;
    bool isLocal() const;

    int lag() const;

    bool compressionEnabled() const;
    void setCompressionEnabled(bool enabled);

    QTcpSocket *socket() const;

    // this is only used for the auth phase and should be replaced by something more generic
    virtual void writeSocketData(const QVariant &item) = 0;

public slots:
    void close(const QString &reason = QString());

signals:
    // this is only used for the auth phase and should be replaced by something more generic
    void dataReceived(const QVariant &item);

    void transferProgress(int current, int max);

protected:
    SignalProxy *signalProxy() const;

    using Peer::handle;
    using Peer::dispatch;

    // These protocol messages get handled internally and won't reach SignalProxy
    void handle(const Protocol::HeartBeat &heartBeat);
    void handle(const Protocol::HeartBeatReply &heartBeatReply);
    virtual void dispatch(const Protocol::HeartBeat &msg) = 0;
    virtual void dispatch(const Protocol::HeartBeatReply &msg) = 0;

private slots:
    void sendHeartBeat();
    void changeHeartBeatInterval(int secs);

private:
    QTcpSocket *_socket;
    SignalProxy *_signalProxy;
    QTimer *_heartBeatTimer;
    int _heartBeatCount;
    int _lag;
};

#endif

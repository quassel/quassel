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

#ifndef REMOTECONNECTION_H
#define REMOTECONNECTION_H

#include <QDateTime>
#include <QTcpSocket>

#include "protocol.h"
#include "signalproxy.h"

class QTimer;

class RemoteConnection : public SignalProxy::AbstractPeer
{
    Q_OBJECT

public:
    RemoteConnection(QTcpSocket *socket, QObject *parent = 0);
    virtual ~RemoteConnection() {};

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

    void disconnected();
    void error(QAbstractSocket::SocketError);

    void transferProgress(int current, int max);

protected:
    SignalProxy *signalProxy() const;

    template<class T>
    void handle(const T &protoMessage);

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


// Template methods we need in the header
template<class T> inline
void RemoteConnection::handle(const T &protoMessage)
{
    if (!signalProxy()) {
        qWarning() << Q_FUNC_INFO << "Cannot handle messages without a SignalProxy!";
        return;
    }

    // _heartBeatCount = 0;

    signalProxy()->handle(this, protoMessage);
}


#endif

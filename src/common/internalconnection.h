/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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

#ifndef INTERNALCONNECTION_H
#define INTERNALCONNECTION_H

#include <QTcpSocket>

#include "protocol.h"
#include "signalproxy.h"

class QEvent;

class InternalConnection : public SignalProxy::AbstractPeer
{
    Q_OBJECT

public:
    enum EventType {
        SyncMessageEvent = QEvent::User,
        RpcCallEvent,
        InitRequestEvent,
        InitDataEvent
    };

    InternalConnection(QObject *parent = 0);
    virtual ~InternalConnection();

    QString description() const;

    SignalProxy *signalProxy() const;
    void setSignalProxy(SignalProxy *proxy);

    InternalConnection *peer() const;
    void setPeer(InternalConnection *peer);

    bool isOpen() const;
    bool isSecure() const;
    bool isLocal() const;

    int lag() const;

    void dispatch(const Protocol::SyncMessage &msg);
    void dispatch(const Protocol::RpcCall &msg);
    void dispatch(const Protocol::InitRequest &msg);
    void dispatch(const Protocol::InitData &msg);

public slots:
    void close(const QString &reason = QString());

signals:

    void disconnected();
    void error(QAbstractSocket::SocketError);

protected:
    void customEvent(QEvent *event);

private slots:
    void peerDisconnected();

private:
    template<class T>
    void dispatch(EventType eventType, const T &msg);

    template<class T>
    void handle(const T &msg);

private:
    SignalProxy *_proxy;
    InternalConnection *_peer;
    bool _isOpen;
};


#endif

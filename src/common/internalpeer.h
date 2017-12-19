/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef INTERNALPEER_H
#define INTERNALPEER_H

#include "peer.h"
#include "protocol.h"
#include "signalproxy.h"

class QEvent;

class InternalPeer : public Peer
{
    Q_OBJECT

public:
    enum EventType {
        SyncMessageEvent = QEvent::User,
        RpcCallEvent,
        InitRequestEvent,
        InitDataEvent
    };

    InternalPeer(QObject *parent = 0);
    virtual ~InternalPeer();

    Protocol::Type protocol() const { return Protocol::InternalProtocol; }
    QString description() const;

    virtual QString address() const;
    virtual quint16 port() const;

    SignalProxy *signalProxy() const;
    void setSignalProxy(SignalProxy *proxy);

    InternalPeer *peer() const;
    void setPeer(InternalPeer *peer);

    bool isOpen() const;
    bool isSecure() const;
    bool isLocal() const;

    int lag() const;

    void dispatch(const Protocol::SyncMessage &msg);
    void dispatch(const Protocol::RpcCall &msg);
    void dispatch(const Protocol::InitRequest &msg);
    void dispatch(const Protocol::InitData &msg);

    /* These are not needed for InternalPeer */
    void dispatch(const Protocol::RegisterClient &) {}
    void dispatch(const Protocol::ClientDenied &) {}
    void dispatch(const Protocol::ClientRegistered &) {}
    void dispatch(const Protocol::SetupData &) {}
    void dispatch(const Protocol::SetupFailed &) {}
    void dispatch(const Protocol::SetupDone &) {}
    void dispatch(const Protocol::Login &) {}
    void dispatch(const Protocol::LoginFailed &) {}
    void dispatch(const Protocol::LoginSuccess &) {}
    void dispatch(const Protocol::SessionState &) {}

public slots:
    void close(const QString &reason = QString());

protected:
    void customEvent(QEvent *event);

private slots:
    void peerDisconnected();

private:
    template<class T>
    void dispatch(EventType eventType, const T &msg);

private:
    SignalProxy *_proxy;
    InternalPeer *_peer;
    bool _isOpen;
};


#endif

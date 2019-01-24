/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include <QPointer>
#include <QString>

#include "peer.h"
#include "protocol.h"
#include "signalproxy.h"

class COMMON_EXPORT InternalPeer : public Peer
{
    Q_OBJECT

public:
    InternalPeer(QObject* parent = nullptr);
    ~InternalPeer() override;

    Protocol::Type protocol() const override { return Protocol::InternalProtocol; }
    QString description() const override;

    QString address() const override;
    quint16 port() const override;

    SignalProxy* signalProxy() const override;
    void setSignalProxy(SignalProxy* proxy) override;

    InternalPeer* peer() const;
    void setPeer(InternalPeer* peer);

    bool isOpen() const override;
    bool isSecure() const override;
    bool isLocal() const override;

    int lag() const override;

    void dispatch(const Protocol::SyncMessage& msg) override;
    void dispatch(const Protocol::RpcCall& msg) override;
    void dispatch(const Protocol::InitRequest& msg) override;
    void dispatch(const Protocol::InitData& msg) override;

    /* These are not needed for InternalPeer */
    void dispatch(const Protocol::RegisterClient&) override {}
    void dispatch(const Protocol::ClientDenied&) override {}
    void dispatch(const Protocol::ClientRegistered&) override {}
    void dispatch(const Protocol::SetupData&) override {}
    void dispatch(const Protocol::SetupFailed&) override {}
    void dispatch(const Protocol::SetupDone&) override {}
    void dispatch(const Protocol::Login&) override {}
    void dispatch(const Protocol::LoginFailed&) override {}
    void dispatch(const Protocol::LoginSuccess&) override {}
    void dispatch(const Protocol::SessionState&) override {}

public slots:
    void close(const QString& reason = QString()) override;

signals:
    void dispatchMessage(const Protocol::SyncMessage& msg);
    void dispatchMessage(const Protocol::RpcCall& msg);
    void dispatchMessage(const Protocol::InitRequest& msg);
    void dispatchMessage(const Protocol::InitData& msg);

private slots:
    void peerDisconnected();

    void handleMessage(const Protocol::SyncMessage& msg);
    void handleMessage(const Protocol::RpcCall& msg);
    void handleMessage(const Protocol::InitRequest& msg);
    void handleMessage(const Protocol::InitData& msg);

private:
    template<typename T>
    void handle(const T& msg);

private:
    SignalProxy* _proxy{nullptr};
    bool _isOpen{true};
};

Q_DECLARE_METATYPE(QPointer<InternalPeer>)

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

#ifndef LEGACYPEER_H
#define LEGACYPEER_H

#include "../../remotepeer.h"

class LegacyPeer : public RemotePeer
{
    Q_OBJECT

public:
    enum RequestType
    {
        Sync = 1,
        RpcCall,
        InitRequest,
        InitData,
        HeartBeat,
        HeartBeatReply
    };

    LegacyPeer(AuthHandler* authHandler, QTcpSocket* socket, Compressor::CompressionLevel level, QObject* parent = nullptr);

    Protocol::Type protocol() const override { return Protocol::LegacyProtocol; }
    QString protocolName() const override { return "the legacy protocol"; }

    void setSignalProxy(SignalProxy* proxy) override;

    void dispatch(const Protocol::RegisterClient& msg) override;
    void dispatch(const Protocol::ClientDenied& msg) override;
    void dispatch(const Protocol::ClientRegistered& msg) override;
    void dispatch(const Protocol::SetupData& msg) override;
    void dispatch(const Protocol::SetupFailed& msg) override;
    void dispatch(const Protocol::SetupDone& msg) override;
    void dispatch(const Protocol::Login& msg) override;
    void dispatch(const Protocol::LoginFailed& msg) override;
    void dispatch(const Protocol::LoginSuccess& msg) override;
    void dispatch(const Protocol::SessionState& msg) override;

    void dispatch(const Protocol::SyncMessage& msg) override;
    void dispatch(const Protocol::RpcCall& msg) override;
    void dispatch(const Protocol::InitRequest& msg) override;
    void dispatch(const Protocol::InitData& msg) override;

    void dispatch(const Protocol::HeartBeat& msg) override;
    void dispatch(const Protocol::HeartBeatReply& msg) override;

signals:
    void protocolError(const QString& errorString);

private:
    using RemotePeer::writeMessage;
    void writeMessage(const QVariant& item);
    void processMessage(const QByteArray& msg) override;

    void handleHandshakeMessage(const QVariant& msg);
    void handlePackedFunc(const QVariant& packedFunc);
    void dispatchPackedFunc(const QVariantList& packedFunc);

    void toLegacyIrcUsersAndChannels(QVariantMap& initData);
    void fromLegacyIrcUsersAndChannels(QVariantMap& initData);

    bool _useCompression;
};

#endif

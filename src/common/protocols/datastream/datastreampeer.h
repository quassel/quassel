/***************************************************************************
 *   Copyright (C) 2005-2014 by the Quassel Project                        *
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

#ifndef DATASTREAMPEER_H
#define DATASTREAMPEER_H

#include <QDataStream>

#include "../../remotepeer.h"

class QDataStream;

class DataStreamPeer : public RemotePeer
{
    Q_OBJECT

public:
    enum RequestType {
        Sync = 1,
        RpcCall,
        InitRequest,
        InitData,
        HeartBeat,
        HeartBeatReply
    };

    DataStreamPeer(AuthHandler *authHandler, QTcpSocket *socket, quint16 features, QObject *parent = 0);

    Protocol::Type protocol() const { return Protocol::DataStreamProtocol; }

    static quint16 supportedFeatures();
    static bool acceptsFeatures(quint16 peerFeatures);
    quint16 enabledFeatures() const;

    void dispatch(const Protocol::RegisterClient &msg);
    void dispatch(const Protocol::ClientDenied &msg);
    void dispatch(const Protocol::ClientRegistered &msg);
    void dispatch(const Protocol::SetupData &msg);
    void dispatch(const Protocol::SetupFailed &msg);
    void dispatch(const Protocol::SetupDone &msg);
    void dispatch(const Protocol::Login &msg);
    void dispatch(const Protocol::LoginFailed &msg);
    void dispatch(const Protocol::LoginSuccess &msg);
    void dispatch(const Protocol::SessionState &msg);

    void dispatch(const Protocol::SyncMessage &msg);
    void dispatch(const Protocol::RpcCall &msg);
    void dispatch(const Protocol::InitRequest &msg);
    void dispatch(const Protocol::InitData &msg);

    void dispatch(const Protocol::HeartBeat &msg);
    void dispatch(const Protocol::HeartBeatReply &msg);

signals:
    void protocolError(const QString &errorString);

protected slots:
    void onSocketDataAvailable();

private:
    bool readSocketData(QByteArray &data);
    void writeSocketData(const QVariantList &list);
    void writeSocketData(const QVariantMap &handshakeMsg);

    void handleHandshakeMessage(const QVariantList &mapData);
    void handlePackedFunc(const QVariantList &packedFunc);
    void dispatchPackedFunc(const QVariantList &packedFunc);

    QDataStream _stream;
    quint32 _blockSize;
};

#endif

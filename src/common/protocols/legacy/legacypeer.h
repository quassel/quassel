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

#ifndef LEGACYPEER_H
#define LEGACYPEER_H

#include <QDataStream>

#include "../../remotepeer.h"

class QDataStream;

class LegacyPeer : public RemotePeer
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

    LegacyPeer(QTcpSocket *socket, QObject *parent = 0);
    ~LegacyPeer() {}

    void setSignalProxy(SignalProxy *proxy);

    void dispatch(const Protocol::SyncMessage &msg);
    void dispatch(const Protocol::RpcCall &msg);
    void dispatch(const Protocol::InitRequest &msg);
    void dispatch(const Protocol::InitData &msg);

    void dispatch(const Protocol::HeartBeat &msg);
    void dispatch(const Protocol::HeartBeatReply &msg);

    // FIXME: this is only used for the auth phase and should be replaced by something more generic
    void writeSocketData(const QVariant &item);

private slots:
    void socketDataAvailable();

private:
    bool readSocketData(QVariant &item);
    void handlePackedFunc(const QVariant &packedFunc);
    void dispatchPackedFunc(const QVariantList &packedFunc);

    QDataStream _stream;
    qint32 _blockSize;
    bool _useCompression;
};

#endif

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

#include <QTcpSocket>

#include "legacypeer.h"

using namespace Protocol;

LegacyPeer::LegacyPeer(QTcpSocket *socket, QObject *parent)
    : RemotePeer(socket, parent),
    _blockSize(0),
    _useCompression(false)
{
    _stream.setDevice(socket);
    _stream.setVersion(QDataStream::Qt_4_2);

    connect(socket, SIGNAL(readyRead()), SLOT(socketDataAvailable()));
}


void LegacyPeer::setSignalProxy(::SignalProxy *proxy)
{
    RemotePeer::setSignalProxy(proxy);

    if (proxy) {
        // enable compression now if requested - the initial handshake is uncompressed in the legacy protocol!
        _useCompression = socket()->property("UseCompression").toBool();
    }

}


void LegacyPeer::socketDataAvailable()
{
    QVariant item;
    while (readSocketData(item)) {
        // if no sigproxy is set, we're in handshake mode and let the data be handled elsewhere
        if (!signalProxy())
            emit dataReceived(item);
        else
            handlePackedFunc(item);
    }
}


bool LegacyPeer::readSocketData(QVariant &item)
{
    if (_blockSize == 0) {
        if (socket()->bytesAvailable() < 4)
            return false;
        _stream >> _blockSize;
    }

    if (_blockSize > 1 << 22) {
        close("Peer tried to send package larger than max package size!");
        return false;
    }

    if (_blockSize == 0) {
        close("Peer tried to send 0 byte package!");
        return false;
    }

    if (socket()->bytesAvailable() < _blockSize) {
        emit transferProgress(socket()->bytesAvailable(), _blockSize);
        return false;
    }

    emit transferProgress(_blockSize, _blockSize);

    _blockSize = 0;

    if (_useCompression) {
        QByteArray rawItem;
        _stream >> rawItem;

        int nbytes = rawItem.size();
        if (nbytes <= 4) {
            const char *data = rawItem.constData();
            if (nbytes < 4 || (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0)) {
                close("Peer sent corrupted compressed data!");
                return false;
            }
        }

        rawItem = qUncompress(rawItem);

        QDataStream itemStream(&rawItem, QIODevice::ReadOnly);
        itemStream.setVersion(QDataStream::Qt_4_2);
        itemStream >> item;
    }
    else {
        _stream >> item;
    }

    if (!item.isValid()) {
        close("Peer sent corrupt data: unable to load QVariant!");
        return false;
    }

    return true;
}


void LegacyPeer::writeSocketData(const QVariant &item)
{
    if (!socket()->isOpen()) {
        qWarning() << Q_FUNC_INFO << "Can't write to a closed socket!";
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_2);

    if (_useCompression) {
        QByteArray rawItem;
        QDataStream itemStream(&rawItem, QIODevice::WriteOnly);
        itemStream.setVersion(QDataStream::Qt_4_2);
        itemStream << item;

        rawItem = qCompress(rawItem);

        out << rawItem;
    }
    else {
        out << item;
    }

    _stream << block;  // also writes the length as part of the serialization format
}


void LegacyPeer::handlePackedFunc(const QVariant &packedFunc)
{
    QVariantList params(packedFunc.toList());

    if (params.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Received incompatible data:" << packedFunc;
        return;
    }

    RequestType requestType = (RequestType)params.takeFirst().value<int>();
    switch (requestType) {
        case Sync: {
            if (params.count() < 3) {
                qWarning() << Q_FUNC_INFO << "Received invalid sync call:" << params;
                return;
            }
            QByteArray className = params.takeFirst().toByteArray();
            QString objectName = params.takeFirst().toString();
            QByteArray slotName = params.takeFirst().toByteArray();
            handle(Protocol::SyncMessage(className, objectName, slotName, params));
            break;
        }
        case RpcCall: {
            if (params.empty()) {
                qWarning() << Q_FUNC_INFO << "Received empty RPC call!";
                return;
            }
            QByteArray slotName = params.takeFirst().toByteArray();
            handle(Protocol::RpcCall(slotName, params));
            break;
        }
        case InitRequest: {
            if (params.count() != 2) {
                qWarning() << Q_FUNC_INFO << "Received invalid InitRequest:" << params;
                return;
            }
            QByteArray className = params[0].toByteArray();
            QString objectName = params[1].toString();
            handle(Protocol::InitRequest(className, objectName));
            break;
        }
        case InitData: {
            if (params.count() != 3) {
                qWarning() << Q_FUNC_INFO << "Received invalid InitData:" << params;
                return;
            }
            QByteArray className = params[0].toByteArray();
            QString objectName = params[1].toString();
            QVariantMap initData = params[2].toMap();
            handle(Protocol::InitData(className, objectName, initData));
            break;
        }
        case HeartBeat: {
            if (params.count() != 1) {
                qWarning() << Q_FUNC_INFO << "Received invalid HeartBeat:" << params;
                return;
            }
            // The legacy protocol would only send a QTime, no QDateTime
            // so we assume it's sent today, which works in exactly the same cases as it did in the old implementation
            QDateTime dateTime = QDateTime::currentDateTime().toUTC();
            dateTime.setTime(params[0].toTime());
            handle(Protocol::HeartBeat(dateTime));
            break;
        }
        case HeartBeatReply: {
            if (params.count() != 1) {
                qWarning() << Q_FUNC_INFO << "Received invalid HeartBeat:" << params;
                return;
            }
            // The legacy protocol would only send a QTime, no QDateTime
            // so we assume it's sent today, which works in exactly the same cases as it did in the old implementation
            QDateTime dateTime = QDateTime::currentDateTime().toUTC();
            dateTime.setTime(params[0].toTime());
            handle(Protocol::HeartBeatReply(dateTime));
            break;
        }

    }
}


void LegacyPeer::dispatch(const Protocol::SyncMessage &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)Sync << msg.className() << msg.objectName() << msg.slotName() << msg.params());
}


void LegacyPeer::dispatch(const Protocol::RpcCall &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)RpcCall << msg.slotName() << msg.params());
}


void LegacyPeer::dispatch(const Protocol::InitRequest &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)InitRequest << msg.className() << msg.objectName());
}


void LegacyPeer::dispatch(const Protocol::InitData &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)InitData << msg.className() << msg.objectName() << msg.initData());
}


void LegacyPeer::dispatch(const Protocol::HeartBeat &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)HeartBeat << msg.timestamp().time());
}


void LegacyPeer::dispatch(const Protocol::HeartBeatReply &msg)
{
    dispatchPackedFunc(QVariantList() << (qint16)HeartBeatReply << msg.timestamp().time());
}


void LegacyPeer::dispatchPackedFunc(const QVariantList &packedFunc)
{
    writeSocketData(QVariant(packedFunc));
}

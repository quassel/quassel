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

#include "compressor.h"

#include <QTcpSocket>
#include <QTimer>

const qint64 maxBufferSize = 64 * 1024 * 1024; // protect us from zip bombs

Compressor::Compressor(QTcpSocket *socket, Compressor::CompressionLevel level, QObject *parent)
    : QObject(parent),
    _socket(socket),
    _level(level)
{
    _level = NoCompression; // compression not implemented yet

    connect(socket, SIGNAL(readyRead()), SLOT(readData()));

    // It's possible that more data has already arrived during the handshake, so readyRead() wouldn't be triggered.
    // However, we want to give RemotePeer a chance to connect to our signals, so trigger this asynchronously.
    if (socket->bytesAvailable())
        QTimer::singleShot(0, this, SLOT(readData()));
}


qint64 Compressor::bytesAvailable() const
{
    return _readBuffer.size();
}


qint64 Compressor::read(char *data, qint64 maxSize)
{
    if (maxSize <= 0)
        maxSize = _readBuffer.size();

    qint64 n = qMin(maxSize, (qint64)_readBuffer.size());
    memcpy(data, _readBuffer.constData(), n);

    // TODO: don't copy for every read
    if (n == _readBuffer.size())
        _readBuffer.clear();
    else
        _readBuffer = _readBuffer.mid(n);

    // If there's still data left in the socket buffer, make sure to schedule a read
    if (_socket->bytesAvailable())
        QTimer::singleShot(0, this, SLOT(readData()));

    return n;
}


// The usual usage pattern is to write a blocksize first, followed by the actual data.
// By setting NoFlush, one can indicate that the write buffer should not immediately be
// written, which should make things a bit more efficient.
qint64 Compressor::write(const char *data, qint64 count, WriteBufferHint flush)
{
    int pos = _writeBuffer.size();
    _writeBuffer.resize(pos + count);
    memcpy(_writeBuffer.data() + pos, data, count);

    if (flush != NoFlush)
        writeData();

    return count;
}


void Compressor::readData()
{
    // don't try to read more data if we're already closing
    if (_socket->state() !=  QAbstractSocket::ConnectedState)
        return;

    if (!_socket->bytesAvailable() || _readBuffer.size() >= maxBufferSize)
        return;

    if (compressionLevel() == NoCompression)
        _readBuffer.append(_socket->read(maxBufferSize - _readBuffer.size()));

    emit readyRead();
}


void Compressor::writeData()
{
    if (compressionLevel() == NoCompression) {
        _socket->write(_writeBuffer);
        _writeBuffer.clear();
    }
}


void Compressor::flush()
{
    if (compressionLevel() == NoCompression && _socket->state() == QAbstractSocket::ConnectedState)
        _socket->flush();

}

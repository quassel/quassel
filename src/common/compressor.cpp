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

#include "compressor.h"

#include <QTcpSocket>
#include <QTimer>

const int maxBufferSize = 64 * 1024 * 1024;  // protect us from zip bombs
const int ioBufferSize = 64 * 1024;          // chunk size for inflate/deflate; should not be too large as we preallocate that space!

Compressor::Compressor(QTcpSocket* socket, Compressor::CompressionLevel level, QObject* parent)
    : QObject(parent)
    , _socket(socket)
    , _level(level)
    , _inflater(nullptr)
    , _deflater(nullptr)
{
    connect(socket, &QIODevice::readyRead, this, &Compressor::readData);

    bool ok = true;
    if (level != NoCompression)
        ok = initStreams();

    if (!ok) {
        // something went wrong during initialization... but we can only emit an error after RemotePeer has connected its signal
        QTimer::singleShot(0, this, [this]() { emit error(); });
        return;
    }

    // It's possible that more data has already arrived during the handshake, so readyRead() wouldn't be triggered.
    // However, we want to give RemotePeer a chance to connect to our signals, so trigger this asynchronously.
    if (socket->bytesAvailable())
        QTimer::singleShot(0, this, &Compressor::readData);
}

Compressor::~Compressor()
{
    // release resources allocated by zlib
    if (_inflater) {
        inflateEnd(_inflater);
        delete _inflater;
    }
    if (_deflater) {
        deflateEnd(_deflater);
        delete _deflater;
    }
}

bool Compressor::initStreams()
{
    int zlevel;
    switch (compressionLevel()) {
    case BestCompression:
        zlevel = 9;
        break;
    case BestSpeed:
        zlevel = 1;
        break;
    default:
        zlevel = Z_DEFAULT_COMPRESSION;
    }

    _inflater = new z_stream;
    memset(_inflater, 0, sizeof(z_stream));
    if (Z_OK != inflateInit(_inflater)) {
        qWarning() << "Could not initialize the inflate stream!";
        return false;
    }

    _deflater = new z_stream;
    memset(_deflater, 0, sizeof(z_stream));
    if (Z_OK != deflateInit(_deflater, zlevel)) {
        qWarning() << "Could not initialize the deflate stream!";
        return false;
    }

    _inputBuffer.reserve(ioBufferSize);  // pre-allocate space
    _outputBuffer.resize(ioBufferSize);  // not a typo; we never change the size of this buffer anyway (we *do* for _inputBuffer!)

    qDebug() << "Enabling compression...";

    return true;
}

qint64 Compressor::bytesAvailable() const
{
    return _readBuffer.size();
}

qint64 Compressor::read(char* data, qint64 maxSize)
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
        QTimer::singleShot(0, this, &Compressor::readData);

    return n;
}

// The usual usage pattern is to write a blocksize first, followed by the actual data.
// By setting NoFlush, one can indicate that the write buffer should not immediately be
// written, which should make things a bit more efficient.
qint64 Compressor::write(const char* data, qint64 count, WriteBufferHint flush)
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
    if (_socket->state() != QAbstractSocket::ConnectedState)
        return;

    if (!_socket->bytesAvailable() || _readBuffer.size() >= maxBufferSize)
        return;

    if (compressionLevel() == NoCompression) {
        _readBuffer.append(_socket->read(maxBufferSize - _readBuffer.size()));
        emit readyRead();
        return;
    }

    // We let zlib directly append to the readBuffer, which means we pre-allocate extra space for ioBufferSize.
    // Afterwards, we'll shrink the buffer appropriately. Since shrinking should not reallocate, the readBuffer's
    // capacity should over time adapt to the largest message sizes we encounter. However, this is not a bad thing
    // considering that otherwise (using an intermediate buffer) we'd copy around data for every single message.
    // TODO: Benchmark if it would still make sense to squeeze the buffer from time to time (e.g. after initial sync)!

    while (_socket->bytesAvailable() && _readBuffer.size() + ioBufferSize < maxBufferSize && _inputBuffer.size() < ioBufferSize) {
        _readBuffer.resize(_readBuffer.size() + ioBufferSize);
        _inputBuffer.append(_socket->read(ioBufferSize - _inputBuffer.size()));

        _inflater->next_in = reinterpret_cast<unsigned char*>(_inputBuffer.data());
        _inflater->avail_in = _inputBuffer.size();
        _inflater->next_out = reinterpret_cast<unsigned char*>(_readBuffer.data() + _readBuffer.size() - ioBufferSize);
        _inflater->avail_out = ioBufferSize;

        const unsigned char* orig_out = _inflater->next_out;  // so we see if we have actually produced any output

        int status = inflate(_inflater, Z_SYNC_FLUSH);  // get as much data as possible

        // adjust input and output buffers
        _readBuffer.resize(_inflater->next_out - reinterpret_cast<unsigned char*>(_readBuffer.data()));
        if (_inflater->avail_in > 0)
            memmove(_inputBuffer.data(), _inflater->next_in, _inflater->avail_in);
        _inputBuffer.resize(_inflater->avail_in);

        if (_inflater->next_out != orig_out)
            emit readyRead();

        switch (status) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
        case Z_STREAM_ERROR:
            qWarning() << "Error while decompressing stream:" << status;
            emit error(StreamError);
            return;
        case Z_BUF_ERROR:
            // means that we need more input to continue, so this is not an actual error
            return;
        case Z_STREAM_END:
            qWarning() << "Reached end of zlib stream!";  // this should really never happen
            return;
        default:
            // just try to get more out of the stream
            break;
        }
    }
    // qDebug() << "inflate in:" << _inflater->total_in << "out:" << _inflater->total_out << "ratio:" << (double)_inflater->total_in/_inflater->total_out;
}

void Compressor::writeData()
{
    if (compressionLevel() == NoCompression) {
        _socket->write(_writeBuffer);
        _writeBuffer.clear();
        return;
    }

    _deflater->next_in = reinterpret_cast<unsigned char*>(_writeBuffer.data());
    _deflater->avail_in = _writeBuffer.size();

    int status;
    do {
        _deflater->next_out = reinterpret_cast<unsigned char*>(_outputBuffer.data());
        _deflater->avail_out = ioBufferSize;
        status = deflate(_deflater, Z_PARTIAL_FLUSH);
        if (status != Z_OK && status != Z_BUF_ERROR) {
            qWarning() << "Error while compressing stream:" << status;
            emit error(StreamError);
            return;
        }

        if (_deflater->avail_out == static_cast<unsigned int>(ioBufferSize))
            continue;  // nothing to write here

        if (!_socket->write(_outputBuffer.constData(), ioBufferSize - _deflater->avail_out)) {
            qWarning() << "Error while writing to socket:" << _socket->errorString();
            emit error(DeviceError);
            return;
        }
    } while (_deflater->avail_out == 0);  // the output buffer being full is the only reason we should have to loop here!

    if (_deflater->avail_in > 0) {
        qWarning() << "Oops, something weird happened: data still remaining in write buffer!";
        emit error(StreamError);
    }

    _writeBuffer.resize(0);

    // qDebug() << "deflate in:" << _deflater->total_in << "out:" << _deflater->total_out << "ratio:" << (double)_deflater->total_out/_deflater->total_in;
}

void Compressor::flush()
{
    if (compressionLevel() == NoCompression && _socket->state() == QAbstractSocket::ConnectedState)
        _socket->flush();

    // FIXME: missing impl for enabled compression; but then we're not using this method yet
}

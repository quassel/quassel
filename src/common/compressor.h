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

#pragma once

#include <QObject>

#include <zlib.h>

class QTcpSocket;

class Compressor : public QObject
{
    Q_OBJECT

public:
    enum CompressionLevel
    {
        NoCompression,
        DefaultCompression,
        BestCompression,
        BestSpeed
    };

    enum Error
    {
        NoError,
        StreamError,
        DeviceError
    };

    enum WriteBufferHint
    {
        NoFlush,
        Flush
    };

    Compressor(QTcpSocket* socket, CompressionLevel level, QObject* parent = nullptr);
    ~Compressor() override;

    CompressionLevel compressionLevel() const { return _level; }

    qint64 bytesAvailable() const;

    qint64 read(char* data, qint64 maxSize);
    qint64 write(const char* data, qint64 count, WriteBufferHint flush = Flush);

    void flush();

signals:
    void readyRead();
    void error(Compressor::Error errorCode = StreamError);

private slots:
    void readData();

private:
    bool initStreams();
    void writeData();

private:
    QTcpSocket* _socket;
    CompressionLevel _level;

    QByteArray _readBuffer;
    QByteArray _writeBuffer;

    QByteArray _inputBuffer;
    QByteArray _outputBuffer;

    z_streamp _inflater;
    z_streamp _deflater;
};

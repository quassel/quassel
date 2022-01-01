/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "transfer.h"

#include <utility>

Transfer::Transfer(const QUuid& uuid, QObject* parent)
    : SyncableObject(parent)
    , _status(Status::New)
    , _direction(Direction::Receive)
    , _port(0)
    , _fileSize(0)
    , _uuid(uuid)
{
    init();
}

Transfer::Transfer(
    Direction direction, QString nick, QString fileName, const QHostAddress& address, quint16 port, quint64 fileSize, QObject* parent)
    : SyncableObject(parent)
    , _status(Status::New)
    , _direction(direction)
    , _fileName(std::move(fileName))
    , _address(address)
    , _port(port)
    , _fileSize(fileSize)
    , _nick(std::move(nick))
    , _uuid(QUuid::createUuid())
{
    init();
}

void Transfer::init()
{
    static auto regTypes = []() -> bool {
        qRegisterMetaType<Status>("Transfer::Status");
        qRegisterMetaType<Direction>("Transfer::Direction");
        qRegisterMetaTypeStreamOperators<Status>("Transfer::Status");
        qRegisterMetaTypeStreamOperators<Direction>("Transfer::Direction");
        return true;
    }();
    Q_UNUSED(regTypes);

    setObjectName(QString("Transfer/%1").arg(_uuid.toString()));
    setAllowClientUpdates(true);
}

QUuid Transfer::uuid() const
{
    return _uuid;
}

Transfer::Status Transfer::status() const
{
    return _status;
}

void Transfer::setStatus(Transfer::Status status)
{
    if (_status != status) {
        _status = status;
        SYNC(ARG(status));
        emit statusChanged(status);
        if (status == Status::Completed || status == Status::Failed) {
            cleanUp();
        }
    }
}

QString Transfer::prettyStatus() const
{
    switch (status()) {
    case Status::New:
        return tr("New");
    case Status::Pending:
        return tr("Pending");
    case Status::Connecting:
        return tr("Connecting");
    case Status::Transferring:
        return tr("Transferring");
    case Status::Paused:
        return tr("Paused");
    case Status::Completed:
        return tr("Completed");
    case Status::Failed:
        return tr("Failed");
    case Status::Rejected:
        return tr("Rejected");
    }

    return QString();
}

Transfer::Direction Transfer::direction() const
{
    return _direction;
}

void Transfer::setDirection(Transfer::Direction direction)
{
    if (_direction != direction) {
        _direction = direction;
        SYNC(ARG(direction));
        emit directionChanged(direction);
    }
}

QHostAddress Transfer::address() const
{
    return _address;
}

void Transfer::setAddress(const QHostAddress& address)
{
    if (_address != address) {
        _address = address;
        SYNC(ARG(address));
        emit addressChanged(address);
    }
}

quint16 Transfer::port() const
{
    return _port;
}

void Transfer::setPort(quint16 port)
{
    if (_port != port) {
        _port = port;
        SYNC(ARG(port));
        emit portChanged(port);
    }
}

QString Transfer::fileName() const
{
    return _fileName;
}

void Transfer::setFileName(const QString& fileName)
{
    if (_fileName != fileName) {
        _fileName = fileName;
        SYNC(ARG(fileName));
        emit fileNameChanged(fileName);
    }
}

quint64 Transfer::fileSize() const
{
    return _fileSize;
}

void Transfer::setFileSize(quint64 fileSize)
{
    if (_fileSize != fileSize) {
        _fileSize = fileSize;
        SYNC(ARG(fileSize));
        emit fileSizeChanged(fileSize);
    }
}

QString Transfer::nick() const
{
    return _nick;
}

void Transfer::setNick(const QString& nick)
{
    if (_nick != nick) {
        _nick = nick;
        SYNC(ARG(nick));
        emit nickChanged(nick);
    }
}

void Transfer::setError(const QString& errorString)
{
    qWarning() << Q_FUNC_INFO << errorString;
    emit error(errorString);
    setStatus(Status::Failed);
}

QDataStream& operator<<(QDataStream& out, Transfer::Status state)
{
    out << static_cast<qint8>(state);
    return out;
}

QDataStream& operator>>(QDataStream& in, Transfer::Status& state)
{
    qint8 s;
    in >> s;
    state = static_cast<Transfer::Status>(s);
    return in;
}

QDataStream& operator<<(QDataStream& out, Transfer::Direction direction)
{
    out << static_cast<qint8>(direction);
    return out;
}

QDataStream& operator>>(QDataStream& in, Transfer::Direction& direction)
{
    qint8 d;
    in >> d;
    direction = static_cast<Transfer::Direction>(d);
    return in;
}

/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include <QHostAddress>
#include <QUuid>

#include "peer.h"
#include "syncableobject.h"

class COMMON_EXPORT Transfer : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

    Q_PROPERTY(QUuid uuid READ uuid)
    Q_PROPERTY(Transfer::Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(Transfer::Direction direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QHostAddress address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(quint64 fileSize READ fileSize WRITE setFileSize NOTIFY fileSizeChanged)
    Q_PROPERTY(QString nick READ nick WRITE setNick NOTIFY nickChanged)

public:
    enum class Status {
        New,
        Pending,
        Connecting,
        Transferring,
        Paused,
        Completed,
        Failed,
        Rejected
    };
    Q_ENUMS(State)

    enum class Direction {
        Send,
        Receive
    };
    Q_ENUMS(Direction)

    Transfer(const QUuid &uuid, QObject *parent = nullptr); // for creating a syncable object client-side
    Transfer(Direction direction, QString nick, QString fileName, const QHostAddress &address, quint16 port, quint64 size = 0, QObject *parent = nullptr);

    QUuid uuid() const;
    Status status() const;
    QString prettyStatus() const;
    Direction direction() const;
    QString fileName() const;
    QHostAddress address() const;
    quint16 port() const;
    quint64 fileSize() const;
    QString nick() const;

    virtual quint64 transferred() const = 0;

public slots:
    // called on the client side
    virtual void accept(const QString &savePath) const { Q_UNUSED(savePath); }
    virtual void reject() const {}

    // called on the core side through sync calls
    virtual void requestAccepted(PeerPtr peer) { Q_UNUSED(peer); }
    virtual void requestRejected(PeerPtr peer) { Q_UNUSED(peer); }

signals:
    void statusChanged(Transfer::Status state);
    void directionChanged(Transfer::Direction direction);
    void addressChanged(const QHostAddress &address);
    void portChanged(quint16 port);
    void fileNameChanged(const QString &fileName);
    void fileSizeChanged(quint64 fileSize);
    void transferredChanged(quint64 transferred);
    void nickChanged(const QString &nick);

    void error(const QString &errorString);

    void accepted(PeerPtr peer = nullptr) const;
    void rejected(PeerPtr peer = nullptr) const;

protected slots:
    void setStatus(Transfer::Status status);
    void setError(const QString &errorString);

    // called on the client side through sync calls
    virtual void dataReceived(PeerPtr, const QByteArray &data) { Q_UNUSED(data); }

    virtual void cleanUp() = 0;

private:
    void init();

    void setDirection(Direction direction);
    void setAddress(const QHostAddress &address);
    void setPort(quint16 port);
    void setFileName(const QString &fileName);
    void setFileSize(quint64 fileSize);
    void setNick(const QString &nick);


    Status _status;
    Direction _direction;
    QString _fileName;
    QHostAddress _address;
    quint16 _port;
    quint64 _fileSize;
    QString _nick;
    QUuid _uuid;
};

Q_DECLARE_METATYPE(Transfer::Status)
Q_DECLARE_METATYPE(Transfer::Direction)

QDataStream &operator<<(QDataStream &out, Transfer::Status state);
QDataStream &operator>>(QDataStream &in, Transfer::Status &state);
QDataStream &operator<<(QDataStream &out, Transfer::Direction direction);
QDataStream &operator>>(QDataStream &in, Transfer::Direction &direction);

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

#include <QFile>

#include "clienttransfer.h"

ClientTransfer::ClientTransfer(const QUuid &uuid, QObject *parent)
    : Transfer(uuid, parent),
    _file(0)
{
    connect(this, SIGNAL(statusChanged(Transfer::Status)), SLOT(onStatusChanged(Transfer::Status)));
}


quint64 ClientTransfer::transferred() const
{
    if (status() == Status::Completed)
        return fileSize();

    return _file ? _file->size() : 0;
}


void ClientTransfer::cleanUp()
{
    if (_file) {
        _file->close();
        _file->deleteLater();
        _file = 0;
    }
}


QString ClientTransfer::savePath() const
{
    return _savePath;
}


void ClientTransfer::accept(const QString &savePath) const
{
    _savePath = savePath;
    PeerPtr ptr = 0;
    REQUEST_OTHER(requestAccepted, ARG(ptr));
    emit accepted();
}


void ClientTransfer::reject() const
{
    PeerPtr ptr = 0;
    REQUEST_OTHER(requestRejected, ARG(ptr));
    emit rejected();
}


void ClientTransfer::dataReceived(PeerPtr, const QByteArray &data)
{
    // TODO: proper error handling (relay to core)
    if (!_file) {
        _file = new QFile(_savePath, this);
        if (!_file->open(QFile::WriteOnly|QFile::Truncate)) {
            qWarning() << Q_FUNC_INFO << "Could not open file:" << _file->errorString();
            return;
        }
    }

    if (!_file->isOpen())
        return;

    if (_file->write(data) < 0) {
        qWarning() << Q_FUNC_INFO << "Could not write to file:" << _file->errorString();
        return;
    }

    emit transferredChanged(transferred());
}


void ClientTransfer::onStatusChanged(Transfer::Status status)
{
    switch(status) {
        case Status::Completed:
            if (_file)
                _file->close();
            break;
        case Status::Failed:
            if (_file)
                _file->remove();
            break;
        default:
            ;
    }
}

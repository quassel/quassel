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

#include <QHostAddress>

#include "authhandler.h"

AuthHandler::AuthHandler(QObject *parent)
    : QObject(parent)
{

}


QTcpSocket *AuthHandler::socket() const
{
    return _socket;
}


void AuthHandler::setSocket(QTcpSocket *socket)
{
    _socket = socket;
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
}


bool AuthHandler::isLocal() const
{
    if (socket()) {
        if (socket()->peerAddress() == QHostAddress::LocalHost || socket()->peerAddress() == QHostAddress::LocalHostIPv6)
            return true;
    }
    return false;
}


// Some errors (e.g. connection refused) don't trigger a disconnected() from the socket, so send this explicitly
// (but make sure it's only sent once!)
void AuthHandler::onSocketError(QAbstractSocket::SocketError error)
{
    emit socketError(error, _socket->errorString());

    if (!socket()->isOpen() || !socket()->isValid()) {
        if (!_disconnectedSent) {
            _disconnectedSent = true;
            emit disconnected();
        }
    }
}


void AuthHandler::onSocketDisconnected()
{
    if (!_disconnectedSent) {
        _disconnectedSent = true;
        emit disconnected();
    }
}


void AuthHandler::invalidMessage()
{
    qWarning() << Q_FUNC_INFO << "No handler for message!";
}


void AuthHandler::close()
{
    if (_socket && _socket->isOpen())
        _socket->close();
}

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

#include <QHostAddress>
#include <QTimer>

#ifdef HAVE_SSL
#  include <QSslSocket>
#else
#  include <QTcpSocket>
#endif

#include "remotepeer.h"

using namespace Protocol;

RemotePeer::RemotePeer(QTcpSocket *socket, QObject *parent)
    : Peer(parent),
    _socket(socket),
    _signalProxy(0),
    _heartBeatTimer(new QTimer(this)),
    _heartBeatCount(0),
    _lag(0)
{
    socket->setParent(this);
    connect(socket, SIGNAL(disconnected()), SIGNAL(disconnected()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SIGNAL(error(QAbstractSocket::SocketError)));

#ifdef HAVE_SSL
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
    if (sslSocket)
        connect(sslSocket, SIGNAL(encrypted()), SIGNAL(secureStateChanged()));
#endif

    connect(_heartBeatTimer, SIGNAL(timeout()), SLOT(sendHeartBeat()));
}


QString RemotePeer::description() const
{
    if (socket())
        return socket()->peerAddress().toString();

    return QString();
}


::SignalProxy *RemotePeer::signalProxy() const
{
    return _signalProxy;
}


void RemotePeer::setSignalProxy(::SignalProxy *proxy)
{
    if (proxy == _signalProxy)
        return;

    if (!proxy) {
        _heartBeatTimer->stop();
        disconnect(signalProxy(), 0, this, 0);
        _signalProxy = 0;
        if (isOpen())
            close();
    }
    else {
        if (signalProxy()) {
            qWarning() << Q_FUNC_INFO << "Setting another SignalProxy not supported, ignoring!";
            return;
        }
        _signalProxy = proxy;
        connect(proxy, SIGNAL(heartBeatIntervalChanged(int)), SLOT(changeHeartBeatInterval(int)));
        _heartBeatTimer->setInterval(proxy->heartBeatInterval() * 1000);
        _heartBeatTimer->start();
    }
}


void RemotePeer::changeHeartBeatInterval(int secs)
{
    if(secs <= 0)
        _heartBeatTimer->stop();
    else {
        _heartBeatTimer->setInterval(secs * 1000);
        _heartBeatTimer->start();
    }
}


int RemotePeer::lag() const
{
    return _lag;
}


QTcpSocket *RemotePeer::socket() const
{
    return _socket;
}


bool RemotePeer::isSecure() const
{
    if (socket()) {
        if (isLocal())
            return true;
#ifdef HAVE_SSL
        QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket());
        if (sslSocket && sslSocket->isEncrypted())
            return true;
#endif
    }
    return false;
}


bool RemotePeer::isLocal() const
{
    if (socket()) {
        if (socket()->peerAddress() == QHostAddress::LocalHost || socket()->peerAddress() == QHostAddress::LocalHostIPv6)
            return true;
    }
    return false;
}


bool RemotePeer::isOpen() const
{
    return socket() && socket()->state() == QTcpSocket::ConnectedState;
}


void RemotePeer::close(const QString &reason)
{
    if (!reason.isEmpty()) {
        qWarning() << "Disconnecting:" << reason;
    }

    if (socket() && socket()->state() != QTcpSocket::UnconnectedState) {
        socket()->disconnectFromHost();
    }
}


void RemotePeer::handle(const HeartBeat &heartBeat)
{
    dispatch(HeartBeatReply(heartBeat.timestamp()));
}


void RemotePeer::handle(const HeartBeatReply &heartBeatReply)
{
    _heartBeatCount = 0;
#if QT_VERSION >= 0x040900
    emit lagUpdated(heartBeatReply.timestamp().msecsTo(QDateTime::currentDateTime().toUTC()) / 2);
#else
    emit lagUpdated(heartBeatReply.timestamp().time().msecsTo(QDateTime::currentDateTime().toUTC().time()) / 2);
#endif
}


void RemotePeer::sendHeartBeat()
{
    if (signalProxy()->maxHeartBeatCount() > 0 && _heartBeatCount >= signalProxy()->maxHeartBeatCount()) {
        qWarning() << "Disconnecting peer:" << description()
                   << "(didn't receive a heartbeat for over" << _heartBeatCount *_heartBeatTimer->interval() / 1000 << "seconds)";
        socket()->close();
        _heartBeatTimer->stop();
        return;
    }

    if (_heartBeatCount > 0) {
        _lag = _heartBeatCount * _heartBeatTimer->interval();
        emit lagUpdated(_lag);
    }

    dispatch(HeartBeat(QDateTime::currentDateTime().toUTC()));
    ++_heartBeatCount;
}

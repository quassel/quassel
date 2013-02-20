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

#include <QCoreApplication>
#include <QThread>

#include "internalpeer.h"

using namespace Protocol;

template<class T>
class PeerMessageEvent : public QEvent
{
public:
    PeerMessageEvent(InternalPeer *sender, InternalPeer::EventType eventType, const T &message)
    : QEvent(QEvent::Type(eventType)), sender(sender), message(message) {}
    InternalPeer *sender;
    T message;
};


InternalPeer::InternalPeer(QObject *parent)
    : Peer(parent),
    _proxy(0),
    _peer(0),
    _isOpen(true)
{

}


InternalPeer::~InternalPeer()
{
    if (_isOpen)
        emit disconnected();
}


QString InternalPeer::description() const
{
    return tr("internal connection");
}


bool InternalPeer::isOpen() const
{
    return true;
}


bool InternalPeer::isSecure() const
{
    return true;
}


bool InternalPeer::isLocal() const
{
    return true;
}


void InternalPeer::close(const QString &reason)
{
    // FIXME
    Q_UNUSED(reason)
    qWarning() << "closing not implemented!";
}


int InternalPeer::lag() const
{
    return 0;
}


::SignalProxy *InternalPeer::signalProxy() const
{
    return _proxy;
}


void InternalPeer::setSignalProxy(::SignalProxy *proxy)
{
    if (!proxy && _proxy) {
        _proxy = 0;
        if (_isOpen) {
            _isOpen = false;
            emit disconnected();
        }
        return;
    }

    if (proxy && !_proxy) {
        _proxy = proxy;
        return;
    }

    qWarning() << Q_FUNC_INFO << "Changing the SignalProxy is not supported!";
}


void InternalPeer::setPeer(InternalPeer *peer)
{
    if (_peer) {
        qWarning() << Q_FUNC_INFO << "Peer already set, ignoring!";
        return;
    }
    _peer = peer;
    connect(peer, SIGNAL(disconnected()), SLOT(peerDisconnected()));
}


void InternalPeer::peerDisconnected()
{
    disconnect(_peer, 0, this, 0);
    _peer = 0;
    if (_isOpen) {
        _isOpen = false;
        emit disconnected();
    }
}


void InternalPeer::dispatch(const SyncMessage &msg)
{
    dispatch(SyncMessageEvent, msg);
}


void InternalPeer::dispatch(const RpcCall &msg)
{
    dispatch(RpcCallEvent, msg);
}


void InternalPeer::dispatch(const InitRequest &msg)
{
    dispatch(InitRequestEvent, msg);
}


void InternalPeer::dispatch(const InitData &msg)
{
    dispatch(InitDataEvent, msg);
}


template<class T>
void InternalPeer::dispatch(EventType eventType, const T &msg)
{
    if (!_peer) {
        qWarning() << Q_FUNC_INFO << "Cannot dispatch a message without a peer!";
        return;
    }

    if(QThread::currentThread() == _peer->thread())
        _peer->handle(msg);
    else
        QCoreApplication::postEvent(_peer, new PeerMessageEvent<T>(this, eventType, msg));
}


void InternalPeer::customEvent(QEvent *event)
{
    switch ((int)event->type()) {
        case SyncMessageEvent: {
            PeerMessageEvent<SyncMessage> *e = static_cast<PeerMessageEvent<SyncMessage> *>(event);
            handle(e->message);
            break;
        }
        case RpcCallEvent: {
            PeerMessageEvent<RpcCall> *e = static_cast<PeerMessageEvent<RpcCall> *>(event);
            handle(e->message);
            break;
        }
        case InitRequestEvent: {
            PeerMessageEvent<InitRequest> *e = static_cast<PeerMessageEvent<InitRequest> *>(event);
            handle(e->message);
            break;
        }
        case InitDataEvent: {
            PeerMessageEvent<InitData> *e = static_cast<PeerMessageEvent<InitData> *>(event);
            handle(e->message);
            break;
        }

        default:
            qWarning() << Q_FUNC_INFO << "Received unknown custom event:" << event->type();
            return;
    }

    event->accept();
}

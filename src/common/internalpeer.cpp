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

#include "internalpeer.h"

using namespace Protocol;

InternalPeer::InternalPeer(QObject *parent)
    : Peer(nullptr, parent)
{
    static bool registered = []() {
        qRegisterMetaType<QPointer<InternalPeer>>();
        qRegisterMetaType<Protocol::SyncMessage>();
        qRegisterMetaType<Protocol::RpcCall>();
        qRegisterMetaType<Protocol::InitRequest>();
        qRegisterMetaType<Protocol::InitData>();
        return true;
    }();
    Q_UNUSED(registered)

    setFeatures(Quassel::Features{});
}


InternalPeer::~InternalPeer()
{
    if (_isOpen) {
        emit disconnected();
    }
}


QString InternalPeer::description() const
{
    return tr("internal connection");
}


QString InternalPeer::address() const
{
    return tr("internal connection");
}


quint16 InternalPeer::port() const
{
    return 0;
}


bool InternalPeer::isOpen() const
{
    return _isOpen;
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
    Q_UNUSED(reason);
    _isOpen = false;
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
        _proxy = nullptr;
        if (_isOpen) {
            _isOpen = false;
            emit disconnected();
        }
        return;
    }

    if (proxy && !_proxy) {
        _proxy = proxy;
        _isOpen = true;
        return;
    }

    qWarning() << Q_FUNC_INFO << "Changing the SignalProxy is not supported!";
}


void InternalPeer::setPeer(InternalPeer *peer)
{
    connect(peer, SIGNAL(dispatchMessage(Protocol::SyncMessage)), SLOT(handleMessage(Protocol::SyncMessage)));
    connect(peer, SIGNAL(dispatchMessage(Protocol::RpcCall))    , SLOT(handleMessage(Protocol::RpcCall)));
    connect(peer, SIGNAL(dispatchMessage(Protocol::InitRequest)), SLOT(handleMessage(Protocol::InitRequest)));
    connect(peer, SIGNAL(dispatchMessage(Protocol::InitData))   , SLOT(handleMessage(Protocol::InitData)));

    connect(peer, SIGNAL(disconnected()), SLOT(peerDisconnected()));

    _isOpen = true;
}


void InternalPeer::peerDisconnected()
{
    disconnect(sender(), nullptr, this, nullptr);
    if (_isOpen) {
        _isOpen = false;
        emit disconnected();
    }
}


void InternalPeer::dispatch(const SyncMessage &msg)
{
    emit dispatchMessage(msg);
}


void InternalPeer::dispatch(const RpcCall &msg)
{
    emit dispatchMessage(msg);
}


void InternalPeer::dispatch(const InitRequest &msg)
{
    emit dispatchMessage(msg);
}


void InternalPeer::dispatch(const InitData &msg)
{
    emit dispatchMessage(msg);
}


void InternalPeer::handleMessage(const Protocol::SyncMessage &msg)
{
    handle(msg);
}


void InternalPeer::handleMessage(const Protocol::RpcCall &msg)
{
    handle(msg);
}


void InternalPeer::handleMessage(const Protocol::InitRequest &msg)
{
    handle(msg);
}


void InternalPeer::handleMessage(const Protocol::InitData &msg)
{
    handle(msg);
}


template<class T>
void InternalPeer::handle(const T &msg)
{
    static auto setSourcePeer = [](Peer *peer) {
        auto p = SignalProxy::current();
        if (p) {
            p->setSourcePeer(peer);
        }
    };

    setSourcePeer(this);
    Peer::handle(msg);
    setSourcePeer(nullptr);
}

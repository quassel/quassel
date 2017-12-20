/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "peer.h"

Peer::Peer(AuthHandler *authHandler, QObject *parent)
    : QObject(parent)
    , _authHandler(authHandler)
{

}


AuthHandler *Peer::authHandler() const
{
    return _authHandler;
}

QDateTime Peer::connectedSince() const {
    return _connectedSince;
}

void Peer::setConnectedSince(const QDateTime &connectedSince) {
    _connectedSince = connectedSince;
}

QString Peer::buildDate() const {
    return _buildDate;
}

void Peer::setBuildDate(const QString &buildDate) {
    _buildDate = buildDate;
}

QString Peer::clientVersion() const {
    return _clientVersion;
}

void Peer::setClientVersion(const QString &clientVersion) {
    _clientVersion = clientVersion;
}

Quassel::Features Peer::features() const {
    return _features;
}

void Peer::setFeatures(Quassel::Features features) {
    _features = features;
}

int Peer::id() const {
    return _id;
}

void Peer::setId(int id) {
    _id = id;
}

// PeerPtr is used in RPC signatures for enabling receivers to send replies
// to a particular peer rather than broadcast to all connected ones.
// To enable this, the SignalProxy transparently replaces the bogus value
// received over the network with the actual address of the local Peer
// instance. Because the actual value isn't needed on the wire, it is
// serialized as null.
QDataStream &operator<<(QDataStream &out, PeerPtr ptr)
{
    Q_UNUSED(ptr);
    out << static_cast<quint64>(0);  // 64 bit for historic reasons
    return out;
}

QDataStream &operator>>(QDataStream &in, PeerPtr &ptr)
{
    ptr = nullptr;
    quint64 value;
    in >> value;
    return in;
}

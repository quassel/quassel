/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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


// Note that we need to use a fixed-size integer instead of uintptr_t, in order
// to avoid issues with different architectures for client and core.
// In practice, we'll never really have to restore the real value of a PeerPtr from
// a QVariant.
QDataStream &operator<<(QDataStream &out, PeerPtr ptr)
{
    out << reinterpret_cast<quint64>(ptr);
    return out;
}

QDataStream &operator>>(QDataStream &in, PeerPtr &ptr)
{
    quint64 value;
    in >> value;
    ptr = reinterpret_cast<PeerPtr>(value);
    return in;
}

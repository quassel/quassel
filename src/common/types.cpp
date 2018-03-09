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

#include "peer.h"
#include "types.h"

QDataStream &operator<<(QDataStream &out, const SignedId64 &signedId) {
    Q_ASSERT(SignalProxy::current());
    Q_ASSERT(SignalProxy::current()->targetPeer());

    if (SignalProxy::current()->targetPeer()->hasFeature(Quassel::Feature::LongMessageId)) {
        out << signedId.toQint64();
    } else {
        out << (qint32) signedId.toQint64();
    }
    return out;
}

QDataStream &operator>>(QDataStream &in, SignedId64 &signedId) {
    Q_ASSERT(SignalProxy::current());
    Q_ASSERT(SignalProxy::current()->sourcePeer());

    if (SignalProxy::current()->sourcePeer()->hasFeature(Quassel::Feature::LongMessageId)) {
        in >> signedId.id;
    } else {
        qint32 id;
        in >> id;
        signedId.id = id;
    }
    return in;
}

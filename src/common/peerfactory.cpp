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

#include "peerfactory.h"

#include "protocols/datastream/datastreampeer.h"
#include "protocols/legacy/legacypeer.h"

PeerFactory::ProtoList PeerFactory::supportedProtocols()
{
    ProtoList result;
    result.append(ProtoDescriptor(Protocol::DataStreamProtocol, DataStreamPeer::supportedFeatures()));
    result.append(ProtoDescriptor(Protocol::LegacyProtocol, 0));
    return result;
}

RemotePeer* PeerFactory::createPeer(
    const ProtoDescriptor& protocol, AuthHandler* authHandler, QTcpSocket* socket, Compressor::CompressionLevel level, QObject* parent)
{
    return createPeer(ProtoList() << protocol, authHandler, socket, level, parent);
}

RemotePeer* PeerFactory::createPeer(
    const ProtoList& protocols, AuthHandler* authHandler, QTcpSocket* socket, Compressor::CompressionLevel level, QObject* parent)
{
    foreach (const ProtoDescriptor& protodesc, protocols) {
        Protocol::Type proto = protodesc.first;
        quint16 features = protodesc.second;
        switch (proto) {
        case Protocol::LegacyProtocol:
            return new LegacyPeer(authHandler, socket, level, parent);
        case Protocol::DataStreamProtocol:
            if (DataStreamPeer::acceptsFeatures(features))
                return new DataStreamPeer(authHandler, socket, features, level, parent);
            break;
        default:
            break;
        }
    }

    return nullptr;
}

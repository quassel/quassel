/***************************************************************************
 *   Copyright (C) 2005-2014 by the Quassel Project                        *
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

#ifndef PEERFACTORY_H
#define PEERFACTORY_H

#include <QPair>

#include "compressor.h"
#include "protocol.h"

class QObject;
class QTcpSocket;

class AuthHandler;
class RemotePeer;

class PeerFactory
{

public:
    // second value is the protocol-specific features
    typedef QPair<Protocol::Type, quint16> ProtoDescriptor;
    typedef QVector<ProtoDescriptor> ProtoList;

    static ProtoList supportedProtocols();

    static RemotePeer *createPeer(const ProtoDescriptor &protocol, AuthHandler *authHandler, QTcpSocket *socket, Compressor::CompressionLevel level, QObject *parent = 0);
    static RemotePeer *createPeer(const ProtoList &protocols, AuthHandler *authHandler, QTcpSocket *socket, Compressor::CompressionLevel level, QObject *parent = 0);

};

#endif

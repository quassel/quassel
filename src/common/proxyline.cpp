/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "proxyline.h"

#include "ircdecoder.h"

ProxyLine ProxyLine::parseProxyLine(const QByteArray& line)
{
    ProxyLine result;

    int start = 0;
    if (line.startsWith("PROXY")) {
        start = 5;
    }
    IrcDecoder::skipEmptyParts(line, start);
    QByteArray protocol = IrcDecoder::extractFragment(line, start);
    if (protocol == "TCP4") {
        result.protocol = QAbstractSocket::IPv4Protocol;
    } else if (protocol == "TCP6") {
        result.protocol = QAbstractSocket::IPv6Protocol;
    } else {
        result.protocol = QAbstractSocket::UnknownNetworkLayerProtocol;
    }

    if (result.protocol != QAbstractSocket::UnknownNetworkLayerProtocol) {
        bool ok;
        IrcDecoder::skipEmptyParts(line, start);
        result.sourceHost = QHostAddress(QString::fromLatin1(IrcDecoder::extractFragment(line, start)));
        IrcDecoder::skipEmptyParts(line, start);
        result.sourcePort = QString::fromLatin1(IrcDecoder::extractFragment(line, start)).toUShort(&ok);
        if (!ok) result.sourcePort = 0;
        IrcDecoder::skipEmptyParts(line, start);
        result.targetHost = QHostAddress(QString::fromLatin1(IrcDecoder::extractFragment(line, start)));
        IrcDecoder::skipEmptyParts(line, start);
        result.targetPort = QString::fromLatin1(IrcDecoder::extractFragment(line, start)).toUShort(&ok);
        if (!ok) result.targetPort = 0;
    }

    return result;
}


QDebug operator<<(QDebug dbg, const ProxyLine& p) {
    dbg.nospace();
    dbg << "(protocol = " << p.protocol;
    if (p.protocol == QAbstractSocket::UnknownNetworkLayerProtocol) {
        dbg << ")";
    } else {
        dbg << ", sourceHost = " << p.sourceHost << ", sourcePort = " << p.sourcePort << ", targetHost = " << p.targetHost << ", targetPort = " << p.targetPort << ")";
    }
    return dbg.space();
}

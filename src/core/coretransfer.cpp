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

#include "coretransfer.h"

INIT_SYNCABLE_OBJECT(CoreTransfer)

CoreTransfer::CoreTransfer(Direction direction, const QString &nick, const QString &fileName, const QHostAddress &address, quint16 port, quint64 fileSize, QObject *parent)
    : Transfer(direction, nick, fileName, address, port, fileSize, parent)
{

}


void CoreTransfer::requestAccepted(PeerPtr peer)
{
    emit accepted(peer);
}


void CoreTransfer::requestRejected(PeerPtr peer)
{
    emit rejected(peer);
}

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

#include "coretransfermanager.h"

#include "coretransfer.h"

INIT_SYNCABLE_OBJECT(CoreTransferManager)
CoreTransferManager::CoreTransferManager(QObject *parent)
    : TransferManager(parent)
{
    connect(this, SIGNAL(transferAdded(const Transfer*)), SLOT(onTransferAdded(const Transfer*)));
}


CoreTransfer *CoreTransferManager::transfer(const QUuid &uuid) const
{
    return qobject_cast<CoreTransfer *>(transfer_(uuid));
}


void CoreTransferManager::addTransfer(CoreTransfer *transfer)
{
    TransferManager::addTransfer(transfer);
}


void CoreTransferManager::onTransferAdded(const Transfer *transfer)
{
    // for core-side use, publishing a non-const pointer is ok
    CoreTransfer *t = const_cast<CoreTransfer *>(qobject_cast<const CoreTransfer *>(transfer));
    if (!t) {
        qWarning() << "Invalid Transfer added to CoreTransferManager!";
        return;
    }

    emit transferAdded(t);
}

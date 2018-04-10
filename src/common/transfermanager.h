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

#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include "syncableobject.h"

#include <QUuid>

class Transfer;

class TransferManager : public SyncableObject
{
    Q_OBJECT
    SYNCABLE_OBJECT

public:
    TransferManager(QObject *parent = 0);
    inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

    QList<QUuid> transferIds() const;

signals:
    void transferAdded(const Transfer *transfer);

protected:
    Transfer *transfer_(const QUuid &uuid) const;
    void addTransfer(Transfer *transfer);

protected slots:
    virtual void onCoreTransferAdded(const QUuid &uuid) { Q_UNUSED(uuid) };

private:
    QHash<QUuid, Transfer *> _transfers;

};

#endif

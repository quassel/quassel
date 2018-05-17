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

#ifndef BACKLOGMANAGER_H
#define BACKLOGMANAGER_H

#include "syncableobject.h"
#include "types.h"

class BacklogManager : public SyncableObject
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    BacklogManager(QObject *parent = 0) : SyncableObject(parent) {}
    inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

public slots:
    virtual QVariantList requestBacklog(BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0);
    virtual QVariantList requestBacklogFiltered(BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0, int type = -1, int flags = -1);
    inline virtual void receiveBacklog(BufferId, MsgId, MsgId, int, int, QVariantList) {};
    inline virtual void receiveBacklogFiltered(BufferId, MsgId, MsgId, int, int, int, int, QVariantList) {};

    virtual QVariantList requestBacklogAll(MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0);
    virtual QVariantList requestBacklogAllFiltered(MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0, int type = -1, int flags = -1);
    inline virtual void receiveBacklogAll(MsgId, MsgId, int, int, QVariantList) {};
    inline virtual void receiveBacklogAllFiltered(MsgId, MsgId, int, int, int, int, QVariantList) {};

signals:
    void backlogRequested(BufferId, MsgId, MsgId, int, int);
    void backlogAllRequested(MsgId, MsgId, int, int);
};


#endif // BACKLOGMANAGER_H

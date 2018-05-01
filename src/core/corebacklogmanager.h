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

#ifndef COREBACKLOGMANAGER_H
#define COREBACKLOGMANAGER_H

#include "backlogmanager.h"

class CoreSession;

class CoreBacklogManager : public BacklogManager
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreBacklogManager(CoreSession *coreSession = 0);

    CoreSession *coreSession() { return _coreSession; }

public slots:
    virtual QVariantList requestBacklog(BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0);
    QVariantList requestBacklogFiltered(BufferId bufferId, MsgId first = -1, MsgId last = -1, int limit = -1,
                                        int additional = 0, int type = -1, int flags = -1) override;
    virtual QVariantList requestBacklogAll(MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0);
    QVariantList requestBacklogAllFiltered(MsgId first = -1, MsgId last = -1, int limit = -1, int additional = 0,
                                           int type = -1, int flags = -1) override;

private:
    CoreSession *_coreSession;
};


#endif // COREBACKLOGMANAGER_H

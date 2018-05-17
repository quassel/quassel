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

#include "backlogmanager.h"

INIT_SYNCABLE_OBJECT(BacklogManager)
QVariantList BacklogManager::requestBacklog(BufferId bufferId, MsgId first, MsgId last, int limit, int additional)
{
    REQUEST(ARG(bufferId), ARG(first), ARG(last), ARG(limit), ARG(additional))
    return QVariantList();
}

QVariantList BacklogManager::requestBacklogFiltered(BufferId bufferId, MsgId first, MsgId last, int limit, int additional, int type, int flags)
{
    REQUEST(ARG(bufferId), ARG(first), ARG(last), ARG(limit), ARG(additional), ARG(type), ARG(flags))
    return QVariantList();
}

QVariantList BacklogManager::requestBacklogAll(MsgId first, MsgId last, int limit, int additional)
{
    REQUEST(ARG(first), ARG(last), ARG(limit), ARG(additional))
    return QVariantList();
}

QVariantList BacklogManager::requestBacklogAllFiltered(MsgId first, MsgId last, int limit, int additional, int type, int flags)
{
    REQUEST(ARG(first), ARG(last), ARG(limit), ARG(additional), ARG(type), ARG(flags))
    return QVariantList();
}

/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "corebacklogmanager.h"
#include "core.h"
#include "coresession.h"

#include <QDebug>

CoreBacklogManager::CoreBacklogManager(CoreSession *coreSession)
  : BacklogManager(coreSession),
    _coreSession(coreSession)
{
}

QVariantList CoreBacklogManager::requestBacklog(BufferId bufferId, int limit, int offset) {
  QVariantList backlog;
  QList<Message> msgList;
  msgList = Core::requestMsgs(coreSession()->user(), bufferId, limit, offset);

  QList<Message>::const_iterator msgIter = msgList.constBegin();
  QList<Message>::const_iterator msgListEnd = msgList.constEnd();
  while(msgIter != msgListEnd) {
    backlog << qVariantFromValue(*msgIter);
    msgIter++;
  }
  return backlog;
}

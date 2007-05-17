/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#include <QtDebug>

#include "guiproxy.h"
#include "util.h"
#include "message.h"

void GUIProxy::recv(CoreSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  //qDebug() << "[GUI] Received signal:" << sig <<arg1<<arg2<<arg3;
  switch(sig) {
    case CS_CORE_STATE: emit csCoreState(arg1); break;
    case CS_SERVER_STATE: emit csServerState(arg1.toString(), arg2.toMap()); break;
    case CS_SERVER_CONNECTED: emit csServerConnected(arg1.toString()); break;
    case CS_SERVER_DISCONNECTED: emit csServerDisconnected(arg1.toString()); break;
    case CS_UPDATE_GLOBAL_DATA: emit csUpdateGlobalData(arg1.toString(), arg2); break;
    //case CS_GLOBAL_DATA_CHANGED: emit csGlobalDataChanged(arg1.toString()); break;
    case CS_DISPLAY_MSG: emit csDisplayMsg(arg1.value<Message>()); break;
    case CS_DISPLAY_STATUS_MSG: emit csDisplayStatusMsg(arg1.toString(), arg2.toString()); break;
    case CS_MODE_SET: emit csModeSet(arg1.toString(), arg2.toString(), arg3.toString()); break;
    case CS_TOPIC_SET: emit csTopicSet(arg1.toString(), arg2.toString(), arg3.toString()); break;
    case CS_NICK_ADDED: emit csNickAdded(arg1.toString(), arg2.toString(), arg3.toMap()); break;
    case CS_NICK_REMOVED: emit csNickRemoved(arg1.toString(), arg2.toString()); break;
    case CS_NICK_RENAMED: emit csNickRenamed(arg1.toString(), arg2.toString(), arg3.toString()); break;
    case CS_NICK_UPDATED: emit csNickUpdated(arg1.toString(), arg2.toString(), arg3.toMap()); break;
    case CS_OWN_NICK_SET: emit csOwnNickSet(arg1.toString(), arg2.toString()); break;
    case CS_QUERY_REQUESTED: emit csQueryRequested(arg1.toString(), arg2.toString()); break;
    case CS_BACKLOG_DATA: emit csBacklogData(arg1.value<BufferId>(), arg2.toList(), arg3.toBool()); break;
    case CS_UPDATE_BUFFERID: emit csUpdateBufferId(arg1.value<BufferId>()); break;

    //default: qWarning() << "Unknown signal in GUIProxy::recv: " << sig;
    default: emit csGeneric(sig, arg1, arg2, arg3);
  }
}

GUIProxy *guiProxy;

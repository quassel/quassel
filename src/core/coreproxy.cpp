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

#include "coreproxy.h"
#include <QtDebug>

CoreProxy::CoreProxy() {

}

void CoreProxy::recv(ClientSignal sig, QVariant arg1, QVariant arg2, QVariant arg3) {
  //qDebug() << "[CORE] Received signal" << sig << ":" << arg1<<arg2<<arg3;
  switch(sig) {
    case GS_UPDATE_GLOBAL_DATA: emit gsPutGlobalData(arg1.toString(), arg2); break;
    case GS_SESSION_DATA_CHANGED: emit gsSessionDataChanged(arg1.toString(), arg2); break;
    case GS_USER_INPUT: emit gsUserInput(arg1.value<BufferId>(), arg2.toString()); break;
    case GS_REQUEST_CONNECT: emit gsRequestConnect(arg1.toStringList()); break;
    case GS_IMPORT_BACKLOG: emit gsImportBacklog(); break;
    case GS_REQUEST_BACKLOG: emit gsRequestBacklog(arg1.value<BufferId>(), arg2, arg3); break;
    case GS_REQUEST_NETWORK_STATES: emit gsRequestNetworkStates(); break;
    //default: qWarning() << "Unknown signal in CoreProxy::recv: " << sig;
    default: emit gsGeneric(sig, arg1, arg2, arg3);
  }
}

/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "core.h"
#include "corenetwork.h"
#include "coresession.h"

#include "logger.h"

CoreNetwork::CoreNetwork(const NetworkId &networkid, CoreSession *session)
  : Network(networkid, session),
    _coreSession(session)
{
}

void CoreNetwork::requestConnect() const {
  if(connectionState() != Disconnected) {
    quWarning() << "Requesting connect while already being connected!";
    return;
  }
  emit connectRequested(networkId());
}

void CoreNetwork::requestDisconnect() const {
  if(connectionState() == Disconnected) {
    quWarning() << "Requesting disconnect while not being connected!";
    return;
  }
  emit disconnectRequested(networkId());
}

void CoreNetwork::requestSetNetworkInfo(const NetworkInfo &info) {
  setNetworkInfo(info);
  Core::updateNetwork(coreSession()->user(), info);
}

/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#include "corecoreinfo.h"

#include "core.h"
#include "coresession.h"
#include "quassel.h"
#include "signalproxy.h"

INIT_SYNCABLE_OBJECT(CoreCoreInfo)
CoreCoreInfo::CoreCoreInfo(CoreSession *parent)
  : CoreInfo(parent),
    _coreSession(parent)
{
}

QVariantMap CoreCoreInfo::coreData() const {
  QVariantMap data;
  data["quasselVersion"] = Quassel::buildInfo().fancyVersionString;
  data["quasselBuildDate"] = Quassel::buildInfo().buildDate;
  data["startTime"] = Core::instance()->startTime();
  data["sessionConnectedClients"] = _coreSession->signalProxy()->peerCount();
  return data;
}

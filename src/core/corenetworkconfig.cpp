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

#include "core.h"
#include "coresession.h"
#include "corenetworkconfig.h"

CoreNetworkConfig::CoreNetworkConfig(const QString& objectName, CoreSession* session)
  : NetworkConfig(objectName, session)
{
  setAllowClientUpdates(true);

  if(!session) {
    qWarning() << Q_FUNC_INFO << "No CoreSession set, cannot load network configuration!";
    return;
  }

  fromVariantMap(Core::getUserSetting(session->user(), objectName).toMap());
}

void CoreNetworkConfig::save() {
  CoreSession *session = qobject_cast<CoreSession *>(parent());
  if(!session) {
    qWarning() << Q_FUNC_INFO << "No CoreSession set, cannot save network configuration!";
    return;
  }

  Core::setUserSetting(session->user(), objectName(), toVariantMap());
}

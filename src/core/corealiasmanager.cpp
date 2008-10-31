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

#include "corealiasmanager.h"

#include "core.h"
#include "coresession.h"

CoreAliasManager::CoreAliasManager(CoreSession *parent)
  : AliasManager(parent)
{
  CoreSession *session = qobject_cast<CoreSession *>(parent);
  if(!session) {
    qWarning() << "CoreAliasManager: unable to load Aliases. Parent is not a Coresession!";
    loadDefaults();
    return;
  }

  QVariantMap aliases = Core::getUserSetting(session->user(), "Aliases").toMap();
  if(aliases.isEmpty()) {
    loadDefaults();
  } else {
    initSetAliases(aliases);
  }
}


CoreAliasManager::~CoreAliasManager() {
  CoreSession *session = qobject_cast<CoreSession *>(parent());
  if(!session) {
    qWarning() << "CoreAliasManager: unable to save Aliases. Parent is not a Coresession!";
    return;
  }

  Core::setUserSetting(session->user(), "Aliases", initAliases());
}

void CoreAliasManager::loadDefaults() {
  // Default Aliases:
  addAlias("j", "/join $0");

  addAlias("ns", "/msg nickserv $0");
  addAlias("nickserv", "/msg nickserv $0");
  addAlias("cs", "/msg chanserv $0");
  addAlias("chanserv",  "/msg chanserv $0");
  addAlias("hs", "/msg hostserv $0");
  addAlias("hostserv", "/msg hostserv $0");

  addAlias("back", "/quote away");
}

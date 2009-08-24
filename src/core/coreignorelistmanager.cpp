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

#include "coreignorelistmanager.h"

#include "core.h"
#include "coresession.h"

INIT_SYNCABLE_OBJECT(CoreIgnoreListManager)

CoreIgnoreListManager::CoreIgnoreListManager(CoreSession *parent)
  : IgnoreListManager(parent)
{
  CoreSession *session = qobject_cast<CoreSession *>(parent);
  if(!session) {
    qWarning() << "CoreIgnoreListManager: unable to load IgnoreList. Parent is not a Coresession!";
    //loadDefaults();
    return;
  }

  initSetIgnoreList(Core::getUserSetting(session->user(), "IgnoreList").toMap());
  //if(isEmpty())
    //loadDefaults();
}

CoreIgnoreListManager::~CoreIgnoreListManager() {
  CoreSession *session = qobject_cast<CoreSession *>(parent());
  if(!session) {
    qWarning() << "CoreIgnoreListManager: unable to save IgnoreList. Parent is not a Coresession!";
    return;
  }

  Core::setUserSetting(session->user(), "IgnoreList", initIgnoreList());
}

//void CoreIgnoreListManager::loadDefaults() {
//  foreach(IgnoreListItem item, IgnoreListManager::defaults()) {
//    addIgnoreListItem(item.ignoreRule, item.isRegEx, item.strictness, item.scope, item.scopeRule);
//  }
//}

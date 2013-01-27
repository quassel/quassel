/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "coreignorelistmanager.h"

#include "core.h"
#include "coresession.h"

INIT_SYNCABLE_OBJECT(CoreIgnoreListManager)
CoreIgnoreListManager::CoreIgnoreListManager(CoreSession *parent)
    : IgnoreListManager(parent)
{
    CoreSession *session = qobject_cast<CoreSession *>(parent);
    if (!session) {
        qWarning() << "CoreIgnoreListManager: unable to load IgnoreList. Parent is not a Coresession!";
        //loadDefaults();
        return;
    }

    initSetIgnoreList(Core::getUserSetting(session->user(), "IgnoreList").toMap());

    // we store our settings whenever they change
    connect(this, SIGNAL(updatedRemotely()), SLOT(save()));

    //if(isEmpty())
    //loadDefaults();
}


IgnoreListManager::StrictnessType CoreIgnoreListManager::match(const RawMessage &rawMsg, const QString &networkName)
{
    //StrictnessType _match(const QString &msgContents, const QString &msgSender, Message::Type msgType, const QString &network, const QString &bufferName);
    return _match(rawMsg.text, rawMsg.sender, rawMsg.type, networkName, rawMsg.target);
}


void CoreIgnoreListManager::save() const
{
    CoreSession *session = qobject_cast<CoreSession *>(parent());
    if (!session) {
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

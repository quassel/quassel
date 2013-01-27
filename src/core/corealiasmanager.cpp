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

#include "corealiasmanager.h"

#include "core.h"
#include "corenetwork.h"
#include "coresession.h"

INIT_SYNCABLE_OBJECT(CoreAliasManager)
CoreAliasManager::CoreAliasManager(CoreSession *parent)
    : AliasManager(parent)
{
    CoreSession *session = qobject_cast<CoreSession *>(parent);
    if (!session) {
        qWarning() << "CoreAliasManager: unable to load Aliases. Parent is not a Coresession!";
        loadDefaults();
        return;
    }

    initSetAliases(Core::getUserSetting(session->user(), "Aliases").toMap());
    if (isEmpty())
        loadDefaults();

    // we store our settings whenever they change
    connect(this, SIGNAL(updatedRemotely()), SLOT(save()));
}


void CoreAliasManager::save() const
{
    CoreSession *session = qobject_cast<CoreSession *>(parent());
    if (!session) {
        qWarning() << "CoreAliasManager: unable to save Aliases. Parent is not a Coresession!";
        return;
    }

    Core::setUserSetting(session->user(), "Aliases", initAliases());
}


const Network *CoreAliasManager::network(NetworkId id) const
{
    return qobject_cast<CoreSession *>(parent())->network(id);
}


void CoreAliasManager::loadDefaults()
{
    foreach(Alias alias, AliasManager::defaults()) {
        addAlias(alias.name, alias.expansion);
    }
}

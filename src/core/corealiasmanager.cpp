/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

CoreAliasManager::CoreAliasManager(CoreSession* parent)
    : AliasManager(parent)
{
    auto* session = qobject_cast<CoreSession*>(parent);
    if (!session) {
        qWarning() << "CoreAliasManager: unable to load Aliases. Parent is not a Coresession!";
        loadDefaults();
        return;
    }

    initSetAliases(Core::getUserSetting(session->user(), "Aliases").toMap());
    if (isEmpty())
        loadDefaults();

    // we store our settings whenever they change
    connect(this, &SyncableObject::updatedRemotely, this, &CoreAliasManager::save);
}

void CoreAliasManager::save() const
{
    auto* session = qobject_cast<CoreSession*>(parent());
    if (!session) {
        qWarning() << "CoreAliasManager: unable to save Aliases. Parent is not a Coresession!";
        return;
    }

    Core::setUserSetting(session->user(), "Aliases", initAliases());
}

const Network* CoreAliasManager::network(NetworkId id) const
{
    return qobject_cast<CoreSession*>(parent())->network(id);
}

void CoreAliasManager::loadDefaults()
{
    for (const Alias& alias : AliasManager::defaults()) {
        addAlias(alias.name, alias.expansion);
    }
}

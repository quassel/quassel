/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

#include "corehighlightrulemanager.h"

#include "core.h"
#include "coresession.h"

constexpr auto settingsKey = "HighlightRuleList";

INIT_SYNCABLE_OBJECT(CoreHighlightRuleManager)
CoreHighlightRuleManager::CoreHighlightRuleManager(CoreSession *session)
    : HighlightRuleManager(session)
    , _coreSession{session}
{
    // Load config from database if it exists
    auto configMap = Core::getUserSetting(session->user(), settingsKey).toMap();
    if (!configMap.isEmpty())
        update(configMap);
    // Otherwise, we just use the defaults initialized in the base class

    // We store our settings whenever they change
    connect(this, SIGNAL(updatedRemotely()), SLOT(save()));
}

void CoreHighlightRuleManager::save()
{
    Core::setUserSetting(_coreSession->user(), settingsKey, toVariantMap());
}

bool CoreHighlightRuleManager::match(const RawMessage &msg, const QString &currentNick,
                                     const QStringList &identityNicks)
{
    return match(msg.networkId, msg.text, msg.sender, msg.type, msg.flags, msg.target, currentNick,
                 identityNicks);
}

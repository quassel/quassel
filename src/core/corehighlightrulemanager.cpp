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

INIT_SYNCABLE_OBJECT(CoreHighlightRuleManager)
CoreHighlightRuleManager::CoreHighlightRuleManager(CoreSession *parent)
    : HighlightRuleManager(parent)
{
    CoreSession *session = qobject_cast<CoreSession*>(parent);
    if (!session) {
        qWarning() << "CoreHighlightRuleManager: unable to load HighlightRuleList. Parent is not a Coresession!";
        //loadDefaults();
        return;
    }

    initSetHighlightRuleList(Core::getUserSetting(session->user(), "HighlightRuleList").toMap());

    // we store our settings whenever they change
    connect(this, SIGNAL(updatedRemotely()), SLOT(save()));
}

void CoreHighlightRuleManager::save() const
{
    CoreSession *session = qobject_cast<CoreSession *>(parent());
    if (!session) {
        qWarning() << "CoreHighlightRuleManager: unable to save HighlightRuleList. Parent is not a Coresession!";
        return;
    }

    Core::setUserSetting(session->user(), "HighlightRuleList", initHighlightRuleList());
}

bool CoreHighlightRuleManager::match(const RawMessage &msg, const QString &currentNick, const QStringList &identityNicks)
{
    return match(msg.text, msg.sender, msg.type, msg.flags, msg.target, currentNick, identityNicks);
}

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

#include "clientignorelistmanager.h"

ClientIgnoreListManager::ClientIgnoreListManager(QObject* parent)
    : IgnoreListManager(parent)
{
    connect(this, &SyncableObject::updatedRemotely, this, &ClientIgnoreListManager::ignoreListChanged);
}

bool ClientIgnoreListManager::pureMatch(const IgnoreListItem& item, const QString& string) const
{
    return (item.contentsMatcher().match(string));
}

QMap<QString, bool> ClientIgnoreListManager::matchingRulesForHostmask(const QString& hostmask,
                                                                      const QString& network,
                                                                      const QString& channel) const
{
    QMap<QString, bool> result;
    foreach (IgnoreListItem item, ignoreList()) {
        if (item.type() == SenderIgnore && pureMatch(item, hostmask)
            && ((network.isEmpty() && channel.isEmpty()) || item.scope() == GlobalScope
                || (item.scope() == NetworkScope && item.scopeRuleMatcher().match(network))
                || (item.scope() == ChannelScope && item.scopeRuleMatcher().match(channel)))) {
            result[item.contents()] = item.isEnabled();
            // qDebug() << "matchingRulesForHostmask found: " << item.contents()
            //         << "is active: " << item.isActive;
        }
    }
    return result;
}

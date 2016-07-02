/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include <QRegExp>

INIT_SYNCABLE_OBJECT(ClientIgnoreListManager)

ClientIgnoreListManager::ClientIgnoreListManager(QObject *parent)
    : IgnoreListManager(parent)
{
    connect(this, SIGNAL(updatedRemotely()), SIGNAL(ignoreListChanged()));
}


bool ClientIgnoreListManager::pureMatch(const IgnoreListItem &item, const QString &string) const
{
    QRegExp ruleRx = QRegExp(item.ignoreRule);
    ruleRx.setCaseSensitivity(Qt::CaseInsensitive);
    if (!item.isRegEx)
        ruleRx.setPatternSyntax(QRegExp::Wildcard);

    if ((!item.isRegEx && ruleRx.exactMatch(string)) ||
        (item.isRegEx && ruleRx.indexIn(string) != -1))
        return true;
    return false;
}


QMap<QString, bool> ClientIgnoreListManager::matchingRulesForHostmask(const QString &hostmask, const QString &network, const QString &channel) const
{
    QMap<QString, bool> result;
    foreach(IgnoreListItem item, ignoreList()) {
        if (item.type == SenderIgnore && pureMatch(item, hostmask)
            && ((network.isEmpty() && channel.isEmpty()) || item.scope == GlobalScope || (item.scope == NetworkScope && scopeMatch(item.scopeRegex, network))
                || (item.scope == ChannelScope && scopeMatch(item.scopeRegex, channel)))) {
            result[item.ignoreRule] = item.isActive;
//      qDebug() << "matchingRulesForHostmask found: " << item.ignoreRule << "is active: " << item.isActive;
        }
    }
    return result;
}

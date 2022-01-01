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

#include "nickhighlightmatcher.h"

#include <QDebug>
#include <QString>
#include <QStringList>

bool NickHighlightMatcher::match(const QString& string, const NetworkId& netId, const QString& currentNick, const QStringList& identityNicks) const
{
    // Never match for no nicknames
    if (_highlightMode == HighlightNickType::NoNick) {
        return false;
    }

    // Don't match until current nickname is known
    if (currentNick.isEmpty()) {
        return false;
    }

    // Make sure expression matcher is ready
    determineExpressions(netId, currentNick, identityNicks);

    // Check for a match
    if (_nickMatchCache[netId].matcher.isValid() && _nickMatchCache[netId].matcher.match(string)) {
        // Nick matcher is valid and match found
        return true;
    }

    return false;
}

void NickHighlightMatcher::determineExpressions(const NetworkId& netId, const QString& currentNick, const QStringList& identityNicks) const
{
    // Don't do anything for no nicknames
    if (_highlightMode == HighlightNickType::NoNick) {
        return;
    }

    // Only update if needed (check nickname config, current nick, identity nicks for change)
    if (_nickMatchCache.contains(netId) && _nickMatchCache[netId].nickCurrent == currentNick
        && _nickMatchCache[netId].identityNicks == identityNicks) {
        return;
    }

    // Add all nicknames
    QStringList nickList;
    if (_highlightMode == HighlightNickType::CurrentNick) {
        nickList << currentNick;
    }
    else if (_highlightMode == HighlightNickType::AllNicks) {
        nickList = identityNicks;
        if (!nickList.contains(currentNick))
            nickList.prepend(currentNick);
    }

    // Set up phrase matcher, joining with newlines
    _nickMatchCache[netId].matcher = ExpressionMatch(nickList.join("\n"), ExpressionMatch::MatchMode::MatchMultiPhrase, _isCaseSensitive);

    _nickMatchCache[netId].nickCurrent = currentNick;
    _nickMatchCache[netId].identityNicks = identityNicks;

    qDebug() << "Regenerated nickname matching cache for network ID" << netId;
}

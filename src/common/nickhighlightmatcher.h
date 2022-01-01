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

#pragma once

#include "common-export.h"

#include <QHash>
#include <QString>
#include <QStringList>

#include "expressionmatch.h"
#include "types.h"

/**
 * Nickname matcher with automatic caching for performance
 */
class COMMON_EXPORT NickHighlightMatcher
{
public:
    /// Nickname highlighting mode
    enum class HighlightNickType
    {
        NoNick = 0x00,       ///< Don't match any nickname
        CurrentNick = 0x01,  ///< Match the current nickname
        AllNicks = 0x02      ///< Match all configured nicknames in the chosen identity
    };
    // NOTE: Keep this in sync with HighlightRuleManager::HighlightNickType and
    // NotificationSettings::HighlightNickType!

    /**
     * Construct an empty NicknameMatcher
     */
    NickHighlightMatcher() = default;

    /**
     * Construct a configured NicknameMatcher
     *
     * @param highlightMode    Nickname highlighting mode
     * @param isCaseSensitive  If true, nick matching is case-sensitive, otherwise case-insensitive
     */
    NickHighlightMatcher(HighlightNickType highlightMode, bool isCaseSensitive)
        : _highlightMode(highlightMode)
        , _isCaseSensitive(isCaseSensitive)
    {}

    /**
     * Gets the nickname highlighting policy
     *
     * @return HighlightNickType for the given network
     */
    inline HighlightNickType highlightMode() const { return _highlightMode; }

    /**
     * Sets the nickname highlighting policy
     *
     * @param highlightMode Nickname highlighting mode
     */
    void setHighlightMode(HighlightNickType highlightMode)
    {
        if (_highlightMode != highlightMode) {
            _highlightMode = highlightMode;
            invalidateNickCache();
        }
    }

    /**
     * Gets the nickname case-sensitivity policy
     *
     * @return True if nickname highlights are case-sensitive, otherwise false
     */
    inline bool isCaseSensitive() const { return _isCaseSensitive; }

    /**
     * Sets the nickname case-sensitivity policy
     *
     * @param isCaseSensitive If true, nick matching is case-sensitive, otherwise case-insensitive
     */
    void setCaseSensitive(bool isCaseSensitive)
    {
        if (_isCaseSensitive != isCaseSensitive) {
            _isCaseSensitive = isCaseSensitive;
            invalidateNickCache();
        }
    }

    /**
     * Checks if the given string matches the specified network's nickname matcher
     *
     * Updates cache when called if needed.
     *
     * @param string         String to match against
     * @param netId          Network ID of source network
     * @param currentNick    Current nickname
     * @param identityNicks  All nicknames configured for the current identity
     * @return True if match found, otherwise false
     */
    bool match(const QString& string, const NetworkId& netId, const QString& currentNick, const QStringList& identityNicks) const;

public slots:
    /**
     * Removes the specified network ID from the cache
     *
     * @param netId Network ID of source network
     */
    void removeNetwork(const NetworkId& netId)
    {
        // Remove the network from the cache list
        if (_nickMatchCache.remove(netId) > 0) {
            qDebug() << "Cleared nickname matching cache for removed network ID" << netId;
        }
    }

private:
    struct NickMatchCache
    {
        // These represent internal cache and should be safe to mutate in 'const' functions
        QString nickCurrent = {};        ///< Last cached current nick
        QStringList identityNicks = {};  ///< Last cached identity nicks
        ExpressionMatch matcher = {};    ///< Expression match cache for nicks
    };

    /**
     * Update internal cache of nickname matching if needed
     *
     * @param netId          Network ID of source network
     * @param currentNick    Current nickname
     * @param identityNicks  All nicknames configured for the current identity
     */
    void determineExpressions(const NetworkId& netId, const QString& currentNick, const QStringList& identityNicks) const;

    /**
     * Invalidate all nickname match caches
     *
     * Use this after changing global configuration.
     */
    inline void invalidateNickCache()
    {
        // Mark all as invalid
        if (_nickMatchCache.size() > 0) {
            _nickMatchCache.clear();
            qDebug() << "Cleared all nickname matching cache (settings changed)";
        }
    }

    // Global nickname configuration
    /// Nickname highlighting mode
    HighlightNickType _highlightMode = HighlightNickType::CurrentNick;
    bool _isCaseSensitive = false;  ///< If true, match nicknames with exact case

    // These represent internal cache and should be safe to mutate in 'const' functions
    mutable QHash<NetworkId, NickMatchCache> _nickMatchCache;  ///< Per-network nick matching cache
};

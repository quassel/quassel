/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "highlightrulemanager.h"

class CoreSession;
struct RawMessage;

/**
 * Core-side specialization for HighlightRuleManager.
 *
 * Adds the ability to load/save the settings from/to the database.
 */
class CoreHighlightRuleManager : public HighlightRuleManager
{
    Q_OBJECT

    using HighlightRuleManager::match;

public:
    /**
     * Constructor.
     *
     * @param[in] session Pointer to the parent CoreSession (takes ownership)
     */
    explicit CoreHighlightRuleManager(CoreSession* session);

    bool match(const RawMessage& msg, const QString& currentNick, const QStringList& identityNicks);

public slots:
    inline void requestToggleHighlightRule(int highlightRule) override { toggleHighlightRule(highlightRule); }
    inline void requestRemoveHighlightRule(int highlightRule) override { removeHighlightRule(highlightRule); }
    inline void requestAddHighlightRule(int id,
                                        const QString& name,
                                        bool isRegEx,
                                        bool isCaseSensitive,
                                        bool isEnabled,
                                        bool isInverse,
                                        const QString& sender,
                                        const QString& chanName) override
    {
        addHighlightRule(id, name, isRegEx, isCaseSensitive, isEnabled, isInverse, sender, chanName);
    }

private slots:
    /**
     * Saves the config to the database.
     */
    void save();

private:
    CoreSession* _coreSession{nullptr};  ///< Pointer to the parent CoreSession
};

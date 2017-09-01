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

#ifndef COREHIGHLIGHTRULEMANAHER_H
#define COREHIGHLIGHTRULEMANAHER_H

#include "highlightrulemanager.h"

class CoreSession;
struct RawMessage;

class CoreHighlightRuleManager : public HighlightRuleManager
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    explicit CoreHighlightRuleManager(CoreSession *parent);

    inline virtual const QMetaObject *syncMetaObject() const { return &HighlightRuleManager::staticMetaObject; }

    bool match(const RawMessage &msg, const QString &currentNick, const QStringList &identityNicks);
public slots:
    virtual inline void requestToggleHighlightRule(const QString &highlightRule) { toggleHighlightRule(highlightRule); }
    virtual inline void requestRemoveHighlightRule(const QString &highlightRule) { removeHighlightRule(highlightRule); }
    virtual inline void requestAddHighlightRule(const QString &name, bool isRegEx, bool isCaseSensitive,
                                                 bool isEnabled, const QString &chanName)
    {
        addHighlightRule(name, isRegEx, isCaseSensitive, isEnabled, chanName);
    }


private slots:
    void save() const;
};


#endif //COREHIGHLIGHTRULEMANAHER_H

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

#include "highlightrulemanager.h"

#include <QDebug>

#include "expressionmatch.h"
#include "util.h"

HighlightRuleManager& HighlightRuleManager::operator=(const HighlightRuleManager& other)
{
    if (this == &other)
        return *this;

    SyncableObject::operator=(other);
    _highlightRuleList = other._highlightRuleList;
    _nicksCaseSensitive = other._nicksCaseSensitive;
    _highlightNick = other._highlightNick;
    return *this;
}

int HighlightRuleManager::indexOf(int id) const
{
    for (int i = 0; i < _highlightRuleList.count(); i++) {
        if (_highlightRuleList[i].id() == id)
            return i;
    }
    return -1;
}

int HighlightRuleManager::nextId()
{
    int max = 0;
    for (int i = 0; i < _highlightRuleList.count(); i++) {
        int id = _highlightRuleList[i].id();
        if (id > max) {
            max = id;
        }
    }
    return max + 1;
}

QVariantMap HighlightRuleManager::highlightRulesToMap() const
{
    QVariantList id;
    QVariantMap highlightRuleListMap;
    QStringList name;
    QVariantList isRegEx;
    QVariantList isCaseSensitive;
    QVariantList isActive;
    QVariantList isInverse;
    QStringList sender;
    QStringList channel;

    for (int i = 0; i < _highlightRuleList.count(); i++) {
        id << _highlightRuleList[i].id();
        name << _highlightRuleList[i].contents();
        isRegEx << _highlightRuleList[i].isRegEx();
        isCaseSensitive << _highlightRuleList[i].isCaseSensitive();
        isActive << _highlightRuleList[i].isEnabled();
        isInverse << _highlightRuleList[i].isInverse();
        sender << _highlightRuleList[i].sender();
        channel << _highlightRuleList[i].chanName();
    }

    highlightRuleListMap["id"] = id;
    highlightRuleListMap["name"] = name;
    highlightRuleListMap["isRegEx"] = isRegEx;
    highlightRuleListMap["isCaseSensitive"] = isCaseSensitive;
    highlightRuleListMap["isEnabled"] = isActive;
    highlightRuleListMap["isInverse"] = isInverse;
    highlightRuleListMap["sender"] = sender;
    highlightRuleListMap["channel"] = channel;
    return highlightRuleListMap;
}

void HighlightRuleManager::highlightRulesFromMap(const QVariantMap& highlightRuleList)
{
    QVariantList id = highlightRuleList["id"].toList();
    QStringList name = highlightRuleList["name"].toStringList();
    QVariantList isRegEx = highlightRuleList["isRegEx"].toList();
    QVariantList isCaseSensitive = highlightRuleList["isCaseSensitive"].toList();
    QVariantList isActive = highlightRuleList["isEnabled"].toList();
    QVariantList isInverse = highlightRuleList["isInverse"].toList();
    QStringList sender = highlightRuleList["sender"].toStringList();
    QStringList channel = highlightRuleList["channel"].toStringList();

    int count = id.count();
    if (count != name.count() || count != isRegEx.count() || count != isCaseSensitive.count() || count != isActive.count()
        || count != isInverse.count() || count != sender.count() || count != channel.count()) {
        qWarning() << "Corrupted HighlightRuleList settings! (Count mismatch)";
        return;
    }

    _highlightRuleList.clear();
    for (int i = 0; i < name.count(); i++) {
        _highlightRuleList << HighlightRule(id[i].toInt(),
                                            name[i],
                                            isRegEx[i].toBool(),
                                            isCaseSensitive[i].toBool(),
                                            isActive[i].toBool(),
                                            isInverse[i].toBool(),
                                            sender[i],
                                            channel[i]);
    }
}

void HighlightRuleManager::addHighlightRule(
    int id, const QString& name, bool isRegEx, bool isCaseSensitive, bool isActive, bool isInverse, const QString& sender, const QString& channel)
{
    if (contains(id)) {
        return;
    }

    HighlightRule newItem = HighlightRule(id, name, isRegEx, isCaseSensitive, isActive, isInverse, sender, channel);
    _highlightRuleList << newItem;

    SYNC(ARG(id), ARG(name), ARG(isRegEx), ARG(isCaseSensitive), ARG(isActive), ARG(isInverse), ARG(sender), ARG(channel))
}

bool HighlightRuleManager::match(const NetworkId& netId,
                                 const QString& msgContents,
                                 const QString& msgSender,
                                 Message::Type msgType,
                                 Message::Flags msgFlags,
                                 const QString& bufferName,
                                 const QString& currentNick,
                                 const QStringList& identityNicks)
{
    if (!((msgType & (Message::Plain | Message::Notice | Message::Action)) && !(msgFlags & Message::Self))) {
        return false;
    }

    bool matches = false;

    for (int i = 0; i < _highlightRuleList.count(); i++) {
        auto& rule = _highlightRuleList.at(i);
        if (!rule.isEnabled())
            continue;

        // Skip if channel name doesn't match and channel rule is not empty
        //
        // Match succeeds if...
        //   Channel name matches a defined rule
        //   Defined rule is empty
        // And take the inverse of the above
        if (!rule.chanNameMatcher().match(bufferName, true)) {
            // A channel name rule is specified and does NOT match the current buffer name, skip
            // this rule
            continue;
        }

        // Check message according to specified rule, allowing empty rules to match
        bool contentsMatch = rule.contentsMatcher().match(stripFormatCodes(msgContents), true);

        // Check sender according to specified rule, allowing empty rules to match
        bool senderMatch = rule.senderMatcher().match(msgSender, true);

        if (contentsMatch && senderMatch) {
            // If an inverse rule matches, then we know that we never want to return a highlight.
            if (rule.isInverse()) {
                return false;
            }
            else {
                matches = true;
            }
        }
    }

    if (matches)
        return true;

    // Check nicknames
    if (_highlightNick != HighlightNickType::NoNick && !currentNick.isEmpty()) {
        // Nickname matching allowed and current nickname is known
        // Run the nickname matcher on the unformatted string
        if (_nickMatcher.match(stripFormatCodes(msgContents), netId, currentNick, identityNicks)) {
            return true;
        }
    }

    return false;
}

void HighlightRuleManager::removeHighlightRule(int highlightRule)
{
    removeAt(indexOf(highlightRule));
    SYNC(ARG(highlightRule))
}

void HighlightRuleManager::toggleHighlightRule(int highlightRule)
{
    int idx = indexOf(highlightRule);
    if (idx == -1)
        return;
    _highlightRuleList[idx].setIsEnabled(!_highlightRuleList[idx].isEnabled());
    SYNC(ARG(highlightRule))
}

bool HighlightRuleManager::match(const Message& msg, const QString& currentNick, const QStringList& identityNicks)
{
    return match(msg.bufferInfo().networkId(),
                 msg.contents(),
                 msg.sender(),
                 msg.type(),
                 msg.flags(),
                 msg.bufferInfo().bufferName(),
                 currentNick,
                 identityNicks);
}

/**************************************************************************
 * HighlightRule
 *************************************************************************/
bool HighlightRuleManager::HighlightRule::operator!=(const HighlightRule& other) const
{
    return (_id != other._id || _contents != other._contents || _isRegEx != other._isRegEx || _isCaseSensitive != other._isCaseSensitive
            || _isEnabled != other._isEnabled || _isInverse != other._isInverse || _sender != other._sender || _chanName != other._chanName);
    // Don't compare ExpressionMatch objects as they are created as needed from the above
}

void HighlightRuleManager::HighlightRule::determineExpressions() const
{
    // Don't update if not needed
    if (!_cacheInvalid) {
        return;
    }

    // Set up matching rules
    // Message is either phrase or regex
    ExpressionMatch::MatchMode contentsMode = _isRegEx ? ExpressionMatch::MatchMode::MatchRegEx : ExpressionMatch::MatchMode::MatchPhrase;
    // Sender and channel are either multiple wildcard entries or regex
    ExpressionMatch::MatchMode scopeMode = _isRegEx ? ExpressionMatch::MatchMode::MatchRegEx : ExpressionMatch::MatchMode::MatchMultiWildcard;

    _contentsMatch = ExpressionMatch(_contents, contentsMode, _isCaseSensitive);
    _senderMatch = ExpressionMatch(_sender, scopeMode, _isCaseSensitive);
    _chanNameMatch = ExpressionMatch(_chanName, scopeMode, _isCaseSensitive);

    _cacheInvalid = false;
}

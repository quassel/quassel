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

#include "util.h"

#include <QDebug>

INIT_SYNCABLE_OBJECT(HighlightRuleManager)
HighlightRuleManager &HighlightRuleManager::operator=(const HighlightRuleManager &other)
{
    if (this == &other)
        return *this;

    SyncableObject::operator=(other);
    _highlightRuleList = other._highlightRuleList;
    _nicksCaseSensitive = other._nicksCaseSensitive;
    _highlightNick = other._highlightNick;
    return *this;
}


int HighlightRuleManager::indexOf(const QString &name) const
{
    for (int i = 0; i < _highlightRuleList.count(); i++) {
        if (_highlightRuleList[i].name == name)
            return i;
    }
    return -1;
}


QVariantMap HighlightRuleManager::initHighlightRuleList() const
{
    QVariantMap highlightRuleListMap;
    QStringList name;
    QVariantList isRegEx;
    QVariantList isCaseSensitive;
    QVariantList isActive;
    QVariantList isInverse;
    QStringList sender;
    QStringList channel;

    for (int i = 0; i < _highlightRuleList.count(); i++) {
        name << _highlightRuleList[i].name;
        isRegEx << _highlightRuleList[i].isRegEx;
        isCaseSensitive << _highlightRuleList[i].isCaseSensitive;
        isActive << _highlightRuleList[i].isEnabled;
        isInverse << _highlightRuleList[i].isInverse;
        sender << _highlightRuleList[i].sender;
        channel << _highlightRuleList[i].chanName;
    }

    highlightRuleListMap["name"] = name;
    highlightRuleListMap["isRegEx"] = isRegEx;
    highlightRuleListMap["isCaseSensitive"] = isCaseSensitive;
    highlightRuleListMap["isEnabled"] = isActive;
    highlightRuleListMap["isInverse"] = isInverse;
    highlightRuleListMap["sender"] = sender;
    highlightRuleListMap["channel"] = channel;
    highlightRuleListMap["highlightNick"] = _highlightNick;
    highlightRuleListMap["nicksCaseSensitive"] = _nicksCaseSensitive;
    return highlightRuleListMap;
}


void HighlightRuleManager::initSetHighlightRuleList(const QVariantMap &highlightRuleList)
{
    QStringList name = highlightRuleList["name"].toStringList();
    QVariantList isRegEx = highlightRuleList["isRegEx"].toList();
    QVariantList isCaseSensitive = highlightRuleList["isCaseSensitive"].toList();
    QVariantList isActive = highlightRuleList["isEnabled"].toList();
    QVariantList isInverse = highlightRuleList["isInverse"].toList();
    QStringList sender = highlightRuleList["sender"].toStringList();
    QStringList channel = highlightRuleList["channel"].toStringList();

    int count = name.count();
    if (count != isRegEx.count() || count != isCaseSensitive.count() || count != isActive.count() ||
        count != isInverse.count() || count != sender.count() || count != channel.count()) {
        qWarning() << "Corrupted HighlightRuleList settings! (Count mismatch)";
        return;
    }

    _highlightRuleList.clear();
    for (int i = 0; i < name.count(); i++) {
        _highlightRuleList << HighlightRule(name[i], isRegEx[i].toBool(), isCaseSensitive[i].toBool(),
                                            isActive[i].toBool(), isInverse[i].toBool(), sender[i], channel[i]);
    }
    _highlightNick = HighlightNickType(highlightRuleList["highlightNick"].toInt());
    _nicksCaseSensitive = highlightRuleList["nicksCaseSensitive"].toBool();
}

void HighlightRuleManager::addHighlightRule(const QString &name, bool isRegEx, bool isCaseSensitive, bool isActive,
                                            bool isInverse, const QString &sender, const QString &channel)
{
    if (contains(name)) {
        return;
    }

    HighlightRule newItem = HighlightRule(name, isRegEx, isCaseSensitive, isActive, isInverse, sender, channel);
    _highlightRuleList << newItem;

    SYNC(ARG(name), ARG(isRegEx), ARG(isCaseSensitive), ARG(isActive), ARG(isInverse), ARG(sender), ARG(channel))
}


bool HighlightRuleManager::match(const QString &msgContents,
                                 const QString &msgSender,
                                 Message::Type msgType,
                                 Message::Flags msgFlags,
                                 const QString &bufferName,
                                 const QString &currentNick,
                                 const QStringList identityNicks)
{
    if (!((msgType & (Message::Plain | Message::Notice | Message::Action)) && !(msgFlags & Message::Self))) {
       return false;
    }

    bool matches = false;

    for (int i = 0; i < _highlightRuleList.count(); i++) {
        const HighlightRule &rule = _highlightRuleList.at(i);
        if (!rule.isEnabled)
            continue;

        if (!rule.chanName.isEmpty() && !scopeMatch(rule.chanName, bufferName)) {
            // A channel name rule is specified and does NOT match the current buffer name, skip
            // this rule
            continue;
        }

        QRegExp rx;
        if (rule.isRegEx) {
            rx = QRegExp(rule.name, rule.isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        } else {
            rx = QRegExp("(^|\\W)" + QRegExp::escape(rule.name) + "(\\W|$)", rule.isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        }
        bool nameMatch = (rx.indexIn(stripFormatCodes(msgContents)) >= 0);

        bool senderMatch;
        if (rule.sender.isEmpty()) {
            senderMatch = true;
        } else {
            if (rule.isRegEx) {
                rx = QRegExp(rule.sender, rule.isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
            } else {
                rx = QRegExp(rule.sender, Qt::CaseInsensitive, QRegExp::Wildcard);
            }
            senderMatch = rx.exactMatch(msgSender);
        }

        if (nameMatch && senderMatch) {
            // If an inverse rule matches, then we know that we never want to return a highlight.
            if (rule.isInverse) {
                return false;
            } else {
                matches = true;
            }
        }
    }

    if (matches)
        return true;

    if (!currentNick.isEmpty()) {
        QStringList nickList;
        if (_highlightNick == CurrentNick) {
            nickList << currentNick;
        }
        else if (_highlightNick == AllNicks) {
            nickList = identityNicks;
            if (!nickList.contains(currentNick))
                nickList.prepend(currentNick);
        }

        for(const QString &nickname : nickList) {
            QRegExp nickRegExp("(^|\\W)" + QRegExp::escape(nickname) + "(\\W|$)", _nicksCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
            if (nickRegExp.indexIn(stripFormatCodes(msgContents)) >= 0) {
                return true;
            }
        }
    }

    return false;
}

void HighlightRuleManager::removeHighlightRule(const QString &highlightRule)
{
    removeAt(indexOf(highlightRule));
    SYNC(ARG(highlightRule))
}


void HighlightRuleManager::toggleHighlightRule(const QString &highlightRule)
{
    int idx = indexOf(highlightRule);
    if (idx == -1)
        return;
    _highlightRuleList[idx].isEnabled = !_highlightRuleList[idx].isEnabled;
    SYNC(ARG(highlightRule))
}

bool HighlightRuleManager::match(const Message &msg, const QString &currentNick, const QStringList &identityNicks)
{
    return match(msg.contents(), msg.sender(), msg.type(), msg.flags(), msg.bufferInfo().bufferName(), currentNick, identityNicks);
}

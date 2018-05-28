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

#include "ignorelistmanager.h"

#include <QtCore>
#include <QDebug>
#include <QStringList>

INIT_SYNCABLE_OBJECT(IgnoreListManager)
IgnoreListManager &IgnoreListManager::operator=(const IgnoreListManager &other)
{
    if (this == &other)
        return *this;

    SyncableObject::operator=(other);
    _ignoreList = other._ignoreList;
    return *this;
}


int IgnoreListManager::indexOf(const QString &ignore) const
{
    for (int i = 0; i < _ignoreList.count(); i++) {
        if (_ignoreList[i].ignoreRule == ignore)
            return i;
    }
    return -1;
}


QVariantMap IgnoreListManager::initIgnoreList() const
{
    QVariantMap ignoreListMap;
    QVariantList ignoreTypeList;
    QStringList ignoreRuleList;
    QStringList scopeRuleList;
    QVariantList isRegExList;
    QVariantList scopeList;
    QVariantList strictnessList;
    QVariantList isActiveList;

    for (int i = 0; i < _ignoreList.count(); i++) {
        ignoreTypeList << _ignoreList[i].type;
        ignoreRuleList << _ignoreList[i].ignoreRule;
        scopeRuleList << _ignoreList[i].scopeRule;
        isRegExList << _ignoreList[i].isRegEx;
        scopeList << _ignoreList[i].scope;
        strictnessList << _ignoreList[i].strictness;
        isActiveList << _ignoreList[i].isActive;
    }

    ignoreListMap["ignoreType"] = ignoreTypeList;
    ignoreListMap["ignoreRule"] = ignoreRuleList;
    ignoreListMap["scopeRule"] = scopeRuleList;
    ignoreListMap["isRegEx"] = isRegExList;
    ignoreListMap["scope"] = scopeList;
    ignoreListMap["strictness"] = strictnessList;
    ignoreListMap["isActive"] = isActiveList;
    return ignoreListMap;
}


void IgnoreListManager::initSetIgnoreList(const QVariantMap &ignoreList)
{
    QVariantList ignoreType = ignoreList["ignoreType"].toList();
    QStringList ignoreRule = ignoreList["ignoreRule"].toStringList();
    QStringList scopeRule = ignoreList["scopeRule"].toStringList();
    QVariantList isRegEx = ignoreList["isRegEx"].toList();
    QVariantList scope = ignoreList["scope"].toList();
    QVariantList strictness = ignoreList["strictness"].toList();
    QVariantList isActive = ignoreList["isActive"].toList();

    int count = ignoreRule.count();
    if (count != scopeRule.count() || count != isRegEx.count() ||
        count != scope.count() || count != strictness.count() || count != ignoreType.count() || count != isActive.count()) {
        qWarning() << "Corrupted IgnoreList settings! (Count mismatch)";
        return;
    }

    _ignoreList.clear();
    for (int i = 0; i < ignoreRule.count(); i++) {
        _ignoreList << IgnoreListItem(static_cast<IgnoreType>(ignoreType[i].toInt()), ignoreRule[i], isRegEx[i].toBool(),
            static_cast<StrictnessType>(strictness[i].toInt()), static_cast<ScopeType>(scope[i].toInt()),
            scopeRule[i], isActive[i].toBool());
    }
}


/* since overloaded methods aren't syncable (yet?) we can't use that anymore
void IgnoreListManager::addIgnoreListItem(const IgnoreListItem &item) {
  addIgnoreListItem(item.type, item.ignoreRule, item.isRegEx, item.strictness, item.scope, item.scopeRule, item.isActive);
}
*/
void IgnoreListManager::addIgnoreListItem(int type, const QString &ignoreRule, bool isRegEx, int strictness,
    int scope, const QString &scopeRule, bool isActive)
{
    if (contains(ignoreRule)) {
        return;
    }

    IgnoreListItem newItem = IgnoreListItem(static_cast<IgnoreType>(type), ignoreRule, isRegEx, static_cast<StrictnessType>(strictness),
        static_cast<ScopeType>(scope), scopeRule, isActive);
    _ignoreList << newItem;

    SYNC(ARG(type), ARG(ignoreRule), ARG(isRegEx), ARG(strictness), ARG(scope), ARG(scopeRule), ARG(isActive))
}


IgnoreListManager::StrictnessType IgnoreListManager::_match(const QString &msgContents, const QString &msgSender, Message::Type msgType, const QString &network, const QString &bufferName)
{
    // We method don't rely on a proper Message object to make this method more versatile.
    // This allows us to use it in the core with unprocessed Messages or in the Client
    // with properly preprocessed Messages.
    if (!(msgType & (Message::Plain | Message::Notice | Message::Action)))
        return UnmatchedStrictness;

    foreach(IgnoreListItem item, _ignoreList) {
        if (!item.isActive || item.type == CtcpIgnore)
            continue;
        if (item.scope == GlobalScope
            || (item.scope == NetworkScope && scopeMatch(item.scopeRule, network))
            || (item.scope == ChannelScope && scopeMatch(item.scopeRule, bufferName))) {
            QString str;
            if (item.type == MessageIgnore)
                str = msgContents;
            else
                str = msgSender;

//      qDebug() << "IgnoreListManager::match: ";
//      qDebug() << "string: " << str;
//      qDebug() << "pattern: " << ruleRx.pattern();
//      qDebug() << "scopeRule: " << item.scopeRule;
//      qDebug() << "now testing";
            if ((!item.isRegEx && item.regEx.exactMatch(str)) ||
                (item.isRegEx && item.regEx.indexIn(str) != -1)) {
//        qDebug() << "MATCHED!";
                return item.strictness;
            }
        }
    }
    return UnmatchedStrictness;
}


bool IgnoreListManager::scopeMatch(const QString &scopeRule, const QString &string) const
{
    // A match happens when the string does NOT match ANY inverted rules and matches AT LEAST one
    // normal rule, unless no normal rules exist (implicit wildcard match).  This gives inverted
    // rules higher priority regardless of ordering.
    //
    // TODO: After switching to Qt 5, use of this should be split into two parts, one part that
    // would generate compiled QRegularExpressions for match/inverted match, regenerating it on any
    // rule changes, and another part that would check each message against these compiled rules.

    // Keep track if any matches are found
    bool matches = false;
    // Keep track if normal rules and inverted rules are found, allowing for implicit wildcard
    bool normalRuleFound = false, invertedRuleFound = false;

    // Split each scope rule by separator, ignoring empty parts
    foreach(QString rule, scopeRule.split(";", QString::SkipEmptyParts)) {
        // Trim whitespace from the start/end of the rule
        rule = rule.trimmed();
        // Ignore empty rules
        if (rule.isEmpty())
            continue;

        // Check if this is an inverted rule (starts with '!')
        if (rule.startsWith("!")) {
            // Inverted rule found
            invertedRuleFound = true;

            // Take the reminder of the string
            QRegExp ruleRx(rule.mid(1), Qt::CaseInsensitive);
            ruleRx.setPatternSyntax(QRegExp::Wildcard);
            if (ruleRx.exactMatch(string)) {
                // Matches an inverted rule, full rule cannot match
                return false;
            }
        } else {
            // Normal rule found
            normalRuleFound = true;

            QRegExp ruleRx(rule, Qt::CaseInsensitive);
            ruleRx.setPatternSyntax(QRegExp::Wildcard);
            if (ruleRx.exactMatch(string)) {
                // Matches a normal rule, full rule might match
                matches = true;
                // Continue checking in case other inverted rules negate this
            }
        }
    }
    // No inverted rules matched, okay to match normally
    // Return true if...
    // ...we found a normal match
    // ...implicit wildcard: we had inverted rules (that didn't match) and no normal rules
    return matches || (invertedRuleFound && !normalRuleFound);
}


void IgnoreListManager::removeIgnoreListItem(const QString &ignoreRule)
{
    removeAt(indexOf(ignoreRule));
    SYNC(ARG(ignoreRule))
}


void IgnoreListManager::toggleIgnoreRule(const QString &ignoreRule)
{
    int idx = indexOf(ignoreRule);
    if (idx == -1)
        return;
    _ignoreList[idx].isActive = !_ignoreList[idx].isActive;
    SYNC(ARG(ignoreRule))
}


bool IgnoreListManager::ctcpMatch(const QString sender, const QString &network, const QString &type)
{
    foreach(IgnoreListItem item, _ignoreList) {
        if (!item.isActive)
            continue;
        if (item.scope == GlobalScope || (item.scope == NetworkScope && scopeMatch(item.scopeRule, network))) {
            QString sender_;
            QStringList types = item.ignoreRule.split(QRegExp("\\s+"), QString::SkipEmptyParts);

            sender_ = types.takeAt(0);

            QRegExp ruleRx = QRegExp(sender_);
            ruleRx.setCaseSensitivity(Qt::CaseInsensitive);
            if (!item.isRegEx)
                ruleRx.setPatternSyntax(QRegExp::Wildcard);
            if ((!item.isRegEx && ruleRx.exactMatch(sender)) ||
                (item.isRegEx && ruleRx.indexIn(sender) != -1)) {
                if (types.isEmpty() || types.contains(type, Qt::CaseInsensitive))
                    return true;
            }
        }
    }
    return false;
}

/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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
#include <QRegExp>

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
        qWarning() << "Corrupted IgnoreList settings! (Count missmatch)";
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

            QRegExp ruleRx = QRegExp(item.ignoreRule);
            ruleRx.setCaseSensitivity(Qt::CaseInsensitive);
            if (!item.isRegEx) {
                ruleRx.setPatternSyntax(QRegExp::Wildcard);
            }

//      qDebug() << "IgnoreListManager::match: ";
//      qDebug() << "string: " << str;
//      qDebug() << "pattern: " << ruleRx.pattern();
//      qDebug() << "scopeRule: " << item.scopeRule;
//      qDebug() << "now testing";
            if ((!item.isRegEx && ruleRx.exactMatch(str)) ||
                (item.isRegEx && ruleRx.indexIn(str) != -1)) {
//        qDebug() << "MATCHED!";
                return item.strictness;
            }
        }
    }
    return UnmatchedStrictness;
}


bool IgnoreListManager::scopeMatch(const QString &scopeRule, const QString &string) const
{
    foreach(QString rule, scopeRule.split(";")) {
        QRegExp ruleRx = QRegExp(rule.trimmed());
        ruleRx.setCaseSensitivity(Qt::CaseInsensitive);
        ruleRx.setPatternSyntax(QRegExp::Wildcard);
        if (ruleRx.exactMatch(string)) {
            return true;
        }
    }
    return false;
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

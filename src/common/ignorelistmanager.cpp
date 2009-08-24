/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ignorelistmanager.h"

#include <QDebug>
#include <QStringList>
#include <QRegExp>

#include "message.h"

INIT_SYNCABLE_OBJECT(IgnoreListManager)

IgnoreListManager &IgnoreListManager::operator=(const IgnoreListManager &other) {
  if(this == &other)
    return *this;

  SyncableObject::operator=(other);
  _ignoreList = other._ignoreList;
  return *this;
}

int IgnoreListManager::indexOf(const QString &ignore) const {
  for(int i = 0; i < _ignoreList.count(); i++) {
    if(_ignoreList[i].ignoreRule == ignore)
      return i;
  }
  return -1;
}

QVariantMap IgnoreListManager::initIgnoreList() const {
  QVariantMap ignoreListMap;
  QVariantList ignoreTypeList;
  QStringList ignoreRuleList;
  QStringList scopeRuleList;
  QVariantList isRegExList;
  QVariantList scopeList;
  QVariantList strictnessList;
  QVariantList isActiveList;

  for(int i = 0; i < _ignoreList.count(); i++) {
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

void IgnoreListManager::initSetIgnoreList(const QVariantMap &ignoreList) {
  QVariantList ignoreType = ignoreList["ignoreType"].toList();
  QStringList ignoreRule = ignoreList["ignoreRule"].toStringList();
  QStringList scopeRule = ignoreList["scopeRule"].toStringList();
  QVariantList isRegEx = ignoreList["isRegEx"].toList();
  QVariantList scope = ignoreList["scope"].toList();
  QVariantList strictness = ignoreList["strictness"].toList();
  QVariantList isActive = ignoreList["isActive"].toList();

  int count = ignoreRule.count();
  if(count != scopeRule.count() || count != isRegEx.count() ||
     count != scope.count() || count != strictness.count() || count != ignoreType.count() || count != isActive.count()) {
    qWarning() << "Corrupted IgnoreList settings! (Count missmatch)";
    return;
  }

  _ignoreList.clear();
  for(int i = 0; i < ignoreRule.count(); i++) {
    _ignoreList << IgnoreListItem(static_cast<IgnoreType>(ignoreType[i].toInt()), ignoreRule[i], isRegEx[i].toBool(),
                              static_cast<StrictnessType>(strictness[i].toInt()), static_cast<ScopeType>(scope[i].toInt()),
                              scopeRule[i], isActive[i].toBool());
  }
}

void IgnoreListManager::addIgnoreListItem(const IgnoreListItem &item) {
  addIgnoreListItem(item.type, item.ignoreRule, item.isRegEx, item.strictness, item.scope, item.scopeRule, item.isActive);
}

void IgnoreListManager::addIgnoreListItem(IgnoreType type, const QString &ignoreRule, bool isRegEx, StrictnessType strictness,
                                      ScopeType scope, const QString &scopeRule, bool isActive) {
  if(contains(ignoreRule)) {
    return;
  }

  _ignoreList << IgnoreListItem(type, ignoreRule, isRegEx, strictness, scope, scopeRule, isActive);

  emit ignoreAdded(type, ignoreRule, isRegEx, strictness, scope, scopeRule, isActive);
}

IgnoreListManager::StrictnessType IgnoreListManager::match(const Message &msg, const QString &network) {
  if(!(msg.type() & (Message::Plain | Message::Notice | Message::Action)))
    return UnmatchedStrictness;

  foreach(IgnoreListItem item, _ignoreList) {
    if(!item.isActive)
      continue;
    if(item.scope == GlobalScope || (item.scope == NetworkScope && scopeMatch(item.scopeRule, network)) ||
       (item.scope == ChannelScope && scopeMatch(item.scopeRule, msg.bufferInfo().bufferName()))) {

      QString str;
      if(item.type == MessageIgnore)
        str = msg.contents();
      else
        str = msg.sender();

      QRegExp ruleRx = QRegExp(item.ignoreRule);
      ruleRx.setCaseSensitivity(Qt::CaseInsensitive);
      if(!item.isRegEx) {
        ruleRx.setPatternSyntax(QRegExp::Wildcard);
      }

//      qDebug() << "IgnoreListManager::match: ";
//      qDebug() << "string: " << str;
//      qDebug() << "pattern: " << ruleRx.pattern();
//      qDebug() << "scopeRule: " << item.scopeRule;
//      qDebug() << "now testing";
      if((!item.isRegEx && ruleRx.exactMatch(str)) ||
          (item.isRegEx && ruleRx.indexIn(str) != -1)) {
//        qDebug() << "MATCHED!";
        return item.strictness;
      }
    }
  }
  return UnmatchedStrictness;
}

bool IgnoreListManager::scopeMatch(const QString &scopeRule, const QString &string) {
  foreach(QString rule, scopeRule.split(";")) {
    QRegExp ruleRx = QRegExp(rule.trimmed());
    ruleRx.setCaseSensitivity(Qt::CaseInsensitive);
    ruleRx.setPatternSyntax(QRegExp::Wildcard);
    if(ruleRx.exactMatch(string)) {
      return true;
    }
  }
  return false;
}

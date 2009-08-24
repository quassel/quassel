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

#ifndef IGNORELISTMANAGER_H
#define IGNORELISTMANAGER_H

#include <QString>

#include "syncableobject.h"

class Message;

class IgnoreListManager : public SyncableObject
{
  SYNCABLE_OBJECT
  Q_OBJECT
public:
  inline IgnoreListManager(QObject *parent = 0) : SyncableObject(parent) { setAllowClientUpdates(true); }
  IgnoreListManager &operator=(const IgnoreListManager &other);

  enum IgnoreType {
    SenderIgnore,
    MessageIgnore
  };

  enum StrictnessType {
    UnmatchedStrictness = 0,
    SoftStrictness = 1,
    HardStrictness = 2
  };

  enum ScopeType {
    GlobalScope,
    NetworkScope,
    ChannelScope,
  };

  struct IgnoreListItem {
    IgnoreType type;
    QString ignoreRule;
    bool isRegEx;
    StrictnessType strictness;
    ScopeType scope;
    QString scopeRule;
    bool isActive;
    IgnoreListItem() {}
    IgnoreListItem(IgnoreType type_, const QString &ignoreRule_, bool isRegEx_, StrictnessType strictness_,
               ScopeType scope_, const QString &scopeRule_, bool isActive_)
        : type(type_), ignoreRule(ignoreRule_), isRegEx(isRegEx_), strictness(strictness_), scope(scope_), scopeRule(scopeRule_), isActive(isActive_)  {}
    bool operator!=(const IgnoreListItem &other) {
      return (type != other.type ||
        ignoreRule != other.ignoreRule ||
        isRegEx != other.isRegEx ||
        strictness != other.strictness ||
        scope != other.scope ||
        scopeRule != other.scopeRule ||
        isActive != other.isActive);
    }
  };
  typedef QList<IgnoreListItem> IgnoreList;

  int indexOf(const QString &ignore) const;
  inline bool contains(const QString &ignore) const { return indexOf(ignore) != -1; }
  inline bool isEmpty() const { return _ignoreList.isEmpty(); }
  inline int count() const { return _ignoreList.count(); }
  inline void removeAt(int index) { _ignoreList.removeAt(index); }
  inline IgnoreListItem &operator[](int i) { return _ignoreList[i]; }
  inline const IgnoreListItem &operator[](int i) const { return _ignoreList.at(i); }
  inline const IgnoreList &ignoreList() const { return _ignoreList; }

  //! Check if a message matches the IgnoreRule
  /** This method checks if a message matches the users ignorelist.
    * \param msg The Message that should be checked
    * \param network The networkname the message belongs to
    * \return UnmatchedStrictness, HardStrictness or SoftStrictness representing the match type
    */
  StrictnessType match(const Message &msg, const QString &network = QString());

  virtual void addIgnoreListItem(IgnoreType type, const QString &ignoreRule, bool isRegEx, StrictnessType strictness,
                                 ScopeType scope, const QString &scopeRule, bool isActive);
  virtual void addIgnoreListItem(const IgnoreListItem &item);

public slots:
  virtual QVariantMap initIgnoreList() const;
  virtual void initSetIgnoreList(const QVariantMap &ignoreList);

protected:
  void setIgnoreList(const QList<IgnoreListItem> &ignoreList) { _ignoreList = ignoreList; }

signals:
  void ignoreAdded(IgnoreType type, const QString &ignoreRule, bool isRegex, StrictnessType strictness, ScopeType scope, const QVariant &scopeRule, bool isActive);

private:
  // scopeRule is a ; separated list, string is a network/channel-name
  bool scopeMatch(const QString &scopeRule, const QString &string);
  IgnoreList _ignoreList;
};

#endif // IGNORELISTMANAGER_H

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

#include "aliasmanager.h"

#include <QDebug>
#include <QStringList>

AliasManager &AliasManager::operator=(const AliasManager &other) {
  if(this == &other)
    return *this;
  
  SyncableObject::operator=(other);
  _aliases = other._aliases;
  return *this;
}

int AliasManager::indexOf(const QString &name) const {
  for(int i = 0; i < _aliases.count(); i++) {
    if(_aliases[i].name == name)
      return i;
  }
  return -1;
}

QVariantMap AliasManager::initAliases() const {
  QVariantMap aliases;
  QStringList names;
  QStringList expansions;

  for(int i = 0; i < _aliases.count(); i++) {
    names << _aliases[i].name;
    expansions << _aliases[i].expansion;
  }

  aliases["names"] = names;
  aliases["expansions"] = expansions;
  return aliases;
}

void AliasManager::initSetAliases(const QVariantMap &aliases) {
  QStringList names = aliases["names"].toStringList();
  QStringList expansions = aliases["expansions"].toStringList();

  if(names.count() != expansions.count()) {
    qWarning() << "AliasesManager::initSetAliases: received" << names.count() << "alias names but only" << expansions.count() << "expansions!";
    return;
  }

  _aliases.clear();
  for(int i = 0; i < names.count(); i++) {
    _aliases << Alias(names[i], expansions[i]);
  }
}


void AliasManager::addAlias(const QString &name, const QString &expansion) {
  if(contains(name)) {
    return;
  }

  _aliases << Alias(name, expansion);

  emit aliasAdded(name, expansion);
}

AliasManager::AliasList AliasManager::defaults() {
  AliasList aliases;
  aliases << Alias("j", "/join $0")
	  << Alias("ns", "/msg nickserv $0")
	  << Alias("nickserv", "/msg nickserv $0")
	  << Alias("cs", "/msg chanserv $0")
	  << Alias("chanserv",  "/msg chanserv $0")
	  << Alias("hs", "/msg hostserv $0")
	  << Alias("hostserv", "/msg hostserv $0")
	  << Alias("back", "/quote away");
  return aliases;
}

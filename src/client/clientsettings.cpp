/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#include "client.h"
#include "clientsettings.h"
#include "global.h"

#include <QStringList>

ClientSettings::ClientSettings(QString g) : Settings(g, Global::clientApplicationName) {


}

ClientSettings::~ClientSettings() {


}

/***********************************************************************************************/

CoreAccountSettings::CoreAccountSettings(const QString &subgroup) : ClientSettings("CoreAccounts") {
  _subgroup = subgroup;

}

QList<AccountId> CoreAccountSettings::knownAccounts() {
  QList<AccountId> ids;
  foreach(QString key, localChildGroups()) {
    ids << key.toInt();
  }
  return ids;
}

AccountId CoreAccountSettings::lastAccount() {
  return localValue("LastAccount", 0).toInt();
}

void CoreAccountSettings::setLastAccount(AccountId account) {
  setLocalValue("LastAccount", account.toInt());
}

AccountId CoreAccountSettings::autoConnectAccount() {
  return localValue("AutoConnectAccount", 0).toInt();
}

void CoreAccountSettings::setAutoConnectAccount(AccountId account) {
  setLocalValue("AutoConnectAccount", account.toInt());
}

void CoreAccountSettings::storeAccountData(AccountId id, const QVariantMap &data) {
  setLocalValue(QString("%1/Connection").arg(id.toInt()), data);
}

QVariantMap CoreAccountSettings::retrieveAccountData(AccountId id) {
  return localValue(QString("%1/Connection").arg(id.toInt()), QVariant()).toMap();
}

void CoreAccountSettings::setAccountValue(const QString &key, const QVariant &value) {
  if(!Client::currentCoreAccount().isValid()) return;
  setLocalValue(QString("%1/%2/%3").arg(Client::currentCoreAccount().toInt()).arg(_subgroup).arg(key), value);
}

QVariant CoreAccountSettings::accountValue(const QString &key, const QVariant &def) {
  if(!Client::currentCoreAccount().isValid()) return QVariant();
  return localValue(QString("%1/%2/%3").arg(Client::currentCoreAccount().toInt()).arg(_subgroup).arg(key), def);
}

void CoreAccountSettings::removeAccount(AccountId id) {
  removeLocalKey(QString("%1").arg(id.toInt()));
}



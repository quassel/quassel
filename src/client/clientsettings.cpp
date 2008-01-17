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

QStringList ClientSettings::sessionKeys() {
  return Client::sessionDataKeys();
}

void ClientSettings::setSessionValue(const QString &key, const QVariant &data) {
  Client::storeSessionData(key, data);
}

QVariant ClientSettings::sessionValue(const QString &key, const QVariant &def) {
  return Client::retrieveSessionData(key, def);
}

/***********************************************************************************************/

CoreAccountSettings::CoreAccountSettings() : ClientSettings("CoreAccounts") {


}

QStringList CoreAccountSettings::knownAccounts() {
  return localChildKeys("Accounts");
}

QString CoreAccountSettings::lastAccount() {
  return localValue("LastAccount", "").toString();
}

void CoreAccountSettings::setLastAccount(const QString &account) {
  setLocalValue("LastAccount", account);
}

QString CoreAccountSettings::autoConnectAccount() {
  return localValue("AutoConnectAccount", "").toString();
}

void CoreAccountSettings::setAutoConnectAccount(const QString &account) {
  setLocalValue("AutoConnectAccount", account);
}

void CoreAccountSettings::storeAccount(const QString name, const QVariantMap &data) {
  setLocalValue(QString("Accounts/%2").arg(name), data);
}

QVariantMap CoreAccountSettings::retrieveAccount(const QString &name) {
  return localValue(QString("Accounts/%2").arg(name), QVariant()).toMap();
}

void CoreAccountSettings::storeAllAccounts(const QHash<QString, QVariantMap> accounts) {
  removeLocalKey(QString("Accounts"));
  foreach(QString name, accounts.keys()) {
    storeAccount(name, accounts[name]);
  }
}

QHash<QString, QVariantMap> CoreAccountSettings::retrieveAllAccounts() {
  QHash<QString, QVariantMap> accounts;
  foreach(QString name, knownAccounts()) {
    accounts[name] = retrieveAccount(name);
  }
  return accounts;
}

void CoreAccountSettings::removeAccount(const QString &account) {
  removeLocalKey(QString("Accounts/%1").arg(account));
}



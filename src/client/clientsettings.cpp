/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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

AccountSettings::AccountSettings() : ClientSettings("Accounts") {


}

QStringList AccountSettings::knownAccounts() {
  return localChildGroups();
}

QString AccountSettings::lastAccount() {
  return localValue("LastAccount", "").toString();
}

void AccountSettings::setLastAccount(const QString &account) {
  setLocalValue("LastAccount", account);
}

QString AccountSettings::autoConnectAccount() {
  return localValue("AutoConnectAccount", "").toString();
}

void AccountSettings::setAutoConnectAccount(const QString &account) {
  setLocalValue("AutoConnectAccount", account);
}

void AccountSettings::setValue(const QString &account, const QString &key, const QVariant &data) {
  setLocalValue(QString("%1/%2").arg(account).arg(key), data);
}

QVariant AccountSettings::value(const QString &account, const QString &key, const QVariant &def) {
  return localValue(QString("%1/%2").arg(account).arg(key), def);
}

void AccountSettings::removeAccount(const QString &account) {
  removeLocalKey(account);
}



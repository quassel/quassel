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

#include <QStringList>


#include "clientsettings.h"

#include <QHostAddress>
#ifdef HAVE_SSL
#include <QSslSocket>
#endif


#include "client.h"
#include "quassel.h"

ClientSettings::ClientSettings(QString g) : Settings(g, Quassel::buildInfo().clientApplicationName) {
}

ClientSettings::~ClientSettings() {
}

/***********************************************************************************************/

CoreAccountSettings::CoreAccountSettings(const QString &subgroup)
  : ClientSettings("CoreAccounts"),
    _subgroup(subgroup)
{
}

void CoreAccountSettings::notify(const QString &key, QObject *receiver, const char *slot) {
  ClientSettings::notify(QString("%1/%2/%3").arg(Client::currentCoreAccount().toInt()).arg(_subgroup).arg(key), receiver, slot);
}

QList<AccountId> CoreAccountSettings::knownAccounts() {
  QList<AccountId> ids;
  foreach(QString key, localChildGroups()) {
    AccountId acc = key.toInt();
    if(acc.isValid()) ids << acc;
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

void CoreAccountSettings::setJumpKeyMap(const QHash<int, BufferId> &keyMap) {
  QVariantMap variants;
  QHash<int, BufferId>::const_iterator mapIter = keyMap.constBegin();
  while(mapIter != keyMap.constEnd()) {
    variants[QString::number(mapIter.key())] = qVariantFromValue(mapIter.value());
    mapIter++;
  }
  setLocalValue("JumpKeyMap", variants);
}

QHash<int, BufferId> CoreAccountSettings::jumpKeyMap() {
  QHash<int, BufferId> keyMap;
  QVariantMap variants = localValue("JumpKeyMap", QVariant()).toMap();
  QVariantMap::const_iterator mapIter = variants.constBegin();
  while(mapIter != variants.constEnd()) {
    keyMap[mapIter.key().toInt()] = mapIter.value().value<BufferId>();
    mapIter++;
  }
  return keyMap;
}

void CoreAccountSettings::removeAccount(AccountId id) {
  removeLocalKey(QString("%1").arg(id.toInt()));
}


/***********************************************************************************************/
// NotificationSettings:

NotificationSettings::NotificationSettings() : ClientSettings("Notification") {
}

void NotificationSettings::setHighlightList(const QVariantList &highlightList) {
  setLocalValue("Highlights/CustomList", highlightList);
}

QVariantList NotificationSettings::highlightList() {
  return localValue("Highlights/CustomList").toList();
}

void NotificationSettings::setHighlightNick(NotificationSettings::HighlightNickType highlightNickType) {
  setLocalValue("Highlights/HighlightNick", highlightNickType);
}

NotificationSettings::HighlightNickType NotificationSettings::highlightNick() {
  return (NotificationSettings::HighlightNickType) localValue("Highlights/HighlightNick", CurrentNick).toInt();
}

void NotificationSettings::setNicksCaseSensitive(bool cs) {
  setLocalValue("Highlights/NicksCaseSensitive", cs);
}

bool NotificationSettings::nicksCaseSensitive() {
  return localValue("Highlights/NicksCaseSensitive", false).toBool();
}


// ========================================
//  KnownHostsSettings
// ========================================
KnownHostsSettings::KnownHostsSettings()
  : ClientSettings("KnownHosts")
{
}

QByteArray KnownHostsSettings::knownDigest(const QHostAddress &address) {
  return localValue(address.toString(), QByteArray()).toByteArray();
}

void KnownHostsSettings::saveKnownHost(const QHostAddress &address, const QByteArray &certDigest) {
  setLocalValue(address.toString(), certDigest);
}

bool KnownHostsSettings::isKnownHost(const QHostAddress &address, const QByteArray &certDigest) {
  return certDigest == localValue(address.toString(), QByteArray()).toByteArray();
}

#ifdef HAVE_SSL
QByteArray KnownHostsSettings::knownDigest(const QSslSocket *socket) {
  return knownDigest(socket->peerAddress());
}

void KnownHostsSettings::saveKnownHost(const QSslSocket *socket) {
  Q_ASSERT(socket);
  saveKnownHost(socket->peerAddress(), socket->peerCertificate().digest());
}

bool KnownHostsSettings::isKnownHost(const QSslSocket *socket) {
  Q_ASSERT(socket);
  return isKnownHost(socket->peerAddress(), socket->peerCertificate().digest());
}
#endif


// ========================================
//  NickCompletionSettings
// ========================================

NickCompletionSettings::NickCompletionSettings() : ClientSettings("NickCompletion") {
}

void NickCompletionSettings::setCompletionSuffix(const QString &suffix) {
  setLocalValue("CompletionSuffix", suffix);
}

QString NickCompletionSettings::completionSuffix() {
  return localValue("CompletionSuffix", ": ").toString();
}

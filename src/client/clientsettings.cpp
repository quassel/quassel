/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "clientsettings.h"

#include <utility>

#include <QHostAddress>
#include <QSslSocket>
#include <QStringList>

#include "client.h"
#include "quassel.h"

ClientSettings::ClientSettings(QString g)
    : Settings(g, Quassel::buildInfo().clientApplicationName)
{}

/***********************************************************************************************/

CoreAccountSettings::CoreAccountSettings(QString subgroup)
    : ClientSettings("CoreAccounts")
    , _subgroup(std::move(subgroup))
{}

QString CoreAccountSettings::keyForNotify(const QString& key) const
{
    return QString{"%1/%2/%3"}.arg(Client::currentCoreAccount().accountId().toInt()).arg(_subgroup).arg(key);
}

QList<AccountId> CoreAccountSettings::knownAccounts() const
{
    QList<AccountId> ids;
    foreach (const QString& key, localChildGroups()) {
        AccountId acc = key.toInt();
        if (acc.isValid())
            ids << acc;
    }
    return ids;
}

AccountId CoreAccountSettings::lastAccount() const
{
    return localValue("LastAccount", 0).toInt();
}

void CoreAccountSettings::setLastAccount(AccountId account)
{
    setLocalValue("LastAccount", account.toInt());
}

AccountId CoreAccountSettings::autoConnectAccount() const
{
    return localValue("AutoConnectAccount", 0).toInt();
}

void CoreAccountSettings::setAutoConnectAccount(AccountId account)
{
    setLocalValue("AutoConnectAccount", account.toInt());
}

bool CoreAccountSettings::autoConnectOnStartup() const
{
    return localValue("AutoConnectOnStartup", false).toBool();
}

void CoreAccountSettings::setAutoConnectOnStartup(bool b)
{
    setLocalValue("AutoConnectOnStartup", b);
}

bool CoreAccountSettings::autoConnectToFixedAccount() const
{
    return localValue("AutoConnectToFixedAccount", false).toBool();
}

void CoreAccountSettings::setAutoConnectToFixedAccount(bool b)
{
    setLocalValue("AutoConnectToFixedAccount", b);
}

void CoreAccountSettings::storeAccountData(AccountId id, const QVariantMap& data)
{
    QString base = QString::number(id.toInt());
    foreach (const QString& key, data.keys()) {
        setLocalValue(base + "/" + key, data.value(key));
    }

    // FIXME Migration from 0.5 -> 0.6
    removeLocalKey(QString("%1/Connection").arg(base));
}

QVariantMap CoreAccountSettings::retrieveAccountData(AccountId id) const
{
    QVariantMap map;
    QString base = QString::number(id.toInt());
    foreach (const QString& key, localChildKeys(base)) {
        map[key] = localValue(base + "/" + key);
    }

    // FIXME Migration from 0.5 -> 0.6
    if (!map.contains("Uuid") && map.contains("Connection")) {
        QVariantMap oldmap = map.value("Connection").toMap();
        map["AccountName"] = oldmap.value("AccountName");
        map["HostName"] = oldmap.value("Host");
        map["Port"] = oldmap.value("Port");
        map["User"] = oldmap.value("User");
        map["Password"] = oldmap.value("Password");
        map["StorePassword"] = oldmap.value("RememberPasswd");
        map["UseSSL"] = oldmap.value("useSsl");
        map["UseProxy"] = oldmap.value("useProxy");
        map["ProxyHostName"] = oldmap.value("proxyHost");
        map["ProxyPort"] = oldmap.value("proxyPort");
        map["ProxyUser"] = oldmap.value("proxyUser");
        map["ProxyPassword"] = oldmap.value("proxyPassword");
        map["ProxyType"] = oldmap.value("proxyType");
        map["Internal"] = oldmap.value("InternalAccount");

        map["AccountId"] = id.toInt();
        map["Uuid"] = QUuid::createUuid().toString();
    }

    return map;
}

void CoreAccountSettings::setAccountValue(const QString& key, const QVariant& value)
{
    if (!Client::currentCoreAccount().isValid())
        return;
    setLocalValue(QString("%1/%2/%3").arg(Client::currentCoreAccount().accountId().toInt()).arg(_subgroup).arg(key), value);
}

QVariant CoreAccountSettings::accountValue(const QString& key, const QVariant& def) const
{
    if (!Client::currentCoreAccount().isValid())
        return QVariant();
    return localValue(QString("%1/%2/%3").arg(Client::currentCoreAccount().accountId().toInt()).arg(_subgroup).arg(key), def);
}

void CoreAccountSettings::setJumpKeyMap(const QHash<int, BufferId>& keyMap)
{
    QVariantMap variants;
    QHash<int, BufferId>::const_iterator mapIter = keyMap.constBegin();
    while (mapIter != keyMap.constEnd()) {
        variants[QString::number(mapIter.key())] = QVariant::fromValue(mapIter.value());
        ++mapIter;
    }
    setAccountValue("JumpKeyMap", variants);
}

QHash<int, BufferId> CoreAccountSettings::jumpKeyMap() const
{
    QHash<int, BufferId> keyMap;
    QVariantMap variants = accountValue("JumpKeyMap", QVariant()).toMap();
    QVariantMap::const_iterator mapIter = variants.constBegin();
    while (mapIter != variants.constEnd()) {
        keyMap[mapIter.key().toInt()] = mapIter.value().value<BufferId>();
        ++mapIter;
    }
    return keyMap;
}

void CoreAccountSettings::setBufferViewOverlay(const QSet<int>& viewIds)
{
    QVariantList variants;
    foreach (int viewId, viewIds) {
        variants << QVariant::fromValue(viewId);
    }
    setAccountValue("BufferViewOverlay", variants);
}

QSet<int> CoreAccountSettings::bufferViewOverlay() const
{
    QSet<int> viewIds;
    QVariantList variants = accountValue("BufferViewOverlay").toList();
    for (QVariantList::const_iterator iter = variants.constBegin(); iter != variants.constEnd(); ++iter) {
        viewIds << iter->toInt();
    }
    return viewIds;
}

void CoreAccountSettings::removeAccount(AccountId id)
{
    removeLocalKey(QString("%1").arg(id.toInt()));
}

void CoreAccountSettings::clearAccounts()
{
    foreach (const QString& key, localChildGroups())
        removeLocalKey(key);
}

/***********************************************************************************************/
// CoreConnectionSettings:

CoreConnectionSettings::CoreConnectionSettings()
    : ClientSettings("CoreConnection")
{}

void CoreConnectionSettings::setNetworkDetectionMode(NetworkDetectionMode mode)
{
    setLocalValue("NetworkDetectionMode", mode);
}

CoreConnectionSettings::NetworkDetectionMode CoreConnectionSettings::networkDetectionMode() const
{
    auto mode = localValue("NetworkDetectionMode", UseQNetworkConfigurationManager).toInt();
    if (mode == 0)
        mode = UseQNetworkConfigurationManager;  // UseSolid is gone, map that to the new default
    return static_cast<NetworkDetectionMode>(mode);
}

void CoreConnectionSettings::setAutoReconnect(bool autoReconnect)
{
    setLocalValue("AutoReconnect", autoReconnect);
}

bool CoreConnectionSettings::autoReconnect() const
{
    return localValue("AutoReconnect", true).toBool();
}

void CoreConnectionSettings::setPingTimeoutInterval(int interval)
{
    setLocalValue("PingTimeoutInterval", interval);
}

int CoreConnectionSettings::pingTimeoutInterval() const
{
    return localValue("PingTimeoutInterval", 60).toInt();
}

void CoreConnectionSettings::setReconnectInterval(int interval)
{
    setLocalValue("ReconnectInterval", interval);
}

int CoreConnectionSettings::reconnectInterval() const
{
    return localValue("ReconnectInterval", 60).toInt();
}

/***********************************************************************************************/
// NotificationSettings:

NotificationSettings::NotificationSettings()
    : ClientSettings("Notification")
{}

void NotificationSettings::setValue(const QString& key, const QVariant& data)
{
    setLocalValue(key, data);
}

QVariant NotificationSettings::value(const QString& key, const QVariant& def) const
{
    return localValue(key, def);
}

void NotificationSettings::remove(const QString& key)
{
    removeLocalKey(key);
}

void NotificationSettings::setHighlightList(const QVariantList& highlightList)
{
    setLocalValue("Highlights/CustomList", highlightList);
}

QVariantList NotificationSettings::highlightList() const
{
    return localValue("Highlights/CustomList").toList();
}

void NotificationSettings::setHighlightNick(NotificationSettings::HighlightNickType highlightNickType)
{
    setLocalValue("Highlights/HighlightNick", highlightNickType);
}

NotificationSettings::HighlightNickType NotificationSettings::highlightNick() const
{
    return (NotificationSettings::HighlightNickType)localValue("Highlights/HighlightNick", NoNick).toInt();
}

void NotificationSettings::setNicksCaseSensitive(bool cs)
{
    setLocalValue("Highlights/NicksCaseSensitive", cs);
}

bool NotificationSettings::nicksCaseSensitive() const
{
    return localValue("Highlights/NicksCaseSensitive", false).toBool();
}

// ========================================
//  TabCompletionSettings
// ========================================

TabCompletionSettings::TabCompletionSettings()
    : ClientSettings("TabCompletion")
{}

void TabCompletionSettings::setCompletionSuffix(const QString& suffix)
{
    setLocalValue("CompletionSuffix", suffix);
}

QString TabCompletionSettings::completionSuffix() const
{
    return localValue("CompletionSuffix", ": ").toString();
}

void TabCompletionSettings::setAddSpaceMidSentence(bool space)
{
    setLocalValue("AddSpaceMidSentence", space);
}

bool TabCompletionSettings::addSpaceMidSentence() const
{
    return localValue("AddSpaceMidSentence", false).toBool();
}

void TabCompletionSettings::setSortMode(SortMode mode)
{
    setLocalValue("SortMode", mode);
}

TabCompletionSettings::SortMode TabCompletionSettings::sortMode() const
{
    return static_cast<SortMode>(localValue("SortMode"), LastActivity);
}

void TabCompletionSettings::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    setLocalValue("CaseSensitivity", cs);
}

Qt::CaseSensitivity TabCompletionSettings::caseSensitivity() const
{
    return (Qt::CaseSensitivity)localValue("CaseSensitivity", Qt::CaseInsensitive).toInt();
}

void TabCompletionSettings::setUseLastSpokenTo(bool use)
{
    setLocalValue("UseLastSpokenTo", use);
}

bool TabCompletionSettings::useLastSpokenTo() const
{
    return localValue("UseLastSpokenTo", false).toBool();
}

// ========================================
//  ItemViewSettings
// ========================================

ItemViewSettings::ItemViewSettings(const QString& group)
    : ClientSettings(group)
{}

bool ItemViewSettings::displayTopicInTooltip() const
{
    return localValue("DisplayTopicInTooltip", false).toBool();
}

bool ItemViewSettings::mouseWheelChangesBuffer() const
{
    return localValue("MouseWheelChangesBuffer", false).toBool();
}

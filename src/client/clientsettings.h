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

#pragma once

#include "client-export.h"

#include "settings.h"
#include "types.h"

class QHostAddress;
class QSslSocket;

class CLIENT_EXPORT ClientSettings : public Settings
{
protected:
    ClientSettings(QString group = "General");
};

// ========================================
//  CoreAccountSettings
// ========================================

// Deriving from CoreAccountSettings:
// MySettings() : CoreAccountSettings("MyGroup") {};
// Then use accountValue() / setAccountValue() to retrieve/store data associated to the currently
// connected account. This is stored in CoreAccounts/$ACCID/MyGroup/$KEY) then.
//
// Note that you'll get invalid data (and setting is ignored) if you are not connected to a core!

class CLIENT_EXPORT CoreAccountSettings : public ClientSettings
{
public:
    // stores account-specific data in CoreAccounts/$ACCID/$SUBGROUP/$KEY)
    CoreAccountSettings(QString subgroup = "General");

    QList<AccountId> knownAccounts() const;
    AccountId lastAccount() const;
    void setLastAccount(AccountId);
    AccountId autoConnectAccount() const;
    void setAutoConnectAccount(AccountId);
    bool autoConnectOnStartup() const;
    void setAutoConnectOnStartup(bool);
    bool autoConnectToFixedAccount() const;
    void setAutoConnectToFixedAccount(bool);

    void clearAccounts();

    void storeAccountData(AccountId id, const QVariantMap& data);
    QVariantMap retrieveAccountData(AccountId) const;
    void removeAccount(AccountId);

    void setJumpKeyMap(const QHash<int, BufferId>& keyMap);
    QHash<int, BufferId> jumpKeyMap() const;

    void setBufferViewOverlay(const QSet<int>& viewIds);
    QSet<int> bufferViewOverlay() const;

    void setAccountValue(const QString& key, const QVariant& data);
    QVariant accountValue(const QString& key, const QVariant& def = QVariant()) const;

protected:
    QString keyForNotify(const QString& key) const override;

private:
    QString _subgroup;
};

// ========================================
//  NotificationSettings
// ========================================
class CLIENT_EXPORT NotificationSettings : public ClientSettings
{
public:
    enum HighlightNickType
    {
        NoNick = 0x00,
        CurrentNick = 0x01,
        AllNicks = 0x02
    };

    NotificationSettings();

    void setValue(const QString& key, const QVariant& data);
    QVariant value(const QString& key, const QVariant& def = {}) const;
    void remove(const QString& key);

    void setHighlightList(const QVariantList& highlightList);
    QVariantList highlightList() const;

    void setHighlightNick(HighlightNickType);
    HighlightNickType highlightNick() const;

    void setNicksCaseSensitive(bool);
    bool nicksCaseSensitive() const;
};

// ========================================
// CoreConnectionSettings
// ========================================

class CLIENT_EXPORT CoreConnectionSettings : public ClientSettings
{
public:
    enum NetworkDetectionMode
    {
        UseQNetworkConfigurationManager = 1,  // UseSolid is gone
        UsePingTimeout,
        NoActiveDetection
    };

    CoreConnectionSettings();

    void setNetworkDetectionMode(NetworkDetectionMode mode);
    NetworkDetectionMode networkDetectionMode() const;

    void setAutoReconnect(bool autoReconnect);
    bool autoReconnect() const;

    void setPingTimeoutInterval(int interval);
    int pingTimeoutInterval() const;

    void setReconnectInterval(int interval);
    int reconnectInterval() const;
};

// ========================================
// TabCompletionSettings
// ========================================

class CLIENT_EXPORT TabCompletionSettings : public ClientSettings
{
public:
    enum SortMode
    {
        Alphabetical,
        LastActivity
    };

    TabCompletionSettings();

    void setCompletionSuffix(const QString&);
    QString completionSuffix() const;

    void setAddSpaceMidSentence(bool);
    bool addSpaceMidSentence() const;

    void setSortMode(SortMode);
    SortMode sortMode() const;

    void setCaseSensitivity(Qt::CaseSensitivity);
    Qt::CaseSensitivity caseSensitivity() const;

    void setUseLastSpokenTo(bool);
    bool useLastSpokenTo() const;
};

// ========================================
// ItemViewSettings
// ========================================
class CLIENT_EXPORT ItemViewSettings : public ClientSettings
{
public:
    ItemViewSettings(const QString& group = "ItemViews");

    bool displayTopicInTooltip() const;
    bool mouseWheelChangesBuffer() const;
};

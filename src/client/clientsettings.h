/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#ifndef CLIENTSETTINGS_H
#define CLIENTSETTINGS_H

#include "settings.h"

#include "types.h"

class QHostAddress;
class QSslSocket;

class ClientSettings : public Settings
{
public:
    virtual ~ClientSettings();

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

class CoreAccountSettings : public ClientSettings
{
public:
    // stores account-specific data in CoreAccounts/$ACCID/$SUBGROUP/$KEY)
    CoreAccountSettings(const QString &subgroup = "General");

    virtual void notify(const QString &key, QObject *receiver, const char *slot);

    QList<AccountId> knownAccounts();
    AccountId lastAccount();
    void setLastAccount(AccountId);
    AccountId autoConnectAccount();
    void setAutoConnectAccount(AccountId);
    bool autoConnectOnStartup();
    void setAutoConnectOnStartup(bool);
    bool autoConnectToFixedAccount();
    void setAutoConnectToFixedAccount(bool);

    void clearAccounts();

    void storeAccountData(AccountId id, const QVariantMap &data);
    QVariantMap retrieveAccountData(AccountId);
    void removeAccount(AccountId);

    void setBufferViewOverlay(const QSet<int> &viewIds);
    QSet<int> bufferViewOverlay();

    void setAccountValue(const QString &key, const QVariant &data);
    QVariant accountValue(const QString &key, const QVariant &def = QVariant());

private:
    QString _subgroup;
};


// ========================================
//  NotificationSettings
// ========================================
class NotificationSettings : public ClientSettings
{
public:
    enum HighlightNickType {
        NoNick = 0x00,
        CurrentNick = 0x01,
        AllNicks = 0x02
    };

    NotificationSettings();

    inline void setValue(const QString &key, const QVariant &data) { setLocalValue(key, data); }
    inline QVariant value(const QString &key, const QVariant &def = QVariant()) { return localValue(key, def); }
    inline void remove(const QString &key) { removeLocalKey(key); }

    void setHighlightList(const QVariantList &highlightList);
    QVariantList highlightList();

    void setHighlightNick(HighlightNickType);
    HighlightNickType highlightNick();

    void setNicksCaseSensitive(bool);
    bool nicksCaseSensitive();
};


// ========================================
// CoreConnectionSettings
// ========================================

class CoreConnectionSettings : public ClientSettings
{
public:
    enum NetworkDetectionMode {
        UseQNetworkConfigurationManager = 1, // UseSolid is gone
        UsePingTimeout,
        NoActiveDetection
    };

    CoreConnectionSettings();

    void setNetworkDetectionMode(NetworkDetectionMode mode);
    NetworkDetectionMode networkDetectionMode();

    void setAutoReconnect(bool autoReconnect);
    bool autoReconnect();

    void setPingTimeoutInterval(int interval);
    int pingTimeoutInterval();

    void setReconnectInterval(int interval);
    int reconnectInterval();
};


// ========================================
// TabCompletionSettings
// ========================================

class TabCompletionSettings : public ClientSettings
{
public:
    enum SortMode {
        Alphabetical,
        LastActivity
    };

    TabCompletionSettings();

    void setCompletionSuffix(const QString &);
    QString completionSuffix();

    void setAddSpaceMidSentence(bool);
    bool addSpaceMidSentence();

    void setSortMode(SortMode);
    SortMode sortMode();

    void setCaseSensitivity(Qt::CaseSensitivity);
    Qt::CaseSensitivity caseSensitivity();

    void setUseLastSpokenTo(bool);
    bool useLastSpokenTo();
};


// ========================================
// ItemViewSettings
// ========================================
class ItemViewSettings : public ClientSettings
{
public:
    ItemViewSettings(const QString &group = "ItemViews");

    bool displayTopicInTooltip();
    bool mouseWheelChangesBuffer();
};


#endif

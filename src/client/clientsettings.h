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

#ifndef _CLIENTSETTINGS_H_
#define _CLIENTSETTINGS_H_

#include "settings.h"
#include "types.h"

class ClientSettings : public Settings {

  public:
    virtual ~ClientSettings();

  protected:
    ClientSettings(QString group = "General");

};

// Deriving from CoreAccountSettings:
// MySettings() : CoreAccountSettings("MyGroup") {};
// Then use accountValue() / setAccountValue() to retrieve/store data associated to the currently
// connected account. This is stored in CoreAccounts/$ACCID/MyGroup/$KEY) then.
//
// Note that you'll get invalid data (and setting is ignored) if you are not connected to a core!

class CoreAccountSettings : public ClientSettings {

  public:
    // stores account-specific data in CoreAccounts/$ACCID/$SUBGROUP/$KEY)
    CoreAccountSettings(const QString &subgroup = "General");

    QList<AccountId> knownAccounts();
    AccountId lastAccount();
    void setLastAccount(AccountId);
    AccountId autoConnectAccount();
    void setAutoConnectAccount(AccountId);

    void storeAccountData(AccountId id, const QVariantMap &data);
    QVariantMap retrieveAccountData(AccountId);
    void removeAccount(AccountId);

    void setJumpKeyMap(const QHash<int, BufferId> &keyMap);
    QHash<int, BufferId> jumpKeyMap();

  protected:
    void setAccountValue(const QString &key, const QVariant &data);
    QVariant accountValue(const QString &key, const QVariant &def = QVariant());

  private:
    QString _subgroup;
};

class NotificationSettings : public ClientSettings {

  public:
    enum HighlightNickType {
      NoNick = 0x00,
      CurrentNick= 0x01,
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

};
#endif

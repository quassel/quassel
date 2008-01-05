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

class ClientSettings : public Settings {

  public:
    virtual ~ClientSettings();

  protected:
    ClientSettings(QString group = "General");

    //virtual QStringList allSessionKeys() = 0;
    virtual QStringList sessionKeys();

    virtual void setSessionValue(const QString &key, const QVariant &data);
    virtual QVariant sessionValue(const QString &key, const QVariant &def = QVariant());

};

class AccountSettings : public ClientSettings {

  public:
    AccountSettings();

    QStringList knownAccounts();
    QString lastAccount();
    void setLastAccount(const QString &account);
    QString autoConnectAccount();
    void setAutoConnectAccount(const QString &account);

    void setValue(const QString &account, const QString &key, const QVariant &data);
    QVariant value(const QString &account, const QString &key, const QVariant &def = QVariant());
    void removeAccount(const QString &account);

};

#endif

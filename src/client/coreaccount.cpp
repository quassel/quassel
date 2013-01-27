/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include "coreaccount.h"

CoreAccount::CoreAccount(AccountId accountId)
{
    _accountId = accountId;
    _internal = false;
    _port = 4242;
    _storePassword = false;
    _useSsl = true;
    _useProxy = false;
    _proxyType = QNetworkProxy::Socks5Proxy;
    _proxyPort = 8080;
}


void CoreAccount::setAccountId(AccountId id)
{
    _accountId = id;
}


void CoreAccount::setAccountName(const QString &name)
{
    _accountName = name;
}


void CoreAccount::setUuid(const QUuid &uuid)
{
    _uuid = uuid;
}


void CoreAccount::setInternal(bool internal)
{
    _internal = internal;
}


void CoreAccount::setUser(const QString &user)
{
    _user = user;
}


void CoreAccount::setPassword(const QString &password)
{
    _password = password;
}


void CoreAccount::setStorePassword(bool store)
{
    _storePassword = store;
}


void CoreAccount::setHostName(const QString &hostname)
{
    _hostName = hostname;
}


void CoreAccount::setPort(uint port)
{
    _port = port;
}


void CoreAccount::setUseSsl(bool useSsl)
{
    _useSsl = useSsl;
}


void CoreAccount::setUseProxy(bool useProxy)
{
    _useProxy = useProxy;
}


void CoreAccount::setProxyType(QNetworkProxy::ProxyType type)
{
    _proxyType = type;
}


void CoreAccount::setProxyUser(const QString &proxyUser)
{
    _proxyUser = proxyUser;
}


void CoreAccount::setProxyPassword(const QString &proxyPassword)
{
    _proxyPassword = proxyPassword;
}


void CoreAccount::setProxyHostName(const QString &proxyHostName)
{
    _proxyHostName = proxyHostName;
}


void CoreAccount::setProxyPort(uint proxyPort)
{
    _proxyPort = proxyPort;
}


QVariantMap CoreAccount::toVariantMap(bool forcePassword) const
{
    QVariantMap v;
    v["AccountId"] = accountId().toInt(); // can't use AccountId because then comparison fails
    v["AccountName"] = accountName();
    v["Uuid"] = uuid().toString();
    v["Internal"] = isInternal();
    v["User"] = user();
    if (_storePassword || forcePassword)
        v["Password"] = password();
    else
        v["Password"] = QString();
    v["StorePassword"] = storePassword();
    v["HostName"] = hostName();
    v["Port"] = port();
    v["UseSSL"] = useSsl();
    v["UseProxy"] = useProxy();
    v["ProxyType"] = proxyType();
    v["ProxyUser"] = proxyUser();
    v["ProxyPassword"] = proxyPassword();
    v["ProxyHostName"] = proxyHostName();
    v["ProxyPort"] = proxyPort();
    return v;
}


void CoreAccount::fromVariantMap(const QVariantMap &v)
{
    setAccountId((AccountId)v.value("AccountId").toInt());
    setAccountName(v.value("AccountName").toString());
    setUuid(QUuid(v.value("Uuid").toString()));
    setInternal(v.value("Internal").toBool());
    setUser(v.value("User").toString());
    setPassword(v.value("Password").toString());
    setStorePassword(v.value("StorePassword").toBool());
    setHostName(v.value("HostName").toString());
    setPort(v.value("Port").toUInt());
    setUseSsl(v.value("UseSSL").toBool());
    setUseProxy(v.value("UseProxy").toBool());
    setProxyType((QNetworkProxy::ProxyType)v.value("ProxyType").toInt());
    setProxyUser(v.value("ProxyUser").toString());
    setProxyPassword(v.value("ProxyPassword").toString());
    setProxyHostName(v.value("ProxyHostName").toString());
    setProxyPort(v.value("ProxyPort").toUInt());

    _storePassword = !password().isEmpty();
}


bool CoreAccount::operator==(const CoreAccount &o) const
{
    return toVariantMap(true) == o.toVariantMap(true);
}

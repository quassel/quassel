/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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

/* This file contains an implementation of an LDAP Authenticator, as an example
 * of what a custom external auth provider could do.
 *
 * It's based off of this pull request for quassel by abustany:
 * https://github.com/quassel/quassel/pull/4/
 *
 */

#include "ldapauthenticator.h"

#include "logger.h"
#include "network.h"
#include "quassel.h"

/* We should use openldap on windows if at all possible, rather than trying to
 * write some kind of compatiblity routine.
#ifdef Q_CC_MSVC
#include <windows.h>
#include <winldap.h>
#else*/
#include <ldap.h>
//#endif

LdapAuthenticator::LdapAuthenticator(QObject *parent)
    : Authenticator(parent),
    _connection(0)
{
}


LdapAuthenticator::~LdapAuthenticator()
{
    if (_connection != 0) {
        ldap_unbind_ext(_connection, 0, 0);
    }
}


bool LdapAuthenticator::isAvailable() const
{
    // FIXME: probably this should test if we can speak to the LDAP server.
    return true;
}


QString LdapAuthenticator::backendId() const
{
    // We identify the backend to use for the monolithic core by this identifier.
    // so only change this string if you _really_ have to and make sure the core
    // setup for the mono client still works ;)
    return QString("LDAP");
}


QString LdapAuthenticator::displayName() const
{
    return tr("LDAP");
}


QString LdapAuthenticator::description() const
{
    return tr("Authenticate users using an LDAP server.");
}


QVariantList LdapAuthenticator::setupData() const
{
    // The parameters needed for LDAP.
    QVariantList data;
    data << "Hostname"     << tr("Hostname")      << QString{"ldap://localhost"}
         << "Port"         << tr("Port")          << DEFAULT_LDAP_PORT
         << "BindDN"       << tr("Bind DN")       << QString{}
         << "BindPassword" << tr("Bind Password") << QString{}
         << "BaseDN"       << tr("Base DN")       << QString{}
         << "Filter"       << tr("Filter")        << QString{}
         << "UidAttribute" << tr("UID Attribute") << QString{"uid"}
         ;
    return data;
}


void LdapAuthenticator::setAuthProperties(const QVariantMap &properties,
                                          const QProcessEnvironment &environment,
                                          bool loadFromEnvironment)
{
    if (loadFromEnvironment) {
        _hostName = environment.value("AUTH_LDAP_HOSTNAME");
        _port = environment.value("AUTH_LDAP_PORT").toInt();
        _bindDN = environment.value("AUTH_LDAP_BIND_DN");
        _bindPassword = environment.value("AUTH_LDAP_BIND_PASSWORD");
        _baseDN = environment.value("AUTH_LDAP_BASE_DN");
        _filter = environment.value("AUTH_LDAP_FILTER");
        _uidAttribute = environment.value("AUTH_LDAP_UID_ATTRIBUTE");
    } else {
        _hostName = properties["Hostname"].toString();
        _port = properties["Port"].toInt();
        _bindDN = properties["BindDN"].toString();
        _bindPassword = properties["BindPassword"].toString();
        _baseDN = properties["BaseDN"].toString();
        _filter = properties["Filter"].toString();
        _uidAttribute = properties["UidAttribute"].toString();
    }
}

// TODO: this code is sufficiently general that in the future, perhaps an abstract
// class should be created implementing it.
// i.e. a provider that does its own thing and then pokes at the current storage
// through the default core method.
UserId LdapAuthenticator::validateUser(const QString &username, const QString &password)
{
    bool result = ldapAuth(username, password);
    if (!result) {
        return UserId();
    }

    // LDAP is case-insensitive, thus we will lowercase the username, in spite of
    // a better solution :(
    const QString lUsername = username.toLower();

    // If auth succeeds, but the user has not logged into quassel previously, make
    // a new user for them and return that ID.
    // Users created via LDAP have empty passwords, but authenticator column = LDAP.
    // On the other hand, if auth succeeds and the user already exists, do a final
    // cross-check to confirm we're using the right auth provider.
    UserId quasselId = Core::validateUser(lUsername, QString());
    if (!quasselId.isValid()) {
        return Core::addUser(lUsername, QString(), backendId());
    }
    else if (!(Core::checkAuthProvider(quasselId, backendId()))) {
        return 0;
    }
    return quasselId;
}


bool LdapAuthenticator::setup(const QVariantMap &settings,
                              const QProcessEnvironment &environment,
                              bool loadFromEnvironment)
{
    setAuthProperties(settings, environment, loadFromEnvironment);
    bool status = ldapConnect();
    return status;
}


Authenticator::State LdapAuthenticator::init(const QVariantMap &settings,
                                             const QProcessEnvironment &environment,
                                             bool loadFromEnvironment)
{
    setAuthProperties(settings, environment, loadFromEnvironment);

    bool status = ldapConnect();
    if (!status) {
        quInfo() << qPrintable(backendId()) << "authenticator cannot connect.";
        return NotAvailable;
    }

    quInfo() << qPrintable(backendId()) << "authenticator is ready.";
    return IsReady;
}

// Method based on abustany LDAP quassel patch.
bool LdapAuthenticator::ldapConnect()
{
    if (_connection != 0) {
        ldapDisconnect();
    }

    int res, v = LDAP_VERSION3;

    QString serverURI;
    QByteArray serverURIArray;

    // Convert info to hostname:port.
    serverURI = _hostName + ":" + QString::number(_port);
    serverURIArray = serverURI.toLocal8Bit();
    res = ldap_initialize(&_connection, serverURIArray);

    quInfo() << "LDAP: Connecting to" << serverURI;

    if (res != LDAP_SUCCESS) {
        qWarning() << "Could not connect to LDAP server:" << ldap_err2string(res);
        return false;
    }

    res = ldap_set_option(_connection, LDAP_OPT_PROTOCOL_VERSION, (void*)&v);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Could not set LDAP protocol version to v3:" << ldap_err2string(res);
        ldap_unbind_ext(_connection, 0, 0);
        _connection = 0;
        return false;
    }

    return true;
}


void LdapAuthenticator::ldapDisconnect()
{
    if (_connection == 0) {
        return;
    }

    ldap_unbind_ext(_connection, 0, 0);
    _connection = 0;
}


bool LdapAuthenticator::ldapAuth(const QString &username, const QString &password)
{
    if (password.isEmpty()) {
        return false;
    }

    int res;

    // Attempt to establish a connection.
    if (_connection == 0) {
        if (!ldapConnect()) {
            return false;
        }
    }

    struct berval cred;

    // Convert some things to byte arrays as needed.
    QByteArray bindPassword = _bindPassword.toLocal8Bit();
    QByteArray bindDN = _bindDN.toLocal8Bit();
    QByteArray baseDN = _baseDN.toLocal8Bit();
    QByteArray uidAttribute = _uidAttribute.toLocal8Bit();

    cred.bv_val = (bindPassword.size() > 0 ? bindPassword.data() : NULL);
    cred.bv_len = bindPassword.size();

    res = ldap_sasl_bind_s(_connection, bindDN.size() > 0 ? bindDN.constData() : 0, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP bind failed:" << ldap_err2string(res) << ")";
        ldapDisconnect();
        return false;
    }

    LDAPMessage *msg = NULL, *entry = NULL;

    const QByteArray ldapQuery = "(&(" + uidAttribute + '=' + username.toLocal8Bit() + ")" + _filter.toLocal8Bit() + ")";

    res = ldap_search_ext_s(_connection, baseDN.constData(), LDAP_SCOPE_SUBTREE, ldapQuery.constData(), 0, 0, 0, 0, 0, 0, &msg);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP search failed:" << ldap_err2string(res) << ")";
        return false;
    }

    if (ldap_count_entries(_connection, msg) > 1) {
        qWarning() << "Refusing connection from" << username << "(LDAP search returned more than one result)";
        ldap_msgfree(msg);
        return false;
    }

    entry = ldap_first_entry(_connection, msg);

    if (entry == 0) {
        qWarning() << "Refusing connection from" << username << "(LDAP search returned no results)";
        ldap_msgfree(msg);
        return false;
    }

    QByteArray passwordArray = password.toLocal8Bit();
    cred.bv_val = passwordArray.data();
    cred.bv_len = password.size();

    char *userDN = ldap_get_dn(_connection, entry);

    res = ldap_sasl_bind_s(_connection, userDN, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP authentication failed)";
        ldap_memfree(userDN);
        ldap_msgfree(msg);
        return false;
    }

    // The original implementation had requiredAttributes. I have not included this code
    // but it would be easy to re-add if someone wants this feature.
    // Ben Rosser <bjr@acm.jhu.edu> (12/23/15).

    ldap_memfree(userDN);
    ldap_msgfree(msg);
    return true;
}

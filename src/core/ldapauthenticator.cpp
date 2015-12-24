/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include "network.h"
#include "quassel.h"

// Link against LDAP.
#include <ldap.h>

LdapAuthenticator::LdapAuthenticator(QObject *parent)
    : LdapAuthenticator(parent),
	: _connection(0)
{
}


LdapAuthenticator::~LdapAuthenticator()
{
	if (_connection != 0)
	{
		ldap_unbind_ext(_connection, 0, 0);
    }
}


bool LdapAuthenticator::isAvailable() const
{
    // XXX: probably this should test if we can speak to the LDAP server.
    return true;
}

QString LdapAuthenticator::displayName() const
{
    // We identify the backend to use for the monolithic core by its displayname.
    // so only change this string if you _really_ have to and make sure the core
    // setup for the mono client still works ;)
    return QString("LDAP");
}

QString LdapAuthenticator::description() const
{
    return tr("Authenticate users using an LDAP server.");
}

QStringList LdapAuthenticator::setupKeys() const
{
    // The parameters needed for LDAP.
    QStringList keys;
    keys << "Hostname"
         << "Port"
         << "Base DN"
         << "Bind DN"
         << "Bind Password"
         << "UID Attribute";
    return keys;
}

QVariantMap LdapAuthenticator::setupDefaults() const
{
    QVariantMap map;
    map["Hostname"] = QVariant(QString("ldap://localhost"));
    map["Port"] = QVariant(DEFAULT_LDAP_PORT);
    map["UID Attribute"] = QVariant(QString("uid"));
    return map;
}

void LdapAuthenticator::setConnectionProperties(const QVariantMap &properties)
{
    _hostName = properties["Hostname"].toString();
    _port = properties["Port"].toInt();
    _baseDN = properties["Base DN"].toString();
    _bindDN = properties["Bind DN"].toString();
    _bindPassword = properties["Bind Password"].toString();
    _uidAttribute = properties["UID Attribute"].toString();
}

UserId LdapAuthenticator::validateUser(const QString &user, const QString &password)
{
    if (password.isEmpty()) {
        return false;
    }

    int res;

	// Attempt to establish a connection.
    if (_connection == 0) {
        if (not ldapConnect()) {
            return false;
        }
    }

    struct berval cred;
	
	// Convert some things to byte arrays as needed.
	QByteArray bindPassword = _bindPassword.toByteArray();
	QByteArray bindDN = _bindDN.toByteArray();
	QByteArray baseDN = _baseDN.toByteArray();
	QByteArray uidAttribute = _uidAttribute.toByteArray();

    cred.bv_val = const_cast<char*>(bindPassword.size() > 0 ? bindPasword.constData() : NULL);
    cred.bv_len = bindPasword.size();

    res = ldap_sasl_bind_s(_connection, bindDN.size() > 0 ? bindDN.constData() : 0, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP bind failed:" << ldap_err2string(res) << ")";
        ldapDisconnect();
        return false;
    }

    LDAPMessage *msg = NULL, *entry = NULL;

    const QByteArray ldapQuery = uidAttribute + '=' + username.toLocal8Bit();

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

    const QByteArray passwordArray = password.toLocal8Bit();

    cred.bv_val = const_cast<char*>(passwordArray.constData());
    cred.bv_len = password.size();

    char *userDN = ldap_get_dn(_connection, entry);

    res = ldap_sasl_bind_s(_connection, userDN, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Refusing connection from" << username << "(LDAP authentication failed)";
        ldap_memfree(userDN);
        ldap_msgfree(msg);
        return false;
    }

    if (m_requiredAttributes.isEmpty()) {
        ldap_memfree(userDN);
        ldap_msgfree(msg);
        return true;
    }

    bool authenticated = false;

    // Needed to use foreach
    typedef QPair<QByteArray, QByteArray> QByteArrayPair;

    foreach (const QByteArrayPair &pair, m_requiredAttributes) {
        const QByteArray &attribute = pair.first;
        const QByteArray &value = pair.second;

        struct berval attr_value;
        attr_value.bv_val = const_cast<char*>(value.constData());
        attr_value.bv_len = value.size();

        authenticated = (ldap_compare_ext_s(m_ldapConnection,
                                            userDN,
                                            attribute.constData(),
                                            &attr_value,
                                            0,
                                            0) == LDAP_COMPARE_TRUE);

        if (authenticated) {
            break;
        }
    }

    ldap_memfree(userDN);
    ldap_msgfree(msg);

    return authenticated;
}

bool LdapAuthenticator::setup(const QVariantMap &settings)
{
	setConnectionProperties(settings);
	bool status = ldapConnect();
	return status;
}

Authenticator::State LdapAuthenticator::init(const QVariantMap &settings)
{
	setConnectionProperties(settings);
	
	bool status = ldapConnect();
	if (!status)
	{
		quInfo() << qPrintable(displayName()) << "Authenticator cannot connect.";
		return NotAvailable;
	}
	
	quInfo() << qPrintable(displayName()) << "Authenticator is ready.";
	return IsReady;
}

// Method based on abustany LDAP quassel patch.
bool LdapStorage::ldapConnect()
{
    if (_connection != 0) {
        ldapDisconnect();
    }

    int res, v = LDAP_VERSION3;

	QString serverURI;
	QByteArray serverURIArray;
	
	// Convert info to hostname:port.
	serverURI = _server + ":" + QString::number(port);
	serverURIArray = serverURI.toByteArray();
    res = ldap_initialize(&_connection, serverURIArray);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Could not connect to LDAP server:" << ldap_err2string(res);
        return false;
    }

    res = ldap_set_option(_connection, LDAP_OPT_PROTOCOL_VERSION, (void*)&v);

    if (res != LDAP_SUCCESS) {
        qWarning() << "Could not set LDAP protocol version to v3:" << ldap_err2string(res);
        ldap_unbind_ext(m_ldapConnection, 0, 0);
        m_ldapConnection = 0;
        return false;
    }

    return true;
}


void LdapStorage::ldapDisconnect()
{
    if (_connection == 0) {
        return;
    }

    ldap_unbind_ext(_connection, 0, 0);
    _connection = 0;
}
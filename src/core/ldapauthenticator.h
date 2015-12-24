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

#ifndef LDAPAUTHENTICATOR_H
#define LDAPAUTHENTICATOR_H

#include "authenticator.h"

#include "core.h"

// Link against LDAP.
#include <ldap.h>

// Default LDAP server port.
#define DEFAULT_LDAP_PORT 389

class LdapAuthenticator : public Authenticator
{
    Q_OBJECT

public:
    LdapAuthenticator(QObject *parent = 0);
    virtual ~LdapAuthenticator();

public slots:
    /* General */
    bool isAvailable() const;
    QString displayName() const;
    QString description() const;
    virtual QStringList setupKeys() const;
    virtual QVariantMap setupDefaults() const;
 
    bool setup(const QVariantMap &settings = QVariantMap());
    State init(const QVariantMap &settings = QVariantMap());
    UserId validateUser(const QString &user, const QString &password);
	
protected:
    virtual void setConnectionProperties(const QVariantMap &properties);
    bool ldapConnect();
    void ldapDisconnect();
    bool ldapAuth(const QString &username, const QString &password);

    // Protected methods for retrieving info about the LDAP connection.
    inline virtual QString hostName() { return _hostName; }
    inline virtual int port() { return _port; }
    inline virtual QString bindDN() { return _bindDN; }
    inline virtual QString baseDN() { return _baseDN; }

private:
    QString _hostName;
    int _port;
    QString _bindDN;
    QString _baseDN;
	QString _filter;
    QString _bindPassword;
    QString _uidAttribute;

	// The actual connection object.
	LDAP *_connection;
	
};


#endif

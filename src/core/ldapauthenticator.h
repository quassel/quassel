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

#ifndef LDAPAUTHENTICATOR_H
#define LDAPAUTHENTICATOR_H

#include "authenticator.h"

class LdapAuthenticator : public Authenticator
{
    Q_OBJECT

public:
    LdapAuthenticator(QObject *parent = 0);
    virtual ~LdapAuthenticator();

public slots:
    /* General */
    virtual bool isAvailable() const;
    virtual QString displayName() const;
    virtual QString description() const;
    virtual QStringList setupKeys() const;
    virtual QVariantMap setupDefaults() const;

    /* User handling */
    virtual UserId getUserId(const QString &username);
 
protected:
	// Protecte methods for retrieving info about the LDAP connection.
	inline virtual QString hostName() { return _hostName; }
	inline virtual int port() { return _port; }
	inline virtual QString bindDN() { return _bindDN; }
	inline virtual QString baseDN() { return _baseDN; }
	
private:
    QString _hostName;
    int _port;
	QString _bindDN;
	QString _baseDN;
};


#endif

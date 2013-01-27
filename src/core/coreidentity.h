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

#ifndef COREIDENTITY_H
#define COREIDENTITY_H

#include "identity.h"

#ifdef HAVE_SSL
#include <QSslKey>
#include <QSslCertificate>
#endif //HAVE_SSL

class SignalProxy;

// ========================================
//  CoreCertManager
// ========================================
#ifdef HAVE_SSL
class CoreIdentity;
class CoreCertManager : public CertManager
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreCertManager(CoreIdentity &identity);

#ifdef HAVE_SSL
    virtual const QSslKey &sslKey() const;
    virtual const QSslCertificate &sslCert() const;

public slots:
    virtual void setSslKey(const QByteArray &encoded);
    virtual void setSslCert(const QByteArray &encoded);
#endif

    void setId(IdentityId id);

private:
    CoreIdentity &identity;
};


#endif //HAVE_SSL

// =========================================
//  CoreIdentity
// =========================================
class CoreIdentity : public Identity
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CoreIdentity(IdentityId id, QObject *parent = 0);
    CoreIdentity(const Identity &other, QObject *parent = 0);
    CoreIdentity(const CoreIdentity &other, QObject *parent = 0);

    void synchronize(SignalProxy *proxy);

#ifdef HAVE_SSL
    inline const QSslKey &sslKey() const { return _sslKey; }
    inline void setSslKey(const QSslKey &key) { _sslKey = key; }
    void setSslKey(const QByteArray &encoded);
    inline const QSslCertificate &sslCert() const { return _sslCert; }
    inline void setSslCert(const QSslCertificate &cert) { _sslCert = cert; }
    void setSslCert(const QByteArray &encoded);
#endif /* HAVE_SSL */

    CoreIdentity &operator=(const CoreIdentity &identity);

private:
#ifdef HAVE_SSL
    QSslKey _sslKey;
    QSslCertificate _sslCert;

    CoreCertManager _certManager;
#endif
};


#ifdef HAVE_SSL
inline const QSslKey &CoreCertManager::sslKey() const
{
    return identity.sslKey();
}


inline const QSslCertificate &CoreCertManager::sslCert() const
{
    return identity.sslCert();
}


#endif

#endif //COREIDENTITY_H

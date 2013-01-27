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

#include "coreidentity.h"

#include "signalproxy.h"

INIT_SYNCABLE_OBJECT(CoreIdentity)
CoreIdentity::CoreIdentity(IdentityId id, QObject *parent)
    : Identity(id, parent)
#ifdef HAVE_SSL
    , _certManager(*this)
#endif
{
#ifdef HAVE_SSL
    connect(this, SIGNAL(idSet(IdentityId)), &_certManager, SLOT(setId(IdentityId)));
    connect(&_certManager, SIGNAL(updated()), this, SIGNAL(updated()));
#endif
}


CoreIdentity::CoreIdentity(const Identity &other, QObject *parent)
    : Identity(other, parent)
#ifdef HAVE_SSL
    , _certManager(*this)
#endif
{
#ifdef HAVE_SSL
    connect(this, SIGNAL(idSet(IdentityId)), &_certManager, SLOT(setId(IdentityId)));
    connect(&_certManager, SIGNAL(updated()), this, SIGNAL(updated()));
#endif
}


CoreIdentity::CoreIdentity(const CoreIdentity &other, QObject *parent)
    : Identity(other, parent)
#ifdef HAVE_SSL
    , _sslKey(other._sslKey),
    _sslCert(other._sslCert),
    _certManager(*this)
#endif
{
#ifdef HAVE_SSL
    connect(this, SIGNAL(idSet(IdentityId)), &_certManager, SLOT(setId(IdentityId)));
    connect(&_certManager, SIGNAL(updated()), this, SIGNAL(updated()));
#endif
}


void CoreIdentity::synchronize(SignalProxy *proxy)
{
    proxy->synchronize(this);
#ifdef HAVE_SSL
    proxy->synchronize(&_certManager);
#endif
}


#ifdef HAVE_SSL
void CoreIdentity::setSslKey(const QByteArray &encoded)
{
    QSslKey key(encoded, QSsl::Rsa);
    if (key.isNull())
        key = QSslKey(encoded, QSsl::Dsa);
    setSslKey(key);
}


void CoreIdentity::setSslCert(const QByteArray &encoded)
{
    setSslCert(QSslCertificate(encoded));
}


#endif

CoreIdentity &CoreIdentity::operator=(const CoreIdentity &identity)
{
    Identity::operator=(identity);
#ifdef HAVE_SSL
    _sslKey = identity._sslKey;
    _sslCert = identity._sslCert;
#endif
    return *this;
}


#ifdef HAVE_SSL
// ========================================
//  CoreCertManager
// ========================================
INIT_SYNCABLE_OBJECT(CoreCertManager)
CoreCertManager::CoreCertManager(CoreIdentity &identity)
    : CertManager(identity.id()),
    identity(identity)
{
    setAllowClientUpdates(true);
}


void CoreCertManager::setId(IdentityId id)
{
    renameObject(QString::number(id.toInt()));
}


void CoreCertManager::setSslKey(const QByteArray &encoded)
{
    identity.setSslKey(encoded);
    CertManager::setSslKey(encoded);
}


void CoreCertManager::setSslCert(const QByteArray &encoded)
{
    identity.setSslCert(encoded);
    CertManager::setSslCert(encoded);
}


#endif //HAVE_SSL

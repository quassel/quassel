/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

CoreIdentity::CoreIdentity(IdentityId id, QObject* parent)
    : Identity(id, parent)
    , _certManager(this)
{
    connect(this, &Identity::idSet, &_certManager, &CoreCertManager::setId);
    connect(&_certManager, &SyncableObject::updated, this, &SyncableObject::updated);
}

CoreIdentity::CoreIdentity(const Identity& other, QObject* parent)
    : Identity(other, parent)
    , _certManager(this)
{
    connect(this, &Identity::idSet, &_certManager, &CoreCertManager::setId);
    connect(&_certManager, &SyncableObject::updated, this, &SyncableObject::updated);
}

CoreIdentity::CoreIdentity(const CoreIdentity& other, QObject* parent)
    : Identity(other, parent)
    , _sslKey(other._sslKey)
    , _sslCert(other._sslCert)
    , _certManager(this)
{
    connect(this, &Identity::idSet, &_certManager, &CoreCertManager::setId);
    connect(&_certManager, &SyncableObject::updated, this, &SyncableObject::updated);
}

void CoreIdentity::synchronize(SignalProxy* proxy)
{
    proxy->synchronize(this);
    proxy->synchronize(&_certManager);
}

void CoreIdentity::setSslKey(const QByteArray& encoded)
{
    QSslKey key(encoded, QSsl::Rsa);
    if (key.isNull())
        key = QSslKey(encoded, QSsl::Ec);
    if (key.isNull())
        key = QSslKey(encoded, QSsl::Dsa);
    setSslKey(key);
}

void CoreIdentity::setSslCert(const QByteArray& encoded)
{
    setSslCert(QSslCertificate(encoded));
}

// ========================================
//  CoreCertManager
// ========================================

CoreCertManager::CoreCertManager(CoreIdentity* identity)
    : CertManager(identity->id())
    , _identity(identity)
{
    setAllowClientUpdates(true);
}

void CoreCertManager::setId(IdentityId id)
{
    setObjectName(QString::number(id.toInt()));
}

void CoreCertManager::setSslKey(const QByteArray& encoded)
{
    _identity->setSslKey(encoded);
    CertManager::setSslKey(encoded);
}

void CoreCertManager::setSslCert(const QByteArray& encoded)
{
    _identity->setSslCert(encoded);
    CertManager::setSslCert(encoded);
}

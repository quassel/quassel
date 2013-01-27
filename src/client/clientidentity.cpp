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

#include "clientidentity.h"

#include "client.h"
#include "signalproxy.h"

INIT_SYNCABLE_OBJECT(CertIdentity)
CertIdentity::CertIdentity(IdentityId id, QObject *parent)
    : Identity(id, parent)
#ifdef HAVE_SSL
    ,  _certManager(0),
    _isDirty(false)
#endif
{
}


CertIdentity::CertIdentity(const Identity &other, QObject *parent)
    : Identity(other, parent)
#ifdef HAVE_SSL
    , _certManager(0),
    _isDirty(false)
#endif
{
}


CertIdentity::CertIdentity(const CertIdentity &other, QObject *parent)
    : Identity(other, parent)
#ifdef HAVE_SSL
    , _certManager(0),
    _isDirty(other._isDirty),
    _sslKey(other._sslKey),
    _sslCert(other._sslCert)
#endif
{
}


#ifdef HAVE_SSL
void CertIdentity::enableEditSsl(bool enable)
{
    if (!enable || _certManager)
        return;

    _certManager = new ClientCertManager(id(), this);
    if (isValid()) { // this means we are not a newly created Identity but have a proper Id
        Client::signalProxy()->synchronize(_certManager);
        connect(_certManager, SIGNAL(updated()), this, SLOT(markClean()));
        connect(_certManager, SIGNAL(initDone()), this, SLOT(markClean()));
    }
}


void CertIdentity::setSslKey(const QSslKey &key)
{
    if (key.toPem() == _sslKey.toPem())
        return;
    _sslKey = key;
    _isDirty = true;
}


void CertIdentity::setSslCert(const QSslCertificate &cert)
{
    if (cert.toPem() == _sslCert.toPem())
        return;
    _sslCert = cert;
    _isDirty = true;
}


void CertIdentity::requestUpdateSslSettings()
{
    if (!_certManager)
        return;

    _certManager->requestUpdate(_certManager->toVariantMap());
}


void CertIdentity::markClean()
{
    _isDirty = false;
    emit sslSettingsUpdated();
}


// ========================================
//  ClientCertManager
// ========================================
void ClientCertManager::setSslKey(const QByteArray &encoded)
{
    QSslKey key(encoded, QSsl::Rsa);
    if (key.isNull())
        key = QSslKey(encoded, QSsl::Dsa);
    _certIdentity->setSslKey(key);
}


void ClientCertManager::setSslCert(const QByteArray &encoded)
{
    _certIdentity->setSslCert(QSslCertificate(encoded));
}


#endif // HAVE_SSL

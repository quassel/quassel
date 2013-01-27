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

#ifndef CLIENTIDENTITY_H
#define CLIENTIDENTITY_H

#include "identity.h"

class ClientCertManager;

class CertIdentity : public Identity
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    CertIdentity(IdentityId id = 0, QObject *parent = 0);
    CertIdentity(const Identity &other, QObject *parent = 0);
    CertIdentity(const CertIdentity &other, QObject *parent = 0);

#ifdef HAVE_SSL
    inline bool isDirty() const { return _isDirty; }
#else
    inline bool isDirty() const { return false; }
#endif

#ifdef HAVE_SSL
    void enableEditSsl(bool enable = true);
    inline const QSslKey &sslKey() const { return _sslKey; }
    inline const QSslCertificate &sslCert() const { return _sslCert; }

    void setSslKey(const QSslKey &key);
    void setSslCert(const QSslCertificate &cert);

public slots:
    void requestUpdateSslSettings();

signals:
    void sslSettingsUpdated();

private slots:
    void markClean();

private:
    ClientCertManager *_certManager;
    bool _isDirty;
    QSslKey _sslKey;
    QSslCertificate _sslCert;
#endif //HAVE_SSL
};


// ========================================
//  ClientCertManager
// ========================================
#ifdef HAVE_SSL
class ClientCertManager : public CertManager
{
    Q_OBJECT

public:
    ClientCertManager(IdentityId id, CertIdentity *parent) : CertManager(id, parent), _certIdentity(parent) {}

    virtual inline const QSslKey &sslKey() const { return _certIdentity->sslKey(); }
    virtual inline const QSslCertificate &sslCert() const { return _certIdentity->sslCert(); }

public slots:
    virtual void setSslKey(const QByteArray &encoded);
    virtual void setSslCert(const QByteArray &encoded);

private:
    CertIdentity *_certIdentity;
};


#endif //HAVE_SSL

#endif //CLIENTIDENTITY_H

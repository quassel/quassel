/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef COREIDENTITY_H
#define COREIDENTITY_H

#include "identity.h"

#include <QSslKey>
#include <QSslCertificate>

class CoreCertManager;
class CoreSession;
class SignalProxy;

class CoreIdentity : public Identity {
  Q_OBJECT

public:
  CoreIdentity(IdentityId id, SignalProxy *proxy, CoreSession *parent);
  CoreIdentity(const Identity &other, SignalProxy *proxy, CoreSession *parent);

  inline const QSslKey &sslKey() const { return _sslKey; }
  inline void setSslKey(const QSslKey &key) { _sslKey = key; }
  void setSslKey(const QByteArray &encoded);
  inline const QSslCertificate &sslCert() const { return _sslCert; }
  inline void setSslCert(const QSslCertificate &cert) { _sslCert = cert; }
  void setSslCert(const QByteArray &encoded);

public slots:
  virtual void update(const QVariantMap &properties);
  void save();

private:
  QSslKey _sslKey;
  QSslCertificate _sslCert;

  CoreCertManager *_certManager;
  CoreSession *_coreSession;
};


// ========================================
//  CoreCertManager
// ========================================
class CoreCertManager : public CertManager {
  Q_OBJECT

public:
  CoreCertManager(CoreIdentity *identity);

  inline CoreIdentity *identity() const { return _identity; }
  virtual inline const QSslKey &sslKey() const { return identity()->sslKey(); }
  virtual inline const QSslCertificate &sslCert() const { return identity()->sslCert(); }

public slots:
  virtual void setSslKey(const QByteArray &encoded);
  virtual void setSslCert(const QByteArray &encoded);

  void setId(IdentityId id);

private:
  CoreIdentity *_identity;
};

#endif //COREIDENTITY_H

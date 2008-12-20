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

#include "coreidentity.h"

#include "coresession.h"
#include "coreusersettings.h"
#include "signalproxy.h"

CoreIdentity::CoreIdentity(IdentityId id, SignalProxy *proxy, CoreSession *parent)
  : Identity(id, parent),
    _certManager(new CoreCertManager(this)),
    _coreSession(parent)
{
  proxy->synchronize(_certManager);
  connect(this, SIGNAL(idSet(IdentityId)), _certManager, SLOT(setId(IdentityId)));
}

CoreIdentity::CoreIdentity(const Identity &other, SignalProxy *proxy, CoreSession *parent)
  : Identity(other, parent),
    _certManager(new CoreCertManager(this)),
    _coreSession(parent)
{
  proxy->synchronize(_certManager);
  connect(this, SIGNAL(idSet(IdentityId)), _certManager, SLOT(setId(IdentityId)));
}

void CoreIdentity::update(const QVariantMap &properties) {
  SyncableObject::update(properties);
  save();
}

void CoreIdentity::save() {
  CoreUserSettings s(_coreSession->user());
  s.storeIdentity(*this);
}

void CoreIdentity::setSslKey(const QByteArray &encoded) {
  QSslKey key(encoded, QSsl::Rsa);
  if(key.isNull())
    key = QSslKey(encoded, QSsl::Dsa);
  setSslKey(key);
}

void CoreIdentity::setSslCert(const QByteArray &encoded) {
  setSslCert(QSslCertificate(encoded));
}


// ========================================
//  CoreCertManager
// ========================================
CoreCertManager::CoreCertManager(CoreIdentity *identity)
  : CertManager(identity->id(), identity),
    _identity(identity)
{
  setAllowClientUpdates(true);
}

void CoreCertManager::setSslKey(const QByteArray &encoded) {
  identity()->setSslKey(encoded);
  CertManager::setSslKey(encoded);
}

void CoreCertManager::setSslCert(const QByteArray &encoded) {
  identity()->setSslCert(encoded);
  CertManager::setSslCert(encoded);
}

void CoreCertManager::setId(IdentityId id) {
  renameObject(QString::number(id.toInt()));
}

/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include <QMetaProperty>
#include <QVariantMap>

#include "identity.h"

Identity::Identity(IdentityId id, QObject *parent) : QObject(parent), _identityId(id) {
  init();
  setToDefaults();
}

Identity::Identity(const Identity &other, QObject *parent) : QObject(parent),
            _identityId(other.id()),
            _identityName(other.identityName()),
            _realName(other.realName()),
            _nicks(other.nicks()),
            _awayNick(other.awayNick()),
            _awayReason(other.awayReason()),
            _returnMessage(other.returnMessage()) {
  init();
}

void Identity::init() {
  _initialized = false;
  setObjectName(QString::number(id()));
}

void Identity::setToDefaults() {
  setIdentityName(tr("Default Identity"));
  setRealName(tr("Quassel IRC User"));
  QStringList n;
  n << QString("quassel%1").arg(qrand() & 0xff); // FIXME provide more sensible default nicks
  setNicks(n);
  setAwayNick("");
  setAwayReason(tr("Gone fishing."));
  setReturnMessage(tr("Brought fish."));

}

bool Identity::initialized() const {
  return _initialized;
}

void Identity::setInitialized() {
  _initialized = true;
}

IdentityId Identity::id() const {
  return _identityId;
}

QString Identity::identityName() const {
  return _identityName;
}

QString Identity::realName() const {
  return _realName;
}

QStringList Identity::nicks() const {
  return _nicks;
}

QString Identity::awayNick() const {
  return _awayNick;
}

QString Identity::awayReason() const {
  return _awayReason;
}

QString Identity::returnMessage() const {
  return _returnMessage;
}

//////////////////////

void Identity::setIdentityName(const QString &identityName) {
  _identityName = identityName;
  emit identityNameSet(identityName);
}

void Identity::setRealName(const QString &realName) {
  _realName = realName;
  emit realNameSet(realName);
}

void Identity::setNicks(const QStringList &nicks) {
  _nicks = nicks;
  emit nicksSet(nicks);
}

void Identity::setAwayNick(const QString &nick) {
  _awayNick = nick;
  emit awayNickSet(nick);
}

void Identity::setAwayReason(const QString &reason) {
  _awayReason = reason;
  emit awayReasonSet(reason);
}

void Identity::setReturnMessage(const QString &message) {
  _returnMessage = message;
  emit returnMessageSet(message);
}

void Identity::update(const Identity &other) {
  for(int idx = 0; idx < metaObject()->propertyCount(); idx++) {
    QMetaProperty metaProp = metaObject()->property(metaObject()->propertyOffset() + idx);
    Q_ASSERT(metaProp.isValid());
    if(this->property(metaProp.name()) != other.property(metaProp.name())) {
      setProperty(metaProp.name(), other.property(metaProp.name()));
    }
  }
}

///////////////////////////////

// we use a hash, so we can easily extend identities without breaking saved ones
QDataStream &operator<<(QDataStream &out, const Identity &id) {
  QVariantMap i;
  i["IdentityId"] = id.id();
  i["IdentityName"] = id.identityName();
  i["RealName"] = id.realName();
  i["Nicks"] = id.nicks();
  i["AwayNick"] = id.awayNick();
  i["AwayReason"] = id.awayReason();
  i["ReturnMessage"] = id.returnMessage();
  out << i;
  return out;
}

QDataStream &operator>>(QDataStream &in, Identity &id) {
  QVariantMap i;
  in >> i;
  id._identityId = i["IdentityId"].toUInt();
  id.setIdentityName(i["IdentityName"].toString());
  id.setRealName(i["RealName"].toString());
  id.setNicks(i["Nicks"].toStringList());
  id.setAwayNick(i["AwayNick"].toString());
  id.setAwayReason(i["AwayReason"].toString());
  id.setReturnMessage(i["ReturnMessage"].toString());
  return in;
}

/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

Identity::Identity(IdentityId id, QObject *parent)
  : SyncableObject(parent),
    _identityId(id)
{
  init();
  setToDefaults();
}

Identity::Identity(const Identity &other, QObject *parent)
  : SyncableObject(parent),
    _identityId(other.id()),
    _identityName(other.identityName()),
    _realName(other.realName()),
    _nicks(other.nicks()),
    _awayNick(other.awayNick()),
    _awayNickEnabled(other.awayNickEnabled()),
    _awayReason(other.awayReason()),
    _awayReasonEnabled(other.awayReasonEnabled()),
    _autoAwayEnabled(other.autoAwayEnabled()),
    _autoAwayTime(other.autoAwayTime()),
    _autoAwayReason(other.autoAwayReason()),
    _autoAwayReasonEnabled(other.autoAwayReasonEnabled()),
    _detachAwayEnabled(other.detachAwayEnabled()),
    _detachAwayReason(other.detachAwayReason()),
    _detachAwayReasonEnabled(other.detachAwayReasonEnabled()),
    _ident(other.ident()),
    _kickReason(other.kickReason()),
    _partReason(other.partReason()),
    _quitReason(other.quitReason())
{
  init();
}

void Identity::init() {
  setObjectName(QString::number(id().toInt()));
  setAllowClientUpdates(true);
}

void Identity::setToDefaults() {
  setIdentityName(tr("<empty>"));
  setRealName(tr("Quassel IRC User"));
  QStringList n;
  n << QString("quassel%1").arg(qrand() & 0xff); // FIXME provide more sensible default nicks
  setNicks(n);
  setAwayNick("");
  setAwayNickEnabled(false);
  setAwayReason(tr("Gone fishing."));
  setAwayReasonEnabled(true);
  setAutoAwayEnabled(false);
  setAutoAwayTime(10);
  setAutoAwayReason(tr("Not here. No, really. not here!"));
  setAutoAwayReasonEnabled(false);
  setDetachAwayEnabled(false);
  setDetachAwayReason(tr("All Quassel clients vanished from the face of the earth..."));
  setDetachAwayReasonEnabled(false);
  setIdent("quassel");
  setKickReason(tr("Kindergarten is elsewhere!"));
  setPartReason(tr("http://quassel-irc.org - Chat comfortably. Anywhere."));
  setQuitReason(tr("http://quassel-irc.org - Chat comfortably. Anywhere."));
}

/*** setters ***/

void Identity::setId(IdentityId _id) {
  _identityId = _id;
  emit idSet(_id);
  renameObject(QString::number(id().toInt()));
}

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

void Identity::setAwayNickEnabled(bool enabled) {
  _awayNickEnabled = enabled;
  emit awayNickEnabledSet(enabled);
}

void Identity::setAwayReasonEnabled(bool enabled) {
  _awayReasonEnabled = enabled;
  emit awayReasonEnabledSet(enabled);
}

void Identity::setAutoAwayEnabled(bool enabled) {
  _autoAwayEnabled = enabled;
  emit autoAwayEnabledSet(enabled);
}

void Identity::setAutoAwayTime(int time) {
  _autoAwayTime = time;
  emit autoAwayTimeSet(time);
}

void Identity::setAutoAwayReason(const QString &reason) {
  _autoAwayReason = reason;
  emit autoAwayReasonSet(reason);
}

void Identity::setAutoAwayReasonEnabled(bool enabled) {
  _autoAwayReasonEnabled = enabled;
  emit autoAwayReasonEnabledSet(enabled);
}

void Identity::setDetachAwayEnabled(bool enabled) {
  _detachAwayEnabled = enabled;
  emit detachAwayEnabledSet(enabled);
}

void Identity::setDetachAwayReason(const QString &reason) {
  _detachAwayReason = reason;
  emit detachAwayReasonSet(reason);
}

void Identity::setDetachAwayReasonEnabled(bool enabled) {
  _detachAwayReasonEnabled = enabled;
  emit detachAwayReasonEnabledSet(enabled);
}

void Identity::setIdent(const QString &ident) {
  _ident = ident;
  emit identSet(ident);
}

void Identity::setKickReason(const QString &reason) {
  _kickReason = reason;
  emit kickReasonSet(reason);
}

void Identity::setPartReason(const QString &reason) {
  _partReason = reason;
  emit partReasonSet(reason);
}

void Identity::setQuitReason(const QString &reason) {
  _quitReason = reason;
  emit quitReasonSet(reason);
}

/***  ***/

void Identity::copyFrom(const Identity &other) {
  for(int idx = staticMetaObject.propertyOffset(); idx < staticMetaObject.propertyCount(); idx++) {
    QMetaProperty metaProp = staticMetaObject.property(idx);
    Q_ASSERT(metaProp.isValid());
    if(this->property(metaProp.name()) != other.property(metaProp.name())) {
      setProperty(metaProp.name(), other.property(metaProp.name()));
    }
  }
}

bool Identity::operator==(const Identity &other) const {
  for(int idx = staticMetaObject.propertyOffset(); idx < staticMetaObject.propertyCount(); idx++) {
    QMetaProperty metaProp = staticMetaObject.property(idx);
    Q_ASSERT(metaProp.isValid());
    QVariant v1 = this->property(metaProp.name());
    QVariant v2 = other.property(metaProp.name()); // qDebug() << v1 << v2;
    // QVariant cannot compare custom types, so we need to check for this special case
    if(QString(v1.typeName()) == "IdentityId") {
      if(v1.value<IdentityId>() != v2.value<IdentityId>()) return false;
    } else {
      if(v1 != v2) return false;
    }
  }
  return true;
}

bool Identity::operator!=(const Identity &other) const {
  return !(*this == other);
}

///////////////////////////////

QDataStream &operator<<(QDataStream &out, Identity id) {
  out << id.toVariantMap();
  return out;
}


QDataStream &operator>>(QDataStream &in, Identity &id) {
  QVariantMap i;
  in >> i;
  id.fromVariantMap(i);
  return in;
}


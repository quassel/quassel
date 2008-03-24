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

#include <QDebug>

#include "syncableobject.h"

#include "signalproxy.h"
#include "util.h"

SyncableObject::SyncableObject(QObject *parent) : QObject(parent) {
  _initialized = false;
}

SyncableObject::SyncableObject(const SyncableObject &other, QObject *parent) : QObject(parent) {
  _initialized = other._initialized;

}

bool SyncableObject::isInitialized() const {
  return _initialized;
}

void SyncableObject::setInitialized() {
  _initialized = true;
  emit initDone();
}

QVariantMap SyncableObject::toVariantMap() {
  QVariantMap properties;

  const QMetaObject* meta = metaObject();

  // we collect data from properties
  for(int i = 0; i < meta->propertyCount(); i++) {
    QMetaProperty prop = meta->property(i);
    properties[QString(prop.name())] = prop.read(this);
  }

  // ...as well as methods, which have names starting with "init"
  for(int i = 0; i < meta->methodCount(); i++) {
    QMetaMethod method = meta->method(i);
    QString methodname(::methodName(method));
    if(!methodname.startsWith("init") || methodname.startsWith("initSet") || methodname.startsWith("initDone"))
      continue;

    QVariant::Type variantType = QVariant::nameToType(method.typeName());
    if(variantType == QVariant::Invalid && !QByteArray(method.typeName()).isEmpty()) {
      qWarning() << "SyncableObject::toVariantMap(): cannot fetch init data for:" << this << method.signature() << "- Returntype is unknown to Qt's MetaSystem:" << QByteArray(method.typeName());
      continue;
    }
    QVariant value = QVariant(variantType);
    QGenericReturnArgument genericvalue = QGenericReturnArgument(method.typeName(), &value);
    QMetaObject::invokeMethod(this, methodname.toAscii(), genericvalue);

    properties[SignalProxy::methodBaseName(method)] = value;
    // qDebug() << ">>> SYNC:" << methodBaseName(method) << value;
  }
  // properties["Payload"] = QByteArray(10000000, 'a');  // for testing purposes
  return properties;

}

void SyncableObject::fromVariantMap(const QVariantMap &properties) {
  const QMetaObject *meta = metaObject();

  QVariantMap::const_iterator iterator = properties.constBegin();
  while(iterator != properties.constEnd()) {
    QString name = iterator.key();
    int propertyIndex = meta->indexOfProperty(name.toAscii());

    if(propertyIndex == -1 || !meta->property(propertyIndex).isWritable())
      setInitValue(name, iterator.value());
    else
      setProperty(name.toAscii(), iterator.value());
    // qDebug() << "<<< SYNC:" << name << iterator.value();
    iterator++;
  }
}

bool SyncableObject::setInitValue(const QString &property, const QVariant &value) {
  QString handlername = QString("initSet") + property;
  handlername[7] = handlername[7].toUpper();
  QGenericArgument param(value.typeName(), value.constData());
  return QMetaObject::invokeMethod(this, handlername.toAscii(), param);
}

void SyncableObject::renameObject(const QString &newName) {
  const QString oldName = objectName();
  if(oldName != newName) {
    setObjectName(newName);
    emit objectRenamed(newName, oldName);
  }
}

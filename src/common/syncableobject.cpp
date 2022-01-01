/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "syncableobject.h"

#include <QDebug>
#include <QMetaProperty>

#include "signalproxy.h"
#include "util.h"

SyncableObject::SyncableObject(QObject* parent)
    : SyncableObject(QString{}, parent)
{}

SyncableObject::SyncableObject(const QString& objectName, QObject* parent)
    : QObject(parent)
{
    _objectName = objectName;
    setObjectName(objectName);

    connect(this, &QObject::objectNameChanged, this, [this](auto&& newName) {
        for (auto&& proxy : _signalProxies) {
            proxy->renameObject(this, newName, _objectName);
        }
        _objectName = newName;
    });
}

SyncableObject::~SyncableObject()
{
    QList<SignalProxy*>::iterator proxyIter = _signalProxies.begin();
    while (proxyIter != _signalProxies.end()) {
        SignalProxy* proxy = (*proxyIter);
        proxyIter = _signalProxies.erase(proxyIter);
        proxy->stopSynchronize(this);
    }
}

bool SyncableObject::isInitialized() const
{
    return _initialized;
}

void SyncableObject::setInitialized()
{
    _initialized = true;
    emit initDone();
}

QVariantMap SyncableObject::toVariantMap()
{
    QVariantMap properties;

    const QMetaObject* meta = metaObject();

    // we collect data from properties
    QMetaProperty prop;
    QString propName;
    for (int i = 0; i < meta->propertyCount(); i++) {
        prop = meta->property(i);
        propName = QString(prop.name());
        if (propName == "objectName")
            continue;
        properties[propName] = prop.read(this);
    }

    // ...as well as methods, which have names starting with "init"
    for (int i = 0; i < meta->methodCount(); i++) {
        QMetaMethod method = meta->method(i);
        QString methodname(SignalProxy::ExtendedMetaObject::methodName(method));
        if (!methodname.startsWith("init") || methodname.startsWith("initSet") || methodname.startsWith("initDone"))
            continue;

        QVariant::Type variantType = QVariant::nameToType(method.typeName());
        if (variantType == QVariant::Invalid && !QByteArray(method.typeName()).isEmpty()) {
            qWarning() << "SyncableObject::toVariantMap(): cannot fetch init data for:" << this << method.methodSignature()
                       << "- Returntype is unknown to Qt's MetaSystem:" << QByteArray(method.typeName());
            continue;
        }

        QVariant value(variantType, (const void*)nullptr);
        QGenericReturnArgument genericvalue = QGenericReturnArgument(method.typeName(), value.data());
        QMetaObject::invokeMethod(this, methodname.toLatin1(), genericvalue);

        properties[SignalProxy::ExtendedMetaObject::methodBaseName(method)] = value;
    }
    return properties;
}

void SyncableObject::fromVariantMap(const QVariantMap& properties)
{
    const QMetaObject* meta = metaObject();

    QVariantMap::const_iterator iterator = properties.constBegin();
    QString propName;
    while (iterator != properties.constEnd()) {
        propName = iterator.key();
        if (propName == "objectName") {
            ++iterator;
            continue;
        }

        int propertyIndex = meta->indexOfProperty(propName.toLatin1());

        if (propertyIndex == -1 || !meta->property(propertyIndex).isWritable())
            setInitValue(propName, iterator.value());
        else
            setProperty(propName.toLatin1(), iterator.value());
        // qDebug() << "<<< SYNC:" << name << iterator.value();
        ++iterator;
    }
}

bool SyncableObject::setInitValue(const QString& property, const QVariant& value)
{
    QString handlername = QString("initSet") + property;
    handlername[7] = handlername[7].toUpper();

    QString methodSignature = QString("%1(%2)").arg(handlername).arg(value.typeName());
    int methodIdx = metaObject()->indexOfMethod(methodSignature.toLatin1().constData());
    if (methodIdx < 0) {
        QByteArray normedMethodName = QMetaObject::normalizedSignature(methodSignature.toLatin1().constData());
        methodIdx = metaObject()->indexOfMethod(normedMethodName.constData());
    }
    if (methodIdx < 0) {
        return false;
    }

    QGenericArgument param(value.typeName(), value.constData());
    return QMetaObject::invokeMethod(this, handlername.toLatin1(), param);
}

void SyncableObject::update(const QVariantMap& properties)
{
    fromVariantMap(properties);
    SYNC(ARG(properties))
    emit updated();
}

void SyncableObject::requestUpdate(const QVariantMap& properties)
{
    if (allowClientUpdates()) {
        update(properties);
    }
    REQUEST(ARG(properties))
}

void SyncableObject::sync_call__(SignalProxy::ProxyMode modeType, const char* funcname, ...) const
{
    // qDebug() << Q_FUNC_INFO << modeType << funcname;
    foreach (SignalProxy* proxy, _signalProxies) {
        va_list ap;
        va_start(ap, funcname);
        proxy->sync_call__(this, modeType, funcname, ap);
        va_end(ap);
    }
}

void SyncableObject::synchronize(SignalProxy* proxy)
{
    if (_signalProxies.contains(proxy))
        return;
    _signalProxies << proxy;
}

void SyncableObject::stopSynchronize(SignalProxy* proxy)
{
    for (int i = 0; i < _signalProxies.count(); i++) {
        if (_signalProxies[i] == proxy) {
            _signalProxies.removeAt(i);
            break;
        }
    }
}

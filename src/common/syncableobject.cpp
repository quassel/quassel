/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
#include "types.h"
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

SyncableObject::SyncableObject(const SyncableObject& other, QObject* parent)
    : SyncableObject(QString{}, parent)
{
    _initialized = other._initialized;
    _allowClientUpdates = other._allowClientUpdates;
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

SyncableObject& SyncableObject::operator=(const SyncableObject& other)
{
    if (this == &other)
        return *this;

    _initialized = other._initialized;
    _allowClientUpdates = other._allowClientUpdates;
    return *this;
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
    auto count = syncMetaObject()->propertyCount();

    // Ignore properties defined by QObject itself
    for (int i = staticMetaObject.propertyOffset(); i < count; ++i) {
        auto property = syncMetaObject()->property(i);
        properties[property.name()] = property.read(this);
    }
    return properties;
}

void SyncableObject::fromVariantMap(const QVariantMap& properties)
{
    auto it = properties.constBegin();
    while (it != properties.constEnd()) {
        auto property = syncMetaObject()->property(propertyIndex(it.key()));
        if (property.isValid() && property.isWritable()) {
            property.write(this, it.value());
        }
        ++it;
    }
}

QByteArray SyncableObject::propertySetter(const QMetaMethod& notifySignal)
{
    if (!notifySignal.isValid()) {
        return {};
    }

    // Unfortunately, Qt provides no way to get the name of the property's write method. Thus, we have to impose
    // naming conventions and rely on heuristics until we can break protocol and explicitly support properties by name.
    QByteArray setter;
    const auto& signalName = notifySignal.name();
    if (signalName.startsWith("sync")) {
        // Allow custom setter names by prefixing the signal with "sync". Setter name needs to start with lower-case letter.
        setter = signalName.mid(4);
        setter[0] = QChar(setter[0]).toLower().toLatin1();
    }
    else if (signalName.endsWith("Changed")) {
        // Notify signals like "myFooChanged" get mapped to the corresponding "setMyFoo"
        setter = "set" + signalName.left(signalName.length() - 7);
        setter[3] = QChar(setter[3]).toUpper().toLatin1();
    }
    else if (signalName.endsWith("Set")) {
        // Same for signals ending with "Set"
        setter = "set" + signalName.left(signalName.length() - 3);
        setter[3] = QChar(setter[3]).toUpper().toLatin1();
    }
    return setter;
}

int SyncableObject::propertyIndex(const QString &propertyName) const
{
    // We cache the property indices per class, because Qt performs a string-based lookup each time a property is accessed.
    // Use thread_local storage to avoid having to lock the cache for each access.
    using PropertyNameMap = std::unordered_map<QString, int, Hash<QString>>;
    thread_local std::unordered_map<QByteArray, PropertyNameMap, Hash<QByteArray>> propertyNameCache;

    auto classIt = propertyNameCache.find(syncMetaObject()->className());
    if (classIt == propertyNameCache.end()) {
        // Fill cache with all known properties
        classIt = propertyNameCache.insert({syncMetaObject()->className(), PropertyNameMap{}}).first;
        auto count = syncMetaObject()->propertyCount();
        for (int i = staticMetaObject.propertyOffset(); i < count; i++) {
            auto property = syncMetaObject()->property(i);
            classIt->second.emplace(property.name(), property.propertyIndex());
        }
    }
    auto propIt = classIt->second.find(propertyName);
    return (propIt != classIt->second.end() ? propIt->second : -1);
}

bool SyncableObject::syncProperty(const QByteArray& setterName, const QVariant& value)
{
    using PropertyMap = std::unordered_map<QByteArray, QMetaProperty, Hash<QByteArray>>;
    thread_local std::unordered_map<QByteArray, PropertyMap, Hash<QByteArray>> propertyCache;

    auto classIt = propertyCache.find(syncMetaObject()->className());
    if (classIt == propertyCache.end()) {
        // Fill cache with all properties that declare a supported NOTIFY signal
        classIt = propertyCache.insert({syncMetaObject()->className(), PropertyMap{}}).first;
        auto count = syncMetaObject()->propertyCount();
        for (int i = staticMetaObject.propertyOffset(); i < count; i++) {
            auto property = syncMetaObject()->property(i);
            if (property.isWritable() && property.hasNotifySignal()) {
                auto setter = propertySetter(property.notifySignal());
                if (!setter.isEmpty()) {
                    classIt->second.emplace(std::move(setter), property);
                }
            }
        }
    }
    auto propIt = classIt->second.find(setterName);
    if (propIt != classIt->second.end()) {
        if (propIt->second.write(this, value)) {
            emit updatedRemotely();
            return true;
        }
    }
    return false;
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

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

#pragma once

#include "common-export.h"

#include <unordered_map>

#include <QDataStream>
#include <QMetaType>
#include <QObject>
#include <QVariantMap>

#include "signalproxy.h"

/**
 * This macro needs to be declared in syncable objects, next to the Q_OBJECT macro.
 *
 * @note: Specializations of a base syncable object for core/client must not use the macro;
 *        i.e., if you have Foo, ClientFoo and/or CoreFoo, the SYNCABLE_OBJECT macro would
 *        only be declared in the class declaration of Foo.
 */
#define SYNCABLE_OBJECT                                                                                                                    \
public:                                                                                                                                    \
    const QMetaObject* syncMetaObject() const final override { return &staticMetaObject; }                                                 \
                                                                                                                                           \
private:

#define SYNC(...) sync_call__(SignalProxy::Server, __func__, __VA_ARGS__);
#define REQUEST(...) sync_call__(SignalProxy::Client, __func__, __VA_ARGS__);

#define SYNC_OTHER(x, ...) sync_call__(SignalProxy::Server, #x, __VA_ARGS__);
#define REQUEST_OTHER(x, ...) sync_call__(SignalProxy::Client, #x, __VA_ARGS__);

#define ARG(x) const_cast<void*>(reinterpret_cast<const void*>(&x))
#define NO_ARG 0

class COMMON_EXPORT SyncableObject : public QObject
{
    Q_OBJECT

public:
    SyncableObject(QObject* parent = nullptr);
    SyncableObject(const QString& objectName, QObject* parent = nullptr);
    SyncableObject(const SyncableObject& other, QObject* parent = nullptr);
    ~SyncableObject() override;

    /**
     * Stores the object's state in a QVariantMap.
     *
     * The returned map contains the values of all properties, indexed by property name.
     *
     * @sa fromVariantMap
     *
     * @returns The object's state in a QVariantMap
     */
    QVariantMap toVariantMap();

    /**
     * Initializes the object's state from a given QVariantMap.
     *
     * Writable properties are updated from the values stored in the given map.
     *
     * @note Properties not contained in the map are left alone; they're not reset to default values.
     *
     * @sa toVariantMap
     *
     * @param properties Map containing property values, indexed by property name
     */
    void fromVariantMap(const QVariantMap& properties);

    virtual bool isInitialized() const;

    virtual const QMetaObject* syncMetaObject() const { return metaObject(); }

    inline void setAllowClientUpdates(bool allow) { _allowClientUpdates = allow; }
    inline bool allowClientUpdates() const { return _allowClientUpdates; }

    /**
     * Derives a property's setter name from the given NOTIFY signal.
     *
     * Unfortunately, Qt provides no way to get the name of the property's write method; only the NOTIFY signal,
     * if declared, is exposed with its meta information.
     * Thus, we have to rely on a naming convention and heuristics to derive the setter name from that signal:
     *
     * - If the signal name starts with "sync", the remainder of the name is used as the setter, with the first letter
     *   converted to lowercase; e.g. "syncAnySlot" becomes "anySlot". This allows for defining custom setter names.
     *
     * - If the signal name ends with "Set" or "Changed", then that suffix is removed and "set" prefixed, so "myFooChanged"
     *   becomes "setMyFoo". This covers a vast majority of existing setters and their notify signals, and is the recommended
     *   naming scheme when introducing new properties.
     *
     * If the given signal isn't valid, or if it does not adhere to the naming scheme, an empty byte array is returned.
     *
     * @param notifySignal A property's NOTIFY signal
     * @returns The property's setter name derived from the signal name if it adheres to the naming scheme, empty byte array otherwise
     */
    static QByteArray propertySetter(const QMetaMethod& notifySignal);

    /**
     * Checks if a property corresponds to the given setter name, and if successful, sets its value.
     *
     * Matches the given setterName against the setters derived from the supported properties (as determined by @a propertySetter),
     * and if a match is found, sets the property's value by calling QMetaProperty::write(), which is more efficient than
     * invoking the setter dynamically. This is mainly useful for syncing properties from a SyncMessage in a more efficient way.
     *
     * The supported properties are cached, so subsequent calls of this method are fast.
     *
     * @param setterName The setter name
     * @param value      The value to set
     * @returns true, if a property was found and the value could be set successfully
     */
    bool syncProperty(const QByteArray& setterName, const QVariant& value);

public slots:
    virtual void setInitialized();
    void requestUpdate(const QVariantMap& properties);
    virtual void update(const QVariantMap& properties);

protected:
    void sync_call__(SignalProxy::ProxyMode modeType, const char* funcname, ...) const;

    SyncableObject& operator=(const SyncableObject& other);

signals:
    void initDone();
    void updatedRemotely();
    void updated();

private:
    void synchronize(SignalProxy* proxy);
    void stopSynchronize(SignalProxy* proxy);

    int propertyIndex(const QString& propertyName) const;

private:
    QString _objectName;
    bool _initialized{false};
    bool _allowClientUpdates{false};

    QList<SignalProxy*> _signalProxies;

    friend class SignalProxy;
};

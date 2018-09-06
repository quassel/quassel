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
#define SYNCABLE_OBJECT \
    public: \
        const QMetaObject *syncMetaObject() const final override { \
            return &staticMetaObject; \
        } \
    private: \

#define SYNC(...) sync_call__(SignalProxy::Server, __func__, __VA_ARGS__);
#define REQUEST(...) sync_call__(SignalProxy::Client, __func__, __VA_ARGS__);

#define SYNC_OTHER(x, ...) sync_call__(SignalProxy::Server, #x, __VA_ARGS__);
#define REQUEST_OTHER(x, ...) sync_call__(SignalProxy::Client, #x, __VA_ARGS__);

#define ARG(x) const_cast<void *>(reinterpret_cast<const void *>(&x))
#define NO_ARG 0

class COMMON_EXPORT SyncableObject : public QObject
{
    Q_OBJECT

public:
    SyncableObject(QObject *parent = nullptr);
    SyncableObject(const QString &objectName, QObject *parent = nullptr);
    SyncableObject(const SyncableObject &other, QObject *parent = nullptr);
    ~SyncableObject() override;

    //! Stores the object's state into a QVariantMap.
    /** The default implementation takes dynamic properties as well as getters that have
     *  names starting with "init" and stores them in a QVariantMap. Override this method in
     *  derived classes in order to store the object state in a custom form.
     *  \note  This is used by SignalProxy to transmit the state of the object to clients
     *         that request the initial object state. Later updates use a different mechanism
     *         and assume that the state is completely covered by properties and init* getters.
     *         DO NOT OVERRIDE THIS unless you know exactly what you do!
     *
     *  \return The object's state in a QVariantMap
     */
    virtual QVariantMap toVariantMap();

    //! Initialize the object's state from a given QVariantMap.
    /** \see toVariantMap() for important information concerning this method.
     */
    virtual void fromVariantMap(const QVariantMap &properties);

    virtual bool isInitialized() const;

    virtual const QMetaObject *syncMetaObject() const { return metaObject(); }

    inline void setAllowClientUpdates(bool allow) { _allowClientUpdates = allow; }
    inline bool allowClientUpdates() const { return _allowClientUpdates; }

public slots:
    virtual void setInitialized();
    void requestUpdate(const QVariantMap &properties);
    virtual void update(const QVariantMap &properties);

protected:
    void sync_call__(SignalProxy::ProxyMode modeType, const char *funcname, ...) const;

    void renameObject(const QString &newName);
    SyncableObject &operator=(const SyncableObject &other);

signals:
    void initDone();
    void updatedRemotely();
    void updated();

private:
    void synchronize(SignalProxy *proxy);
    void stopSynchronize(SignalProxy *proxy);

    bool setInitValue(const QString &property, const QVariant &value);

    bool _initialized{false};
    bool _allowClientUpdates{false};

    QList<SignalProxy *> _signalProxies;

    friend class SignalProxy;
};

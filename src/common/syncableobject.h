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

#ifndef SYNCABLEOBJECT_H
#define SYNCABLEOBJECT_H

#include <QDataStream>
#include <QMetaType>
#include <QObject>
#include <QVariantMap>

#include "signalproxy.h"

#define SYNCABLE_OBJECT static const int _classNameOffset__;
#define INIT_SYNCABLE_OBJECT(x) const int x ::_classNameOffset__ = QByteArray(staticMetaObject.className()).length() + 2;

#ifdef Q_CC_MSVC
#    define SYNC(...) sync_call__(SignalProxy::Server, (__FUNCTION__ + _classNameOffset__), __VA_ARGS__);
#    define REQUEST(...) sync_call__(SignalProxy::Client, (__FUNCTION__ + _classNameOffset__), __VA_ARGS__);
#else
#    define SYNC(...) sync_call__(SignalProxy::Server, __func__, __VA_ARGS__);
#    define REQUEST(...) sync_call__(SignalProxy::Client, __func__, __VA_ARGS__);
#endif //Q_CC_MSVC

#define SYNC_OTHER(x, ...) sync_call__(SignalProxy::Server, #x, __VA_ARGS__);
#define REQUEST_OTHER(x, ...) sync_call__(SignalProxy::Client, #x, __VA_ARGS__);

#define ARG(x) const_cast<void *>(reinterpret_cast<const void *>(&x))
#define NO_ARG 0

class SyncableObject : public QObject
{
    SYNCABLE_OBJECT
        Q_OBJECT

public:
    SyncableObject(QObject *parent = 0);
    SyncableObject(const QString &objectName, QObject *parent = 0);
    SyncableObject(const SyncableObject &other, QObject *parent = 0);
    ~SyncableObject();

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

    virtual const QMetaObject *syncMetaObject() const { return metaObject(); };

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

    bool _initialized;
    bool _allowClientUpdates;

    QList<SignalProxy *> _signalProxies;

    friend class SignalProxy;
};


#endif

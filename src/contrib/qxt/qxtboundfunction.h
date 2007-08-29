/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/

#ifndef QXTBOUNDFUNCTION_H
#define QXTBOUNDFUNCTION_H

#include <QObject>
#include <QMetaObject>
#include <QGenericArgument>
#include <qxtmetaobject.h>
#include <qxtnull.h>
#include <QThread>

class QxtBoundFunction : public QObject
{
    Q_OBJECT
public:
    template <class T>
    inline QxtNullable<T> invoke(QXT_PROTO_10ARGS(QVariant))
    {
        if (QThread::currentThread() == parent()->thread())
            return invoke<T>(Qt::DirectConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); 
#if QT_VERSION >= 0x040300
        return invoke<T>(Qt::BlockingQueuedConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
#else
        qWarning() << "QxtBoundFunction::invoke: Cannot return a value using a queued connection";
        return QxtNull;
#endif
    }

    template <class T>
    QxtNullable<T> invoke(Qt::ConnectionType type, QXT_PROTO_10ARGS(QVariant))
    {
        if (type == Qt::QueuedConnection)
        {
            qWarning() << "QxtBoundFunction::invoke: Cannot return a value using a queued connection";
            return qxtNull;
        }
        T retval;
        // I know this is a totally ugly function call
        if (invoke(type, QGenericReturnArgument(qVariantFromValue<T>(*reinterpret_cast<T*>(0)).typeName(), reinterpret_cast<void*>(&retval)),
                   p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))
        {
            return retval;
        }
        else
        {
            return qxtNull;
        }
    }

    inline bool invoke(QXT_PROTO_10ARGS(QVariant))
    {
        return invoke(Qt::AutoConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    bool invoke(Qt::ConnectionType, QXT_PROTO_10ARGS(QVariant));

    inline bool invoke(QXT_PROTO_10ARGS(QGenericArgument))
    {
        return invoke(Qt::AutoConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    inline bool invoke(Qt::ConnectionType type, QXT_PROTO_10ARGS(QGenericArgument)) {
        return invoke(type, QGenericReturnArgument(), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }

    inline bool invoke(QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QVariant))
    {
        return invoke(Qt::AutoConnection, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    bool invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QVariant));

    inline bool invoke(QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument))
    {
        return invoke(Qt::AutoConnection, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    bool invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument));

protected:
    QxtBoundFunction(QObject* parent = 0);
    virtual bool invokeImpl(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument)) = 0;
};

#endif

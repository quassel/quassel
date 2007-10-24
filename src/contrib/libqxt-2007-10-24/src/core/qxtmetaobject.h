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
#ifndef QXTMETAOBJECT_H
#define QXTMETAOBJECT_H

#include <QMetaObject>
#include <QVariant>
#include <QGenericArgument>
#include <typeinfo>
#include "qxtnullable.h"
class QByteArray;
class QxtBoundArgument;
class QxtBoundFunction;

#define QXT_PROTO_10ARGS(T) T p1 = T(), T p2 = T(), T p3 = T(), T p4 = T(), T p5 = T(), T p6 = T(), T p7 = T(), T p8 = T(), T p9 = T(), T p10 = T()
#define QXT_PROTO_9ARGS(T) T p2 = T(), T p3 = T(), T p4 = T(), T p5 = T(), T p6 = T(), T p7 = T(), T p8 = T(), T p9 = T(), T p10 = T()
#define QXT_IMPL_10ARGS(T) T p1, T p2, T p3, T p4, T p5, T p6, T p7, T p8, T p9, T p10

class QxtGenericFunctionPointer {
template<typename FUNCTION>
friend QxtGenericFunctionPointer qxtFuncPtr(FUNCTION funcPtr);
public:
    QxtGenericFunctionPointer(const QxtGenericFunctionPointer& other) {
        funcPtr = other.funcPtr;
        typeName = other.typeName;
    }

    typedef void(voidFunc)();
    voidFunc* funcPtr;
    QByteArray typeName;

private:
    QxtGenericFunctionPointer(voidFunc* ptr, const QByteArray& typeIdName) {
        funcPtr = ptr;
        typeName = typeIdName;
    }
};

template<typename FUNCTION>
QxtGenericFunctionPointer qxtFuncPtr(FUNCTION funcPtr) {
    return QxtGenericFunctionPointer(reinterpret_cast<QxtGenericFunctionPointer::voidFunc*>(funcPtr), typeid(funcPtr).name());
}

namespace QxtMetaObject
{
QXT_CORE_EXPORT QByteArray methodName(const char* method);
QXT_CORE_EXPORT QByteArray methodSignature(const char* method);

QXT_CORE_EXPORT bool isSignalOrSlot (const char* method);

QXT_CORE_EXPORT QxtBoundFunction* bind(QObject* recv, const char* invokable, QXT_PROTO_10ARGS(QGenericArgument));
QXT_CORE_EXPORT QxtBoundFunction* bind(QObject* recv, const char* invokable, QVariant p1, QXT_PROTO_9ARGS(QVariant));
QXT_CORE_EXPORT bool connect(QObject* sender, const char* signal, QxtBoundFunction* slot,
                             Qt::ConnectionType type = Qt::AutoConnection);
}

/**
 * \relates QxtMetaObject
 * \sa QxtMetaObject::bind
 * Refers to the n'th parameter of QxtBoundFunction::invoke() or a signal connected to
 * a QxtBoundFunction.
 */
#define QXT_BIND(n) QGenericArgument("QxtBoundArgument", reinterpret_cast<void*>(n))

#endif // QXTMETAOBJECT_H

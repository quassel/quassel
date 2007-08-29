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

#ifndef QXTBOUNDCFUNCTION_H
#define QXTBOUNDCFUNCTION_H

#include <QObject>
#include <QMetaObject>
#include <QGenericArgument>
#include <qxtmetaobject.h>
#include <qxtboundfunctionbase.h>
#include <qxtnull.h>
#include <QThread>
/*
template <typename FUNCTION, int argc, typename RETURN = void>
class QxtBoundCFunction : public QxtBoundFunctionBase {
public:
    FUNCTION funcPtr;

    QxtBoundCFunction(QObject* parent, FUNCTION funcPointer, QGenericArgument* params[argc], QByteArray types[argc]) : QxtBoundFunctionBase(parent, params, types), funcPtr(funcPointer) {
        // initializers only, thanks to template magic
    }

    virtual bool invokeImpl(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument)) {
        return qxt_invoke_cfunction_return<FUNCTION, argc, RETURN>(funcPtr, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
};

template <typename FUNCTION, int argc>
class QxtBoundCFunction<FUNCTION, argc, void> : public QxtBoundFunctionBase {
public:
    FUNCTION funcPtr;

    QxtBoundCFunction(QObject* parent, FUNCTION funcPointer, QGenericArgument* params[argc], QByteArray types[argc]) : QxtBoundFunctionBase(parent, params, types), funcPtr(funcPointer) {
        // initializers only, thanks to template magic
    }

    virtual bool invokeImpl(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument)) {
        return qxt_invoke_cfunction<FUNCTION, argc>(funcPtr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
};

QGenericArgument* qbcfP[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
QByteArray types[10] = { "", "", "", "", "", "", "", "", "", "" };

void testFunction(int a) { a++; return; }
template < typename FP > QxtBoundFunction* testBind(FP fn) { return new QxtBoundCFunction<FP, 1>(0, fn, qbcfP, types); }
QxtBoundCFunction<void(*)(int), 1> qbcf(0, testFunction, qbcfP, types);
QxtBoundFunction* qbf = testBind(testFunction);
*/
#endif

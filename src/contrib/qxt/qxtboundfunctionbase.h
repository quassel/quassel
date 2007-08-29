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

// This file exists for the convenience of QxtBoundCFunction.
// It is not part of the public API and is subject to change.
// 
// We mean it.

#ifndef QXTBOUNDFUNCTIONBASE_H
#define QXTBOUNDFUNCTIONBASE_H

#include <QObject>
#include <QMetaObject>
#include <QGenericArgument>
#include <qxtmetaobject.h>
#include <qxtboundfunction.h>

class QxtBoundFunctionBase : public QxtBoundFunction
{
public:
    QByteArray bindTypes[10];
    QGenericArgument arg[10], p[10];
    void* data[10];

    QxtBoundFunctionBase(QObject* parent, QGenericArgument* params[10], QByteArray types[10]);
    virtual ~QxtBoundFunctionBase();

    int qt_metacall(QMetaObject::Call _c, int _id, void **_a);
    bool invokeBase(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument));
};

#endif

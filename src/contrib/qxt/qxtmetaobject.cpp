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
/**
\class QxtMetaObject QxtMetaObject

\ingroup core

\brief provides extensions to QMetaObject

including QxtMetaObject::bind \n

*/
#include "qxtmetaobject.h"
#include "qxtboundfunction.h"
#include "qxtboundcfunction.h"

#include <QByteArray>
#include <QMetaObject>
#include <QMetaMethod>
#include <QtDebug>

class QxtBoundArgument
{
    // This class intentionally left blank
};
Q_DECLARE_METATYPE(QxtBoundArgument)

class QxtBoundFunctionBase;

QxtBoundFunction::QxtBoundFunction(QObject* parent) : QObject(parent)
{
    // initializer only
}

#define QXT_ARG(i) ((argCount>i)?QGenericArgument(p ## i .typeName(), p ## i .constData()):QGenericArgument())
#define QXT_VAR_ARG(i) (p ## i .isValid())?QGenericArgument(p ## i .typeName(), p ## i .constData()):QGenericArgument()
bool QxtBoundFunction::invoke(Qt::ConnectionType type, QXT_IMPL_10ARGS(QVariant))
{
    return invoke(type, QXT_VAR_ARG(1), QXT_VAR_ARG(2), QXT_VAR_ARG(3), QXT_VAR_ARG(4), QXT_VAR_ARG(5), QXT_VAR_ARG(6), QXT_VAR_ARG(7), QXT_VAR_ARG(8), QXT_VAR_ARG(9), QXT_VAR_ARG(10));
}

bool QxtBoundFunction::invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QVariant))
{
    return invoke(type, returnValue, QXT_VAR_ARG(1), QXT_VAR_ARG(2), QXT_VAR_ARG(3), QXT_VAR_ARG(4), QXT_VAR_ARG(5), QXT_VAR_ARG(6), QXT_VAR_ARG(7), QXT_VAR_ARG(8), QXT_VAR_ARG(9), QXT_VAR_ARG(10));
}

QxtBoundFunctionBase::QxtBoundFunctionBase(QObject* parent, QGenericArgument* params[10], QByteArray types[10]) : QxtBoundFunction(parent)
{
    for (int i=0; i<10; i++)
    {
        if (!params[i]) break;
        if (QByteArray(params[i]->name()) == "QxtBoundArgument")
        {
            arg[i] = QGenericArgument("QxtBoundArgument", params[i]->data());
        }
        else
        {
            data[i] = QMetaType::construct(QMetaType::type(params[i]->name()), params[i]->data());
            arg[i] = p[i] = QGenericArgument(params[i]->name(), data[i]);
        }
        bindTypes[i] = types[i];
    }
}

QxtBoundFunctionBase::~QxtBoundFunctionBase()
{
    for (int i=0; i<10; i++)
    {
        if (arg[i].name() == 0) return;
        if (QByteArray(arg[i].name()) != "QxtBoundArgument") QMetaType::destroy(QMetaType::type(arg[i].name()), arg[i].data());
    }
}

int QxtBoundFunctionBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod)
    {
        if (_id == 0)
        {
            for (int i = 0; i < 10; i++)
            {
                if (QByteArray(arg[i].name()) == "QxtBoundArgument")
                {
                    p[i] = QGenericArgument(bindTypes[i].constData(), _a[(quintptr)(arg[i].data())]);
                }
            }
            invokeImpl(Qt::DirectConnection, QGenericReturnArgument(), p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
        }
        _id = -1;
    }
    return _id;
}

bool QxtBoundFunctionBase::invokeBase(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument))
{
    QGenericArgument* args[10] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10 };
    for (int i = 0; i < 10; i++)
    {
        if (QByteArray(arg[i].name()) == "QxtBoundArgument")
        {
            p[i] = *args[(quintptr)(arg[i].data())-1];
        }
    }
    return invokeImpl(type, returnValue, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
}

bool QxtBoundFunction::invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument))
{
    return reinterpret_cast<QxtBoundFunctionBase*>(this)->invokeBase(type, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

class QxtBoundSlot : public QxtBoundFunctionBase
{
public:
    QByteArray sig;

    QxtBoundSlot(QObject* receiver, const char* invokable, QGenericArgument* params[10], QByteArray types[10]) : QxtBoundFunctionBase(receiver, params, types), sig(invokable)
    {
        // initializers only
    }

    virtual bool invokeImpl(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument))
    {
        if (!QMetaObject::invokeMethod(parent(), QxtMetaObject::methodName(sig.constData()), type, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))
        {
            qWarning() << "QxtBoundFunction: call to" << sig << "failed";
            return false;
        }
        return true;
    }
};

namespace QxtMetaObject
{

/*!
\relates QxtMetaObject

    \fn methodName(const char* method)

    Returns the name of the given method.

    Example usage:
    \code
    QByteArray method = QxtMetaObject::methodName(" int foo ( int bar, double baz )");
    // method is now "foo"
    \endcode
 */
QByteArray methodName(const char* method)
{
    QByteArray name = methodSignature(method);
    const int idx = name.indexOf("(");
    if (idx != -1)
        name.truncate(idx);
    return name;
}

/*!
\relates QxtMetaObject

\fn methodSignature(const char* method)

Returns the signature of the given method.
 */
QByteArray methodSignature(const char* method)
{
    QByteArray name = QMetaObject::normalizedSignature(method);
    if (name.startsWith("1") || name.startsWith("2"))
        return name.mid(1);
    return name;
}

/*!
\relates QxtMetaObject

\fn bool isSignalOrSlot(const char* method)

checks if \p method contains parantesis and begins with 1 or 2 */
bool isSignalOrSlot (const char* method)
{
    QByteArray m(method);
    return (m.count() && (m[0] == '1'||m[0] == '2') && m.contains('(') && m.contains(')'));
}

/**
\relates QxtMetaObject
\fn bind(QObject* recv, const char* invokable, QXT_IMPL_10ARGS(QVariant))

creates a QxtBoundFunction from a slot + arguments \n
can be used for QxtMetaObject::connect \

\code
QxtMetaObject::connect(\n
	this, SIGNAL(init()), \\n
	QxtMetaObject::bind(this, SLOT(say(QString)), Q_ARG(QString,"hello")));
\endcode
\n
\code
QxtMetaObject::connect( \n
	this, SIGNAL(init(int i)), \n
	QxtMetaObject::bind(this, SLOT(say(QString),int), Q_ARG(QString,"hello"),Q_BIND(1)));
\endcode

 */
QxtBoundFunction* bind(QObject* recv, const char* invokable, QXT_IMPL_10ARGS(QVariant))
{
    if (!recv)
    {
        qWarning() << "QxtMetaObject::bind: cannot connect to null QObject";
        return 0;
    }

    QVariant* args[10] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10 };
    QByteArray connSlot("2"), recvSlot(QMetaObject::normalizedSignature(invokable));
    const QMetaObject* meta = recv->metaObject();
    int methodID = meta->indexOfMethod(QxtMetaObject::methodSignature(recvSlot.constData()));
    if (methodID == -1)
    {
        qWarning() << "QxtMetaObject::bind: no such method " << recvSlot;
        return 0;
    }
    QMetaMethod method = meta->method(methodID);
    int argCount = method.parameterTypes().count();
    const QList<QByteArray> paramTypes = method.parameterTypes();

    for (int i=0; i<argCount; i++)
    {
        if (paramTypes[i] == "QxtBoundArgument") continue;
        int type = QMetaType::type(paramTypes[i].constData());
        if (!args[i]->canConvert((QVariant::Type)type))
        {
            qWarning() << "QxtMetaObject::bind: incompatible parameter list for " << recvSlot;
            return 0;
        }
    }

    return QxtMetaObject::bind(recv, invokable, QXT_ARG(1), QXT_ARG(2), QXT_ARG(3), QXT_ARG(4), QXT_ARG(5), QXT_ARG(6), QXT_ARG(7), QXT_ARG(8), QXT_ARG(9), QXT_ARG(10));
}

QxtBoundFunction* bind(QObject* recv, const char* invokable, QXT_IMPL_10ARGS(QGenericArgument))
{
    if (!recv)
    {
        qWarning() << "QxtMetaObject::bind: cannot connect to null QObject";
        return 0;
    }

    QGenericArgument* args[10] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10 };
    QByteArray connSlot("2"), recvSlot(QMetaObject::normalizedSignature(invokable)), bindTypes[10];
    const QMetaObject* meta = recv->metaObject();
    int methodID = meta->indexOfMethod(QxtMetaObject::methodSignature(recvSlot.constData()).constData());
    if (methodID == -1)
    {
        qWarning() << "QxtMetaObject::bind: no such method " << recvSlot;
        return 0;
    }
    QMetaMethod method = meta->method(methodID);
    int argCount = method.parameterTypes().count();

    connSlot += QxtMetaObject::methodName(invokable) + "(";
    for (int i=0; i<10; i++)
    {
        if (args[i]->name() == 0) break;        // done
        if (i >= argCount)
        {
            qWarning() << "QxtMetaObject::bind: too many arguments passed to " << invokable;
            return 0;
        }
        if (i > 0) connSlot += ",";             // argument separator
        if (QByteArray(args[i]->name()) == "QxtBoundArgument")
        {
            Q_ASSERT_X((quintptr)(args[i]->data()) > 0 && (quintptr)(args[i]->data()) <= 10, "QXT_BIND", "invalid argument number");
            connSlot += method.parameterTypes()[i];
            bindTypes[i] = method.parameterTypes()[i];
        }
        else
        {
            connSlot += args[i]->name();        // type name
        }
    }
    connSlot = QMetaObject::normalizedSignature(connSlot += ")");

    if (!QMetaObject::checkConnectArgs(recvSlot.constData(), connSlot.constData()))
    {
        qWarning() << "QxtMetaObject::bind: provided parameters " << connSlot.mid(connSlot.indexOf('(')) << " is incompatible with " << invokable;
        return 0;
    }

    return new QxtBoundSlot(recv, invokable, args, bindTypes);
}

/**
\relates QxtMetaObject
\fn connect(QObject* sender, const char* signal, QxtBoundFunction* slot, Qt::ConnectionType type) {

connects a signal to a QxtBoundFunction \n
 */
bool connect(QObject* sender, const char* signal, QxtBoundFunction* slot, Qt::ConnectionType type)
{
    const QMetaObject* meta = sender->metaObject();
    int methodID = meta->indexOfMethod(meta->normalizedSignature(signal).mid(1).constData());
    if (methodID < 0)
    {
        qWarning() << "QxtMetaObject::connect: no such signal: " << QByteArray(signal).mid(1);
        return false;
    }

    return QMetaObject::connect(sender, methodID, slot, QObject::staticMetaObject.methodCount(), (int)(type));
}

}

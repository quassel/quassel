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

#include "basichandler.h"

#include <utility>

#include <QDebug>
#include <QMetaMethod>
#include <QMetaObject>
#include <QStringList>

BasicHandler::BasicHandler(QObject* parent)
    : QObject(parent)
    , _methodPrefix("handle")
{
}

BasicHandler::BasicHandler(QString methodPrefix, QObject* parent)
    : QObject(parent)
    , _methodPrefix(std::move(methodPrefix))
{
}

QStringList BasicHandler::providesHandlers()
{
    return handlerHash().keys();
}

const QHash<QString, int>& BasicHandler::handlerHash()
{
    if (!_initDone) {
        for (int i = metaObject()->methodOffset(); i < metaObject()->methodCount(); i++) {
            QString methodSignature = metaObject()->method(i).methodSignature();
            if (methodSignature.startsWith("defaultHandler")) {
                _defaultHandler = i;
                continue;
            }

            if (!methodSignature.startsWith(_methodPrefix))
                continue;

            methodSignature = methodSignature.section('(', 0, 0);           // chop the parameter list
            methodSignature = methodSignature.mid(_methodPrefix.length());  // strip "handle" or whatever the prefix is
            _handlerHash[methodSignature] = i;
        }
        _initDone = true;
    }
    return _handlerHash;
}

void BasicHandler::handle(const QString& member,
                          QGenericArgument val0,
                          QGenericArgument val1,
                          QGenericArgument val2,
                          QGenericArgument val3,
                          QGenericArgument val4,
                          QGenericArgument val5,
                          QGenericArgument val6,
                          QGenericArgument val7,
                          QGenericArgument val8)
{
    // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
    // and now we even have a fast lookup! Thanks thiago!

    QString handler = member.toLower();
    handler[0] = handler[0].toUpper();
    QByteArray methodName;
    int methodIndex = -1;

    if (!handlerHash().contains(handler)) {
        if (_defaultHandler == -1) {
            qWarning() << QString("No such Handler: %1::%2%3").arg(metaObject()->className(), _methodPrefix, handler);
            return;
        }
        methodIndex = _defaultHandler;
        methodName = QString::fromUtf8(metaObject()->method(_defaultHandler).methodSignature()).split('(').first().toUtf8();
    }
    else {
        methodIndex = handlerHash()[handler];
        methodName = QString::fromUtf8(metaObject()->method(methodIndex).methodSignature()).split('(').first().toUtf8();
    }

    // Prepare arguments to match original param array
    QGenericArgument args[10]
        = {QGenericArgument("QString", static_cast<const void*>(&member)), val0, val1, val2, val3, val4, val5, val6, val7, val8};

    // Check parameter count
    const QMetaMethod method = metaObject()->method(methodIndex);
    int paramCount = method.parameterCount();

    bool success = false;
    if (paramCount <= 10) {
        // Use invokeMethod for up to 10 arguments
        success = QMetaObject::invokeMethod(this,
                                            methodName.constData(),
                                            Qt::DirectConnection,
                                            args[0],  // member
                                            args[1],  // val0
                                            args[2],  // val1
                                            args[3],  // val2
                                            args[4],  // val3
                                            args[5],  // val4
                                            args[6],  // val5
                                            args[7],  // val6
                                            args[8],  // val7
                                            args[9]   // val8
        );
    }
    else {
        // Fallback to metacall for >10 arguments, preserving original param array
        void* param[] = {
            nullptr,                                            // return value
            static_cast<void*>(const_cast<QString*>(&member)),  // member
            val0.data(),
            val1.data(),
            val2.data(),
            val3.data(),
            val4.data(),
            val5.data(),
            val6.data(),
            val7.data(),
            val8.data(),
            val8.data()  // duplicate val8
        };
        if (methodIndex == _defaultHandler) {
            metaObject()->metacall(this, QMetaObject::InvokeMetaMethod, methodIndex, param);
            success = true;  // metacall does not return a success flag
        }
        else {
            void* param_specific[] = {nullptr,  // return value
                                      val0.data(),
                                      val1.data(),
                                      val2.data(),
                                      val3.data(),
                                      val4.data(),
                                      val5.data(),
                                      val6.data(),
                                      val7.data(),
                                      val8.data(),
                                      val8.data(),  // duplicate val8
                                      nullptr};
            metaObject()->metacall(this, QMetaObject::InvokeMetaMethod, methodIndex, param_specific);
            success = true;  // metacall does not return a success flag
        }
    }

    if (!success) {
        qWarning() << QString("Failed to invoke method %1::%2%3").arg(metaObject()->className(), _methodPrefix, handler);
    }
}

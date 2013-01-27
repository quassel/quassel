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

#include "basichandler.h"

#include <QMetaMethod>

#include "logger.h"

BasicHandler::BasicHandler(QObject *parent)
    : QObject(parent),
    _defaultHandler(-1),
    _initDone(false),
    _methodPrefix("handle")
{
}


BasicHandler::BasicHandler(const QString &methodPrefix, QObject *parent)
    : QObject(parent),
    _defaultHandler(-1),
    _initDone(false),
    _methodPrefix(methodPrefix)
{
}


QStringList BasicHandler::providesHandlers()
{
    return handlerHash().keys();
}


const QHash<QString, int> &BasicHandler::handlerHash()
{
    if (!_initDone) {
        for (int i = metaObject()->methodOffset(); i < metaObject()->methodCount(); i++) {
            QString methodSignature(metaObject()->method(i).signature());
            if (methodSignature.startsWith("defaultHandler")) {
                _defaultHandler = i;
                continue;
            }

            if (!methodSignature.startsWith(_methodPrefix))
                continue;

            methodSignature = methodSignature.section('(', 0, 0); // chop the attribute list
            methodSignature = methodSignature.mid(_methodPrefix.length()); // strip "handle" or whatever the prefix is
            _handlerHash[methodSignature] = i;
        }
        _initDone = true;
    }
    return _handlerHash;
}


void BasicHandler::handle(const QString &member, QGenericArgument val0,
    QGenericArgument val1, QGenericArgument val2,
    QGenericArgument val3, QGenericArgument val4,
    QGenericArgument val5, QGenericArgument val6,
    QGenericArgument val7, QGenericArgument val8)
{
    // Now we try to find a handler for this message. BTW, I do love the Trolltech guys ;-)
    // and now we even have a fast lookup! Thanks thiago!

    QString handler = member.toLower();
    handler[0] = handler[0].toUpper();

    if (!handlerHash().contains(handler)) {
        if (_defaultHandler == -1) {
            qWarning() << QString("No such Handler: %1::%2%3").arg(metaObject()->className(), _methodPrefix, handler);
            return;
        }
        else {
            void *param[] = { 0, Q_ARG(QString, member).data(), val0.data(), val1.data(), val2.data(), val3.data(), val4.data(),
                              val5.data(), val6.data(), val7.data(), val8.data(), val8.data() };
            qt_metacall(QMetaObject::InvokeMetaMethod, _defaultHandler, param);
            return;
        }
    }

    void *param[] = { 0, val0.data(), val1.data(), val2.data(), val3.data(), val4.data(),
                      val5.data(), val6.data(), val7.data(), val8.data(), val8.data(), 0 };
    qt_metacall(QMetaObject::InvokeMetaMethod, handlerHash()[handler], param);
}

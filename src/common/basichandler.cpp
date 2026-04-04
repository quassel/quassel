/***************************************************************************
 *   Copyright (C) 2005-2026 by the Quassel Project                        *
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

BasicHandler::BasicHandler(QObject* parent)
    : QObject(parent)
    , _methodPrefix("handle")
{}

BasicHandler::BasicHandler(QString methodPrefix, QObject* parent)
    : QObject(parent)
    , _methodPrefix(std::move(methodPrefix))
{}

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

            methodSignature = methodSignature.section('(', 0, 0);           // chop the attribute list
            methodSignature = methodSignature.mid(_methodPrefix.length());  // strip "handle" or whatever the prefix is
            _handlerHash[methodSignature] = i;
        }
        _initDone = true;
    }
    return _handlerHash;
}

namespace {

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
void* metaCallData(QMetaMethodArgument argument)
{
    return const_cast<void*>(argument.data);
}
#else
void* metaCallData(QGenericArgument argument)
{
    return argument.data();
}
#endif

}  // namespace

void BasicHandler::handle(const QString& member,
                          MetaCallArgument val0,
                          MetaCallArgument val1,
                          MetaCallArgument val2,
                          MetaCallArgument val3,
                          MetaCallArgument val4,
                          MetaCallArgument val5,
                          MetaCallArgument val6,
                          MetaCallArgument val7,
                          MetaCallArgument val8)
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
            const auto memberArg = Q_ARG(QString, member);
            void* param[] = {nullptr,
                             metaCallData(memberArg),
                             metaCallData(val0),
                             metaCallData(val1),
                             metaCallData(val2),
                             metaCallData(val3),
                             metaCallData(val4),
                             metaCallData(val5),
                             metaCallData(val6),
                             metaCallData(val7),
                             metaCallData(val8),
                             metaCallData(val8)};
            qt_metacall(QMetaObject::InvokeMetaMethod, _defaultHandler, param);
            return;
        }
    }

    void* param[] = {nullptr,
                     metaCallData(val0),
                     metaCallData(val1),
                     metaCallData(val2),
                     metaCallData(val3),
                     metaCallData(val4),
                     metaCallData(val5),
                     metaCallData(val6),
                     metaCallData(val7),
                     metaCallData(val8),
                     metaCallData(val8),
                     nullptr};
    qt_metacall(QMetaObject::InvokeMetaMethod, handlerHash()[handler], param);
}

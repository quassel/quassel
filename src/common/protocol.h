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

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <QByteArray>
#include <QDateTime>
#include <QVariantList>

namespace Protocol {

enum Handler {
    SignalProxy
};


/*** handled by SignalProxy ***/

class SyncMessage
{
public:
    inline SyncMessage(const QByteArray &className, const QString &objectName, const QByteArray &slotName, const QVariantList &params)
    : _className(className), _objectName(objectName), _slotName(slotName), _params(params) {}

    inline Handler handler() const { return SignalProxy; }

    inline QByteArray className() const { return _className; }
    inline QString objectName() const { return _objectName; }
    inline QByteArray slotName() const { return _slotName; }

    inline QVariantList params() const { return _params; }

private:
    QByteArray _className;
    QString _objectName;
    QByteArray _slotName;
    QVariantList _params;
};


class RpcCall
{
public:
    inline RpcCall(const QByteArray &slotName, const QVariantList &params)
    : _slotName(slotName), _params(params) {}

    inline Handler handler() const { return SignalProxy; }

    inline QByteArray slotName() const { return _slotName; }
    inline QVariantList params() const { return _params; }

private:
    QByteArray _slotName;
    QVariantList _params;
};


class InitRequest
{
public:
    inline InitRequest(const QByteArray &className, const QString &objectName)
    : _className(className), _objectName(objectName) {}

    inline Handler handler() const { return SignalProxy; }

    inline QByteArray className() const { return _className; }
    inline QString objectName() const { return _objectName; }

private:
    QByteArray _className;
    QString _objectName;
};


class InitData
{
public:
    inline InitData(const QByteArray &className, const QString &objectName, const QVariantMap &initData)
    : _className(className), _objectName(objectName), _initData(initData) {}

    inline Handler handler() const { return SignalProxy; }

    inline QByteArray className() const { return _className; }
    inline QString objectName() const { return _objectName; }

    inline QVariantMap initData() const { return _initData; }

private:
    QByteArray _className;
    QString _objectName;
    QVariantMap _initData;
};


/*** handled by RemoteConnection ***/

class HeartBeat
{
public:
    inline HeartBeat(const QDateTime &timestamp) : _timestamp(timestamp) {}

    inline QDateTime timestamp() const { return _timestamp; }

private:
    QDateTime _timestamp;
};


class HeartBeatReply
{
public:
    inline HeartBeatReply(const QDateTime &timestamp) : _timestamp(timestamp) {}

    inline QDateTime timestamp() const { return _timestamp; }

private:
    QDateTime _timestamp;
};


};

#endif
